// Copyright 2011 RWTH Aachen University. All rights reserved.
//
// Licensed under the RWTH ASR License (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.hltpr.rwth-aachen.de/rwth-asr/rwth-asr-license.html
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "AlignmentNode.hh"
#include "AllophoneStateGraphBuilder.hh"
#include <Core/Directory.hh>
#include <Flow/DataAdaptor.hh>
#include "FsaCache.hh"
#include <Bliss/Orthography.hh>
#include <Fsa/Best.hh>
#include <Fsa/Basic.hh>
#include <Fsa/Compose.hh>
#include <Fsa/Output.hh>
#include <Fsa/Project.hh>
#include <Fsa/Rational.hh>
#include <Fsa/Determinize.hh>
#include <Fsa/Minimize.hh>
#include <Fsa/RemoveEpsilons.hh>
#include <Core/Hash.hh>
#include <Lattice/Archive.hh>
#include <Lattice/Utilities.hh>
#include <Mm/FeatureScorer.hh>

using namespace Speech;
using Search::Aligner;


/** AlignmentBaseNode
 */
const Core::ParameterString AlignmentBaseNode::paramSegmentId(
    "id", "segment identifier for model acceptor cache.");
const Core::ParameterString AlignmentBaseNode::paramOrthography(
    "orthography", "orthography of current segment");

AlignmentBaseNode::AlignmentBaseNode(const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c),
    allophoneStateGraphBuilder_(0),
    modelCache_(0),
    needInit_(true)
{
    segmentId_ = paramSegmentId(c);
    orthography_ = paramOrthography(c);
}

AlignmentBaseNode::~AlignmentBaseNode()
{
    delete modelCache_;
    delete allophoneStateGraphBuilder_;
}

bool AlignmentBaseNode::setParameter(const std::string &name, const std::string &value)
{
    if (paramSegmentId.match(name))
	segmentId_ = paramSegmentId(value);
    else if (paramOrthography.match(name))
	orthography_ = paramOrthography(value);
    else
	return false;
    return true;
}

bool AlignmentBaseNode::configure(const Flow::Datatype *type)
{
    Core::Ref<Flow::Attributes> attributes(new Flow::Attributes);
    getInputAttributes(0, *attributes);
    if (!configureDatatype(attributes, type)) return false;
    attributes->set("datatype", Flow::DataAdaptor<Alignment>::type()->name());
    return AlignmentBaseNode::configure() && putOutputAttributes(0, attributes);
}

bool AlignmentBaseNode::configure()
{
    if (segmentId_.empty()) {
	error("Segment identifier is not given.");
	return false;
    }
    if (orthography_.empty()) {
	warning() << "Orthography is not given for segment " << segmentId_;
	return false;
    }
    return true;
}


// ================================================================================


/** AlignmentNode
 */
const Core::ParameterBool AlignmentNode::paramStoreLattices(
    "store-lattices", "store word lattices in archive", false);

const Core::ParameterBool AlignmentNode::paramUseTracebacks(
    "use-tracebacks", "align to tracebacks from archive instead of orthography", false);

const Core::ParameterBool AlignmentNode::paramWriteAlphabet(
    "write-alignment-alphabet", "write alphabet information into written alignments", true);

const Core::Choice AlignmentNode::choicePhonemeSequenceSet(
    "lemma-loop", lemmaLoop,
    "phone-loop", phoneLoop,
    "orthography", orthography,
    Core::Choice::endMark());

const Core::ParameterChoice AlignmentNode::paramPhonemeSequenceSet(
    "phoneme-sequence-set",
    &choicePhonemeSequenceSet,
    "determines the set of phoneme sequences entering the alignment",
    orthography);

const Core::ParameterBool AlignmentNode::paramNoDependencyCheck(
	"no-dependency-check",
	"do not check any dependencies",
	false
	);

AlignmentNode::AlignmentNode(const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c),
    wordLatticeBuilder_(0),
    phonemeSequenceSet_((PhonemeSequenceSet)paramPhonemeSequenceSet(config)),
    noDependencyCheck_(paramNoDependencyCheck(c)),
    tracebackChannel_(config, "traceback"),
    latticeArchiveWriter_(0),
    tracebackArchiveReader_(0),
    transducerCache_(0),
    aligner_(select("aligner")),
    writeAlphabet_(false)
{}

AlignmentNode::~AlignmentNode()
{
    delete transducerCache_;
    delete latticeArchiveWriter_;
    delete tracebackArchiveReader_;
    delete wordLatticeBuilder_;
}

bool AlignmentNode::configure()
{
    if (!Precursor::configure(Feature::FlowFeature::type())) return false;
    if (needInit_) initialize();
    createModel();
    return true;
}

void AlignmentNode::createModel()
{
    verify(modelCache_);
    verify(allophoneStateGraphBuilder_);

    Fsa::ConstAutomatonRef transducer; // allophone state to lemma-pronunciation transducer
    if (wordLatticeBuilder_) {
	if (phonemeSequenceSet_ != orthography) {
	    criticalError("model type must be forced-alignment");
	}
	if (tracebackArchiveReader_) {
	    warning("transducer will be overwritten");
	}
	verify(transducerCache_);
	transducer = transducerCache_->get(
	    allophoneStateGraphBuilder_->createTransducerFunctor(segmentId_, orthography_));
	wordLatticeBuilder_->setModelTransducer(transducer);
    }

    Fsa::ConstAutomatonRef model = modelCache_->find(segmentId_);
    if (!model) {
	if (tracebackArchiveReader_) {
	    Lattice::ConstWordLatticeRef lattice= tracebackArchiveReader_->get(segmentId_);
	    transducer= allophoneStateGraphBuilder_->buildTransducer(
		Fsa::projectInput(
		    Fsa::composeMatching(
			Fsa::invert(lemmaPronunciationToLemma_),
			lattice->part(Lattice::WordLattice::acousticFsa))));
	}
	if (transducer) {
	    model = modelCache_->get(
		allophoneStateGraphBuilder_->createFinalizationFunctor(segmentId_, transducer));
	} else if (phonemeSequenceSet_ == orthography) {
	    model = modelCache_->get(
		allophoneStateGraphBuilder_->createFunctor(segmentId_, orthography_));
	} else if (phonemeSequenceSet_ == lemmaLoop || phonemeSequenceSet_ == phoneLoop) {
	    model = aligner_.getModel();
	    if (!model) {
		if (phonemeSequenceSet_ == lemmaLoop) {
		    model = modelCache_->get(
			allophoneStateGraphBuilder_->createFunctor(segmentId_, AllophoneStateGraphBuilder::lemma));
		} else {
		    model = modelCache_->get(
			allophoneStateGraphBuilder_->createFunctor(segmentId_, AllophoneStateGraphBuilder::phone));
		}
	    }
	} else {
	    criticalError("cannot create/set model");
	}
    }
    else if (tracebackArchiveReader_) {
	warning("Using model acceptor from cache. Dependencies on Traceback Archives are not handled.");
    }
    acousticModel_->setKey(segmentId_);
    aligner_.setModel(model, acousticModel_);
}

void AlignmentNode::initialize()
{
    ModelCombination modelCombination(select("model-combination"), ModelCombination::useAcousticModel);
    modelCombination.load();
    acousticModel_ = modelCombination.acousticModel();
    verify(!allophoneStateGraphBuilder_);
    allophoneStateGraphBuilder_ = new AllophoneStateGraphBuilder(
	select("allophone-state-graph-builder"),
	modelCombination.lexicon(),
	modelCombination.acousticModel());

    Core::DependencySet dependencies;
    modelCombination.getDependencies(dependencies);

    verify(!modelCache_);
    modelCache_ = new FsaCache(select("model-acceptor-cache"), Fsa::storeStates);
    modelCache_->setDependencies(dependencies);
    modelCache_->respondToDelayedErrors();

    verify(!transducerCache_);
    verify(!latticeArchiveWriter_);
    verify(!tracebackArchiveReader_);
    if (tracebackChannel_.isOpen() || paramStoreLattices(config)) {
	wordLatticeBuilder_ = new Aligner::WordLatticeBuilder(
	    select("word-lattice-builder"),
	    modelCombination.lexicon(),
	    acousticModel_);

	if (paramStoreLattices(config)) {
	    log("opening lattice archive");
	    latticeArchiveWriter_ = Lattice::Archive::openForWriting(select("lattice-archive"), modelCombination.lexicon());
	    if (latticeArchiveWriter_->hasFatalErrors()) {
		delete latticeArchiveWriter_; latticeArchiveWriter_ = 0;
	    }
	}
	Core::DependencySet dependencies;
	modelCombination.getDependencies(dependencies);
	transducerCache_ = new FsaCache(select("transducer-cache"), Fsa::storeStates);
	transducerCache_->setDependencies(dependencies);
	transducerCache_->forceInputAlphabet(acousticModel_->allophoneStateAlphabet());
	transducerCache_->respondToDelayedErrors();
    }

    if (paramUseTracebacks(config)) {
	log("opening traceback archive");
	tracebackArchiveReader_ = Lattice::Archive::openForReading(select("traceback-archive"),
		modelCombination.lexicon());
	if (tracebackArchiveReader_->hasFatalErrors()) {
	    delete tracebackArchiveReader_; tracebackArchiveReader_ = 0;
	}
	lemmaPronunciationToLemma_= modelCombination.lexicon()->createLemmaPronunciationToLemmaTransducer();
    }

    writeAlphabet_ = paramWriteAlphabet(config);

    needInit_ = false;
}

bool AlignmentNode::work(Flow::PortId p)
{
    Flow::DataAdaptor<Alignment> *alignment = new Flow::DataAdaptor<Alignment>();
    alignment->invalidateTimestamp();

    if(writeAlphabet_)
	alignment->data().setAlphabet(acousticModel_->allophoneStateAlphabet());

    bool firstFeature = true;
    Flow::DataPtr<Feature::FlowFeature> in;
    std::vector<Mm::FeatureScorer::Scorer> scorers;
    // reset feature scorer for usage with embedded flow files
    acousticModel_->featureScorer()->reset();
    while(getData(0, in)) {
	Core::Ref<const Feature> feature(new Feature(in));
	if (firstFeature) {
	    checkFeatureDependencies(*feature);
	    firstFeature = false;
	}
	scorers.push_back(acousticModel_->featureScorer()->getScorer(feature));
	alignment->expandTimestamp(feature->timestamp());
	if (tracebackChannel_.isOpen())
	    featureTimes_.push_back(feature->timestamp());
    }
    // finalize embedded network if applicable i.e. EOS
    if(firstFeature){
	acousticModel_->featureScorer()->finalize();
    }

    aligner_.feed(scorers);
    if (!aligner_.reachedFinalState())
	warning("Alignment did not reach any final state in segment '%s'", segmentId_.c_str());

    Fsa::ConstAutomatonRef alignmentFsa = aligner_.getAlignmentFsa();
    if (aligner_.reachedFinalState() && (paramStoreLattices(config) || tracebackChannel_.isOpen()))
	createWordLattice(alignmentFsa);

    std::pair<Fsa::ConstAutomatonRef, Fsa::Weight> alignmentPosteriorFsa = aligner_.getAlignmentPosteriorFsa(alignmentFsa);
    aligner_.getAlignment(alignment->data(), alignmentPosteriorFsa);

    featureTimes_.clear();

    return putData(0, alignment) && putData(0, in.get());
}

void AlignmentNode::checkFeatureDependencies(const Mm::Feature &feature) const
{
	if(!noDependencyCheck_){
		Mm::FeatureDescription description(*this, feature);
		if (!acousticModel_->isCompatible(description))
			acousticModel_->respondToDelayedErrors();
	}
}

void AlignmentNode::createWordLattice(Fsa::ConstAutomatonRef alignmentFsa) const
{
    verify(wordLatticeBuilder_);

    Lattice::ConstWordLatticeRef wordLattice(wordLatticeBuilder_->build(alignmentFsa));
    if (wordLattice) {
	if (paramStoreLattices(config)) {
	    verify(latticeArchiveWriter_);
	    latticeArchiveWriter_->store(segmentId_, wordLattice);
	}
	if (tracebackChannel_.isOpen())
	    logTraceback(wordLattice);
    }
    else
	error("Failed to generate word lattice for the segment '%s'.", segmentId_.c_str());
}


void AlignmentNode::logTraceback(Lattice::ConstWordLatticeRef wordLattice) const
{
    Fsa::ConstAutomatonRef fn = wordLattice->part(Lattice::WordLattice::acousticFsa);
    Fsa::ConstStateRef state = fn->getState(fn->initialStateId());
    Fsa::ConstAlphabetRef ia = fn->getInputAlphabet();

    u32 previousIndex = wordLattice->time(state->id());
    tracebackChannel_ << Core::XmlOpen("traceback") + Core::XmlAttribute("type", "xml");
    while( !state->isFinal() ) {
	u32 index = wordLattice->time(state->begin()->target());
	tracebackChannel_ << Core::XmlOpen("item") + Core::XmlAttribute("type", "lemma-pronunciation")
			  << Core::XmlFull("lemma-pronunciation", ia->symbol(state->begin()->input()))
			  << Core::XmlFull("score", f32(state->begin()->weight())) + Core::XmlAttribute("type", "acoustic")
			  << Core::XmlEmpty("samples") + Core::XmlAttribute("start", f32(featureTimes_[previousIndex].startTime())) +
	    Core::XmlAttribute("end", f32(featureTimes_[index - 1].endTime()))
			  << Core::XmlEmpty("features") + Core::XmlAttribute("start", previousIndex) +
	    Core::XmlAttribute("end", index - 1)
			  << Core::XmlClose("item");
	previousIndex = wordLattice->time(state->begin()->target());
	state = fn->getState(state->begin()->target());
    }
    tracebackChannel_ << Core::XmlClose("traceback");
}


/** AlignmentDumpNode
 */
const Core::ParameterString AlignmentDumpNode::paramFilename(
    "file", "text file for alignment dump", "");

const Core::ParameterString AlignmentDumpNode::paramSegmentId(
    "id", "segment identifier for plaintext archive.");


AlignmentDumpNode::AlignmentDumpNode(const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c),
    archive_(0),
    writer_(0),
    reader_(0),
    archiveExists_(false),
    attributesParser_(select("attributes-parser"))
{
    addInputs(2);
    addOutputs(1);
    ModelCombination modelCombination(
	select("model-combination"), ModelCombination::useAcousticModel,
	Am::AcousticModel::noEmissions | Am::AcousticModel::noStateTying | Am::AcousticModel::noStateTransition);
    modelCombination.load();
    acousticModel_ = modelCombination.acousticModel();
    filename_ = paramFilename(c);
    segmentId_ = paramSegmentId(c);
}

AlignmentDumpNode::~AlignmentDumpNode()
{
    delete archive_;
    archive_ = 0;
}

bool AlignmentDumpNode::hasParameters(const std::string &s)
{
    std::vector<std::string> requiredParams;
    Core::strconv(s, requiredParams);
    for (std::vector<std::string>::const_iterator i = requiredParams.begin(); i != requiredParams.end(); ++i)
	if (parameters_.find(*i) == parameters_.end()) {
	    warning("required parameter '%s' is missing.", i->c_str() );
	    return false;
	}
    return true;
}

bool AlignmentDumpNode::configure() {
    if (!hasParameters("id"))
	error("Missing some parameters.");

    Core::Ref<Flow::Attributes> attributes;

    if (archiveExists_) {
	alignmentType_ = AlignmentType(plainText);
	Core::ArchiveReader r(*archive_, segmentId_ + ".attribs");
	if (r.isOpen()) {
	    Core::Ref<Flow::Attributes> ca(new Flow::Attributes());
	    if (attributesParser_.buildFromStream(*ca, r)) {
		std::string datatype = ca->get("datatype");
		if (datatype.empty()) {
		    error("datatype '%s' from archive is unknown", datatype.c_str());
		}
		attributes = ca;
		attributes->set("datatype", Flow::DataAdaptor<Alignment>::type()->name());
	    }
	}
    } else if (inputConnected(0)) {
	attributes = Core::ref(new Flow::Attributes);
	getInputAttributes(0, *attributes);
	if (!configureDatatype(attributes, Flow::DataAdaptor<Alignment>::type()))
	    return false;

    } else
	attributes = Core::ref(new Flow::Attributes());

    if (writer_) {
	alignmentType_ = AlignmentType(standard);
	Core::ArchiveWriter w(*archive_, segmentId_ + ".attribs", false);
	attributes->set("datatype", Flow::DataAdaptor<std::string>::type()->name());
	if (writer_->isOpen()) {
	    Core::XmlWriter xw(w);
	    xw << *attributes;
	}
	attributes->set("datatype", Flow::DataAdaptor<Alignment>::type()->name());
    }
    return putOutputAttributes(0, attributes);

}

Core::ArchiveWriter* AlignmentDumpNode::newWriter(const std::string &name) {
    verify(archive_);
    Core::ArchiveWriter *writer = new Core::ArchiveWriter(*archive_, name, false);
    if (writer->isOpen()) return writer;
    delete writer;
    return 0;
}

Core::ArchiveReader* AlignmentDumpNode::newReader(const std::string &name) {
    verify(archive_);
    Core::ArchiveReader *reader = new Core::ArchiveReader(*archive_, name);
    if (reader->isOpen()) return reader;
    delete reader;
    return 0;
}

bool AlignmentDumpNode::open(Core::Archive::AccessMode _access) {
    if (isOpen()) close();
    if (filename_.empty() ||
	(!Core::isValidPath(filename_) &&
	 !(_access & Core::Archive::AccessModeWrite))) return false;
    archive_ = Core::Archive::create(config, filename_, _access);
    return isOpen();
}

void AlignmentDumpNode::close() {
    if (!isOpen()) return;
    delete archive_; archive_ = 0;
}


bool AlignmentDumpNode::createContext(const std::string &id) {
    reader_ = 0;

    // Open archive first only for reading. independent if node has an input.
    if (!hasAccess(Core::Archive::AccessModeRead)) {
	if (!open(Core::Archive::AccessModeRead)) {
	    // Report error only if node has no input.
	    if (!inputConnected(0)) {
		error("Failed to open archive '%s' for reading and "	\
		      "node has no input.", filename_.c_str());
		return false;
	    }
	}
    }
    archiveExists_ = false;
    if (hasAccess(Core::Archive::AccessModeRead))
	archiveExists_ = archive_->hasFile(segmentId_ = id);
    if (archiveExists_) {
	return true;
    } else if (inputConnected(0)) {
	// Open archive for writing only on demand.
	if (!hasAccess(Core::Archive::AccessModeWrite)) {
	    if (!open(Core::Archive::AccessModeRead | Core::Archive::AccessModeWrite)) {
		error("Failed to open archive '%s'.", filename_.c_str());
		return false;
	    }
	}
	verify(hasAccess(Core::Archive::AccessModeWrite));
	writer_ = newWriter(id);
	if (!writer_) {
	    error("Failed to prepare writing to cache for id \"%s\".",
		  id.c_str());
	}
	return true;
    }
    error("Cannot provide data for id \"%s\": "
	  "Neither archive entry nor input available.",
	  id.c_str());
    return false;

}


bool AlignmentDumpNode::setParameter(const std::string &name, const std::string &value) {
    parameters_[name] = value;
    if (paramFilename.match(name))
	filename_ = paramFilename(value);
    if (paramSegmentId.match(name)) {
	segmentId_ = paramSegmentId(value);
	return createContext(value);
    }
    return true;
}

bool AlignmentDumpNode::work(Flow::PortId p) {

    // converts standard alignment to plain text alignment
    if (alignmentType_ == standard) {
	Flow::DataPtr<Flow::DataAdaptor<Alignment> > in;

	if (!writer_) {
	    if (segmentId_.empty())
		criticalError("File not given.");
	}
	Core::TextOutputStream o(writer_);
	require(o && o.good());
	o.setEncoding("UTF-8");

	while (getData(0, in)) {
	    Alignment &a = in->data();

	    // dump segment information
	    std::string plainData ="";

	    TimeframeIndex time = 0;
	    Alignment::const_iterator i,j;

	    // dump alignment
	    std::vector<Alignment::Frame> frames;
	    a.getFrames(frames);

	    for (std::vector<Alignment::Frame>::const_iterator i = frames.begin(); i != frames.end(); ++i) {
		// print feature and feature times if the input link is connected
		if (inputConnected(1)) {
		    Flow::DataPtr<Feature::FlowFeature> f;
		    if (getData(1, f)) {
			plainData = Core::form("%i %.3f %.3f ", time, f->startTime(), f->endTime());
			if (writer_->isOpen()) {
			    o << plainData;
			    if (!o)
				error("could not write data \"%s\" to archive \"%s\"", segmentId_.c_str(),
				      archive_->path().c_str());
			} else {
			    error("could not open file \"%s\" for writing in archive \"%s\"",
				  segmentId_.c_str(), archive_->path().c_str());
			}
		    } else {
			error("Feature stream and alignment not synchronized at time %d.", time);
		    }
		}

		// print alignment items
		Alignment::iterator j, j_end;
		for (Core::tie(j,j_end) = *i; j != j_end; ++j) {
		    plainData = Core::form("%s %g\n",
					   acousticModel_->allophoneStateAlphabet()->symbol(j->emission).c_str(),
					   j->weight);
		    if (writer_->isOpen()) {
			o << plainData;
			if (!o)
			    error("could not write data \"%s\" to archive \"%s\"", segmentId_.c_str(),
				  archive_->path().c_str());
		    } else {
			error("could not open file \"%s\" for writing in archive \"%s\"",
			      segmentId_.c_str(), archive_->path().c_str());
		    }
		}
		++time;
	    }
	    putData(0, in.get());
	}
	o.close();

	if (inputConnected(1)) {
	    Flow::DataPtr<Feature::FlowFeature> f;
	    if (getData(1, f))
		warning("Feature stream and alignment not synchronized: There are features remaining.");
	}
	return putData(0, in.get());

	//converts plain Text alignments to standard alignments
    } else if (alignmentType_ == plainText) {
	if (!reader_)
	    reader_ = newReader(segmentId_);

	if (!reader_) {
	    if (segmentId_.empty())
		criticalError("File not given.");
	    else
		return false;
	}

	Core::TextInputStream i(reader_);

	require(i);
	require(i.good());
	i.setEncoding("UTF-8");

	Flow::DataPtr<Flow::DataAdaptor<std::string> > in;
	Alignment a;

	TimeframeIndex timeFrame = 0;
	f64 weight = 0;
	f64 startTime = 0.0;
	f64 endTime = 0.0;
	std::string allophoneString = "";

	f64 previousStartTime = 0.0;
	f64 previousEndTime = 0.0;
	f64 timeStampStart = 0.0;
	f64 timeStampEnd = 0.0;
	bool firstSegment = true;
	bool singleSegment = true;

	std::string line;

	while (Core::getline(i, line) != EOF) {

	    previousEndTime = endTime;

	    std::vector<std::string> fields = Core::split(line, " ");
	    Core::strconv(fields[0], timeFrame);
	    Core::strconv(fields[1], startTime);
	    Core::strconv(fields[2], endTime);
	    Core::strconv(fields[3], allophoneString);
	    Core::strconv(fields[4], weight);

	    if (timeFrame == 0 && firstSegment) {
		timeStampStart = startTime;
		firstSegment = false;
	    }

	    else if (timeFrame == 0 && !firstSegment) {

		singleSegment = false;
		previousStartTime = timeStampStart;
		timeStampEnd = previousEndTime;

		Flow::DataAdaptor<Alignment> *alignment = new Flow::DataAdaptor<Alignment>(a);
		alignment->setTimestamp(Flow::Timestamp(previousStartTime, timeStampEnd));
		putData(0, alignment);

		timeStampStart = startTime;
	    }

	    Fsa::LabelId index = acousticModel_->allophoneStateAlphabet()->index(allophoneString);
	    a.push_back(AlignmentItem(timeFrame, index, weight));
	}
	i.close();
	if (singleSegment) {
	    timeStampEnd = endTime;
	    Flow::DataAdaptor<Alignment> *alignment = new Flow::DataAdaptor<Alignment>(a);
	    alignment->setTimestamp(Flow::Timestamp(timeStampStart, timeStampEnd));
	    return putData(0, alignment);
	}
	else {
	    timeStampEnd = endTime;
	    Flow::DataAdaptor<Alignment> *alignment = new Flow::DataAdaptor<Alignment>(a);
	    alignment->setTimestamp(Flow::Timestamp(timeStampStart, timeStampEnd));
	    return putData(0, alignment);
	}
    } else {
	criticalError("Input format must either be standard or plain text");
	return false;
    }
}
