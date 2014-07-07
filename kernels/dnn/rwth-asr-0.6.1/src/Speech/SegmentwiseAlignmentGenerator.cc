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
#include "SegmentwiseAlignmentGenerator.hh"
#include "FsaCache.hh"
#include <Bliss/CorpusDescription.hh>
#include <Core/Application.hh>
#include <Core/Archive.hh>
#include <Fsa/Automaton.hh>
#include <Fsa/Basic.hh>
#include <Fsa/Cache.hh>
#include <Fsa/Properties.hh>
#include <Fsa/Sssp.hh>
#include <Mm/Feature.hh>
#include <Fsa/Dfs.hh>
#include <Lattice/Archive.hh>

using namespace Speech;

//===============================================================================================
const Core::ParameterString SegmentwiseAlignmentGenerator::paramPortName(
    "port-name", "name of the port", "features");

const Core::ParameterStringVector SegmentwiseAlignmentGenerator::paramSilencesAndNoises(
    "silences-and-noises",
    "list of silence and noise lemmata (strings)",
    "," );

const Core::ParameterBool SegmentwiseAlignmentGenerator::paramCheckCompatibility(
    "check-compatibility",
    "check compatibility of features and acoustic model",
    true);


SegmentwiseAlignmentGenerator::SegmentwiseAlignmentGenerator(
    const Core::Configuration &c, Core::Ref<const ModelCombination> modelCombination) :
    Core::Component(c),
    portId_(Flow::IllegalPortId),
    modelCombination_(modelCombination),
    allophoneStateGraphBuilder_(0),
    aligner_(select("aligner")),
    checkCompatibility_(paramCheckCompatibility(c))
{
    if (!modelCombination_) {
	ModelCombinationRef mc = ModelCombinationRef(new ModelCombination(select("model-combination"), ModelCombination::useAcousticModel));
	mc->load();
	modelCombination_ = mc;
    }
    verify(modelCombination_->acousticModel());
    allophoneStateGraphBuilder_ = new AllophoneStateGraphBuilder(
	select("allophone-state-graph-builder"),
	modelCombination_->lexicon(),
	modelCombination_->acousticModel());
    std::vector <std::string> silencesAndNoises = paramSilencesAndNoises(config);
    allophoneStateGraphBuilder_->setSilencesAndNoises(silencesAndNoises);
    modelCombination_->getDependencies(dependencySet_);
}

SegmentwiseAlignmentGenerator::~SegmentwiseAlignmentGenerator()
{
    delete allophoneStateGraphBuilder_;
}

FsaCache * SegmentwiseAlignmentGenerator::createFsaCache(const Core::Configuration &config)
{
    FsaCache *fsaCache = new FsaCache(config, Fsa::storeStates);
    fsaCache->setDependencies(dependencies());
    return fsaCache;
}

void SegmentwiseAlignmentGenerator::setSegmentwiseFeatureExtractor(
    Core::Ref<SegmentwiseFeatureExtractor> segmentwiseFeatureExtractor)
{
    segmentwiseFeatureExtractor_ = segmentwiseFeatureExtractor;
    portId_ = segmentwiseFeatureExtractor_->addPort(paramPortName(config));
    if (portId_ == Flow::IllegalPortId)
	criticalError("Failed to retrieve output from flow network.");
}

void SegmentwiseAlignmentGenerator::update(const std::string &segmentId)
{
    acousticModel()->setKey(segmentId);
    if (segmentwiseFeatureExtractor_ && checkCompatibility_) {
	segmentwiseFeatureExtractor_->checkCompatibility(portId_, acousticModel());
    }
    features_.reset();
}

void SegmentwiseAlignmentGenerator::setSpeechSegmentId(const std::string &segmentId)
{
    update(segmentId);
}

void SegmentwiseAlignmentGenerator::setSpeechSegment(Bliss::SpeechSegment *segment)
{
    verify(segment);
    update(segment->fullName());
}

void SegmentwiseAlignmentGenerator::signOn(Speech::CorpusVisitor &corpusVisitor)
{
    const_cast<Am::AcousticModel*>(acousticModel().get())->signOn(corpusVisitor);
}

void SegmentwiseAlignmentGenerator::getScorers(
    std::vector<Mm::FeatureScorer::Scorer> &scorers) const
{
    Core::Ref<const Mm::FeatureScorer> featureScorer(acousticModel()->featureScorer());
    scorers.clear();
    for(SegmentwiseFeatures::const_iterator f = features()->begin(), featureEnd = features()->end(); f != featureEnd; ++ f) {
	scorers.push_back(featureScorer->getScorer(*f));
    }
}

void SegmentwiseAlignmentGenerator::getScorers(
    TimeframeIndex tbeg, TimeframeIndex tend,
    std::vector<Mm::FeatureScorer::Scorer> &scorers) const
{
    // ensure(tend <= features().size());
    if (features()->size() < tend) {
	if (features()->size() <= tbeg) {
	    criticalError("Requested alignment is out of range, [%d,%d] not in [0,%zu].",
			  tbeg, tend, features()->size());
	} else {
	    warning("Requested alignment is out of range, [%d,%d] not in [0,%zu]; align [%d,%zu].",
		    tbeg, tend, features()->size(), tbeg, features()->size());
	    tend = features()->size();
	}
    }

    /*
     * Do not use iterators here because pointer arithmetic
     * and polymorphism (e.g. Sparse::SegmentwiseFeatures)
     * are not compatible.
     */
    require(tbeg <= tend);
    scorers.resize(tend - tbeg);
    Core::Ref<const Mm::FeatureScorer> featureScorer(modelCombination()->acousticModel()->featureScorer());
    if (featureScorer->hasTimeIndexedCache()){
	for(u32 t = tbeg; t < tend; ++ t) {
	    scorers[t - tbeg] = featureScorer->getTimeIndexedScorer(t);
	}
    }
    else{
    for(u32 t = tbeg; t < tend; ++ t) {
	Core::Ref<Mm::Feature> f = (*features())[t];
	scorers[t - tbeg] = featureScorer->getScorer(f);
	}
    }
}


//===============================================================================================


namespace {
    const Core::ParameterString paramPath(
	"path", "path", "");
} // namespace

OrthographyAlignmentGenerator::OrthographyAlignmentGenerator(const Core::Configuration &c, Core::Ref<const ModelCombination> mc) :
    Precursor(c, mc)
{
    transducerCache_ = createFsaCache(select("transducer-cache"));
    modelAcceptorCache_ = createFsaCache(select("model-acceptor-cache"));
    wordLatticeBuilder_ = new Search::Aligner::WordLatticeBuilder(
	select("word-lattice-builder"),
	modelCombination()->lexicon(),
	acousticModel());
    lemmaPronunciationToLemma_ = modelCombination()->lexicon()->createLemmaPronunciationToLemmaTransducer();
    std::string alignmentCachePath = paramPath(select("alignment-cache"));
    if (!alignmentCachePath.empty()) {
	log("opening alignment cache");
	alignmentCache_ = createAlignmentCache(select("alignment-cache"));
    } else
	alignmentCache_ = 0;
    isValid_ = false;
}

OrthographyAlignmentGenerator::~OrthographyAlignmentGenerator()
{
    delete transducerCache_;
    delete modelAcceptorCache_;
    delete wordLatticeBuilder_;
    delete alignmentCache_;
}


void OrthographyAlignmentGenerator::update(const std::string &segmentId, const std::string &orthography)
{
    segmentId_ = segmentId;
    orthography_ = orthography;
    alignmentFsa_.reset();
    isValid_ = false;
}

void OrthographyAlignmentGenerator::setSpeechSegment(Bliss::SpeechSegment *segment)
{
    Precursor::setSpeechSegment(segment);
    update(segment->fullName(), segment->orth());
}

void OrthographyAlignmentGenerator::setSpeechSegmentId(const std::string &segmentId)
{
    Precursor::setSpeechSegmentId(segmentId);
    update(segmentId, "");
}

Core::Archive * OrthographyAlignmentGenerator::createAlignmentCache(const Core::Configuration &config)
{
    Core::Archive * alignmentCache = Core::Archive::create(
	config, paramPath(config), Core::Archive::AccessModeReadWrite);
    Core::ArchiveReader ar(*alignmentCache, "DEPENDENCIES");
    if (ar.isOpen()) {
	Core::DependencySet archiveDependencies;
	if (!archiveDependencies.read(config, ar)) {
	    error("Failed to read dependencies from alignment cache \"%s\".",
		  alignmentCache->path().c_str());
	} else if (archiveDependencies == dependencies()) {
	    warning("Archive \"%s\" exists already and will be overwritten.",
		    alignmentCache->path().c_str());
	}
    } else {
	Core::XmlOutputStream xw(new Core::ArchiveWriter(*alignmentCache, "DEPENDENCIES"));
	dependencies().write(xw);
    }
    return alignmentCache;
}

void OrthographyAlignmentGenerator::setOrthography(const std::string &orthography)
{
    if (!orthography.empty() && (orthography_ != orthography)) {
	orthography_ = orthography;
	isValid_ = false;
    }
}

Fsa::ConstAutomatonRef OrthographyAlignmentGenerator::getAlignmentFsa()
{
    verify(!orthography_.empty());
    if (!isValid_) {
	// allophone state to lemma-pronunciation transducer
	Fsa::ConstAutomatonRef transducer = transducerCache_->get(
	    allophoneStateGraphBuilder_->createTransducerFunctor(segmentId_/* + "/" + orthography_ */, orthography_));
	wordLatticeBuilder_->setModelTransducer(transducer);
	// ???
	Fsa::ConstAutomatonRef modelAcceptor = modelAcceptorCache_->get(
	    allophoneStateGraphBuilder_->createFinalizationFunctor(segmentId_ /* + "/" + orthography_ */, transducer));
	aligner_.setModel(modelAcceptor, acousticModel());

	std::vector<Mm::FeatureScorer::Scorer> scorers;
	getScorers(scorers);
	aligner_.feed(scorers);
	if (aligner_.reachedFinalState()) {
	    alignmentFsa_ = aligner_.getAlignmentFsa();
	    verify(alignmentFsa_);
	} else
	    warning("Alignment did not reach any final state in segment '%s'", segmentId_.c_str());
	isValid_ = true;
    }
    return alignmentFsa_;
}

const Alignment* OrthographyAlignmentGenerator::getAlignment(const std::string &orthography)
{
    setOrthography(orthography);
    if (alignmentCache_) {
	Core::ArchiveReader reader(*alignmentCache_, segmentId_/* + "/" + orthography_ */);
	if (reader.isOpen()) {
	    Alignment *alignment = new Alignment;
	    Core::BinaryInputStream bi(reader);
	    bi >> *alignment;
	    if (!bi)
		error("Failed to read alignment \"%s\" from cache \"%s\".",
		      segmentId_.c_str(), alignmentCache_->path().c_str());
	    return alignment;
	}
    }
    Fsa::ConstAutomatonRef alignmentFsa = getAlignmentFsa();
    if (!alignmentFsa)
	return 0;
    std::pair<Fsa::ConstAutomatonRef, Fsa::Weight> alignmentPosteriorFsa = aligner_.getAlignmentPosteriorFsa(alignmentFsa);
    Alignment *alignment = new Alignment;
    aligner_.getAlignment(*alignment, alignmentPosteriorFsa);
    if (alignmentCache_) {
	Core::ArchiveWriter writer(*alignmentCache_, segmentId_/* + "/" + orthography_ */);
	if (writer.isOpen()) {
	    Core::BinaryOutputStream bo(writer);
	    bo << *alignment;
	    if (!bo)
		error("Failed to write alignment \"%s\" to cache \"%s\".",
		      segmentId_.c_str(), alignmentCache_->path().c_str());
	} else {
	    error("Could not open file \"%s\" for writing in cache \"%s\".",
		  segmentId_.c_str(), alignmentCache_->path().c_str());
	}
    }
    return alignment;
}

Lattice::ConstWordLatticeRef OrthographyAlignmentGenerator::getWordLattice(const std::string &orthography)
{
    setOrthography(orthography);
    Fsa::ConstAutomatonRef alignmentFsa = getAlignmentFsa();
    if (!alignmentFsa)
	return Lattice::ConstWordLatticeRef();
    Lattice::ConstWordLatticeRef wordLattice = wordLatticeBuilder_->build(alignmentFsa);
    if (!wordLattice)
	error("Failed to generate word lattice for segment '%s'.", segmentId_.c_str());
    return wordLattice;
}
