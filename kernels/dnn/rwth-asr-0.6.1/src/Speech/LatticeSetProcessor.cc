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
#include <sstream>

#include "LatticeSetProcessor.hh"
#include <Bliss/Evaluation.hh>
#include <Fsa/Basic.hh>
#include <Fsa/Best.hh>
#include <Fsa/Cache.hh>
#include <Fsa/Compose.hh>
#include <Fsa/Determinize.hh>
#include <Fsa/Project.hh>
#include <Fsa/RemoveEpsilons.hh>
#include <Lattice/Arithmetic.hh>
#include <Lattice/Basic.hh>
#include <Lattice/Cache.hh>
#include <Lattice/Static.hh>
#include <Lattice/Compose.hh>
#include <Lattice/Utilities.hh>
#include "DataExtractor.hh"
#include "PhonemeSequenceAlignmentGenerator.hh"

using namespace Speech;

/**
 * LatticeSetProcessor
 */
LatticeSetProcessor::LatticeSetProcessor(const Core::Configuration &c) :
    Core::Component(c),
    initialized_(false),
    timeProcessSegment_(0)
{}

void LatticeSetProcessor::processWordLattice(
    Lattice::ConstWordLatticeRef lattice, Bliss::SpeechSegment *s)
{
    if (lattice) {
	if (processor_) {
	    if (!initialized_) {
		Lattice::WordLatticeDescription description(*this, lattice);
		getWordLatticeDescription(description);
		processor_->setWordLatticeDescription(description);
		initialized_ = true;
	    }
	    processor_->processWordLattice(lattice, s);
	}
    } else {
	error("skip segment because lattice is empty");
    }
}

void LatticeSetProcessor::setSegmentwiseFeatureExtractor(
    Core::Ref<Speech::SegmentwiseFeatureExtractor> segmentwiseFeatureExtractor)
{
    if (processor_) processor_->setSegmentwiseFeatureExtractor(segmentwiseFeatureExtractor);
}

void LatticeSetProcessor::setAlignmentGenerator(
    Core::Ref<Speech::PhonemeSequenceAlignmentGenerator> alignmentGenerator)
{
    if (processor_) processor_->setAlignmentGenerator(alignmentGenerator);
}

void LatticeSetProcessor::logComputationTime() const {
    log("time for lattice processor \"") << name() << "\": " << timeProcessSegment_;
    if (processor_) processor_->logComputationTime();
}

/**
 * LatticeSetProcessorRoot
 */
LatticeSetProcessorRoot::LatticeSetProcessorRoot(const Core::Configuration &c) :
    Core::Component(c),
    CorpusProcessor(c),
    Precursor(c)
{}

LatticeSetProcessorRoot::~LatticeSetProcessorRoot()
{}

void LatticeSetProcessorRoot::signOn(CorpusVisitor &corpusVisitor)
{
    Precursor::signOn(corpusVisitor);
    CorpusProcessor::signOn(corpusVisitor);
}

void LatticeSetProcessorRoot::enterCorpus(Bliss::Corpus *corpus)
{
    CorpusProcessor::enterCorpus(corpus);
    Precursor::enterCorpus(corpus);
}

void LatticeSetProcessorRoot::leaveCorpus(Bliss::Corpus *corpus)
{
    Precursor::leaveCorpus(corpus);
    CorpusProcessor::leaveCorpus(corpus);
}

void LatticeSetProcessorRoot::enterRecording(Bliss::Recording *recording)
{
    CorpusProcessor::enterRecording(recording);
    Precursor::enterRecording(recording);
}

void LatticeSetProcessorRoot::leaveRecording(Bliss::Recording *recording)
{
    Precursor::leaveRecording(recording);
    CorpusProcessor::leaveRecording(recording);
}

void LatticeSetProcessorRoot::enterSegment(Bliss::Segment *s)
{
    CorpusProcessor::enterSegment(s);
    Precursor::enterSegment(s);
}

void LatticeSetProcessorRoot::leaveSegment(Bliss::Segment *s)
{
    Precursor::leaveSegment(s);
    CorpusProcessor::leaveSegment(s);
}

void LatticeSetProcessorRoot::enterSpeechSegment(Bliss::SpeechSegment *s)
{
    CorpusProcessor::enterSpeechSegment(s);
    Precursor::enterSpeechSegment(s);
}

void LatticeSetProcessorRoot::leaveSpeechSegment(Bliss::SpeechSegment *s)
{
    Precursor::leaveSpeechSegment(s);
    CorpusProcessor::leaveSpeechSegment(s);
}

void LatticeSetProcessorRoot::initialize()
{
    ModelCombination modelCombination(select("model-combination"), ModelCombination::useLexicon);
    modelCombination.load();
    initialize(modelCombination.lexicon());
    respondToDelayedErrors();
}

/********************************************************************************************/
/*                             InfoLatticeProcessorNode                                  */
/********************************************************************************************/

InfoLatticeProcessorNode::InfoLatticeProcessorNode(const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c)
{}

InfoLatticeProcessorNode::~InfoLatticeProcessorNode() {}

void InfoLatticeProcessorNode::processWordLattice(
    Lattice::ConstWordLatticeRef lattice, Bliss::SpeechSegment *s)
{
	for(size_t i = 0; i < lattice->nParts(); ++i) {
		Fsa::ConstAutomatonRef fsa = lattice->part(i);
		if (fsa) {
			Component::Message logChannel = log("part \"%s\"", lattice->name(i).c_str());
			Fsa::info(fsa, logChannel);
		}
	}
	std::pair<u32, u32> info = Lattice::densityInfo(lattice);
	log("lattice-info (n-time-frames, n-arcs): %d, %d", info.first, info.second);
    Precursor::processWordLattice(lattice, s);
}

/********************************************************************************************/
/*                             DensityLatticeProcessorNode                                  */
/********************************************************************************************/
const Core::ParameterBool DensityLatticeProcessorNode::paramShallEvaluateArcsPerSpokenWord(
    "shall-evaluate-arcs-per-spoken-word",
    "Fsa::info to calculate lattice density, i.e. number of arcs per spoken word",
    true);

const Core::ParameterBool DensityLatticeProcessorNode::paramShallEvaluateArcsPerTimeframe(
    "shall-evaluate-arcs-per-timeframe",
    "evaluate number of arcs and number of timeframes",
    true);

DensityLatticeProcessorNode::DensityLatticeProcessorNode(const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c),
    shallEvaluateArcsPerSpokenWord_(paramShallEvaluateArcsPerSpokenWord(config)),
    shallEvaluateArcsPerTimeframe_(paramShallEvaluateArcsPerTimeframe(config))
{}

DensityLatticeProcessorNode::~DensityLatticeProcessorNode() {}

void DensityLatticeProcessorNode::processWordLattice(
    Lattice::ConstWordLatticeRef lattice, Bliss::SpeechSegment *s)
{
    if (shallEvaluateArcsPerSpokenWord_) {
	Fsa::info(lattice->part(0), log("lattice"));
    }
    if (shallEvaluateArcsPerTimeframe_) {
	//	log("lattice-density: %f", Lattice::density(lattice));
	std::pair<u32, u32> info = Lattice::densityInfo(lattice);
	log("lattice-info (n-time-frames, n-arcs): %d, %d", info.first, info.second);
    }
    Precursor::processWordLattice(lattice, s);
}

/**
 * GerLatticeProcessorNode
 */
GerLatticeProcessorNode::GerLatticeProcessorNode(const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c),
    evaluator_(0)
{}

GerLatticeProcessorNode::~GerLatticeProcessorNode()
{
    delete evaluator_;
}

void GerLatticeProcessorNode::enterSpeechSegment(Bliss::SpeechSegment *s)
{
    Precursor::enterSpeechSegment(s);
    verify(evaluator_);
    evaluator_->setReferenceTranscription(s->orth());
}

void GerLatticeProcessorNode::processWordLattice(
    Lattice::ConstWordLatticeRef lattice, Bliss::SpeechSegment *s)
{
    verify(evaluator_);
    evaluator_->evaluate(lattice->part(0), "lattice");
    Precursor::processWordLattice(lattice, s);
}

void GerLatticeProcessorNode::initialize(Bliss::LexiconRef lexicon)
{
    Precursor::initialize(lexicon);
    verify(!evaluator_);
    evaluator_ = new Bliss::Evaluator(select("evaluation"), lexicon);
    if (!evaluator_) criticalError("Could not open evaluator.");
}



/**
 * LinearCombinationLatticeProcessorNode
 */
const Core::ParameterStringVector LinearCombinationLatticeProcessorNode::paramOutputs(
    "outputs",
    "output names/selections for linear combinations");

Core::ParameterFloatVector LinearCombinationLatticeProcessorNode::paramScales(
    "scales",
    "build linear combination");

LinearCombinationLatticeProcessorNode::LinearCombinationLatticeProcessorNode(const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c)
{
    const std::vector<std::string> outputs = paramOutputs(config);
    if (outputs.empty()) {
	insert(Lattice::WordLattice::totalFsa, paramScales(config));
	std::vector<f64> scales(paramScales(config));
	std::stringstream ss;
	for (u32 i = 0; i < scales.size(); i++)
	    ss << scales.at(i) << " ";
	log("scales used for linear combination: ") << ss.str();
    } else {
	for (u32 n = 0; n < outputs.size(); ++ n) {
	    insert(outputs[n], paramScales(select(outputs[n])));

	    std::vector<f64> scales(paramScales(select(outputs[n])));
	    std::stringstream ss;
	    for (u32 i = 0; i < scales.size(); i++)
		ss << scales.at(i) << " ";
	    log("scales used for linear combination of output ") << outputs[n] << " : " << ss.str();

	}

    }
}

void LinearCombinationLatticeProcessorNode::insert(
    const std::string &key, const std::vector<f64> &data)
{
    std::vector<Fsa::Weight> &scales = scales_[key];
    for (u32 i = 0; i < data.size(); ++ i) {
	scales.push_back(Fsa::Weight(data[i]));
    }
}

bool LinearCombinationLatticeProcessorNode::checkNParts(u32 nParts) const
{
    bool success = true;
    Core::StringHashMap<std::vector<Fsa::Weight> >::const_iterator it = scales_.begin();
    for (; it != scales_.end(); ++ it) {
	if (it->second.size() != nParts) {
	    success = false;
	    error("mismatch in number of lattice parts and number of scales for output \"") << it->first << "\"";
	}
    }
    return success;
}

void LinearCombinationLatticeProcessorNode::processWordLattice(
    Lattice::ConstWordLatticeRef lattice, Bliss::SpeechSegment *s)
{
    if (!checkNParts(lattice->nParts())) {
	return;
    }
    timeval start, end;
    TIMER_START(start)
    Lattice::ConstWordLatticeRef result = Lattice::linearCombination(lattice, scales_);
    TIMER_STOP(start, end, timeProcessSegment_)
    Precursor::processWordLattice(result, s);
}


/**
 * CacheNode
 */
CacheNode::CacheNode(const Core::Configuration& c):
    Core::Component(c),
    Precursor(c)
{}

CacheNode::~CacheNode()
{}

void CacheNode::processWordLattice(
    Lattice::ConstWordLatticeRef lattice, Bliss::SpeechSegment *segment)
{
    if (lattice) {
	Precursor::processWordLattice(Lattice::cache(lattice), segment);
    } else {
	error("skip segment because lattice is empty");
    }
}

/**
 * CopyNode
 */
CopyNode::CopyNode(const Core::Configuration& c):
    Core::Component(c),
    Precursor(c)
{}

CopyNode::~CopyNode()
{}

void CopyNode::processWordLattice(
    Lattice::ConstWordLatticeRef lattice, Bliss::SpeechSegment *segment)
{
    if (lattice) {
	Precursor::processWordLattice(Lattice::staticCopy(lattice), segment);
    } else {
	error("skip segment because lattice is empty");
    }
}


/**
 * PartialNode
 */
const Core::ParameterInt PartialNode::paramInitial(
    "initial",
    "initial state id of partial lattice",
    Fsa::InvalidStateId);

PartialNode::PartialNode(const Core::Configuration& c):
    Core::Component(c),
    Precursor(c),
    initial_(paramInitial(config))
{}

PartialNode::~PartialNode()
{}

void PartialNode::processWordLattice(
    Lattice::ConstWordLatticeRef lattice, Bliss::SpeechSegment *segment)
{
    if (lattice) {
	Precursor::processWordLattice(Lattice::partial(lattice, initial_), segment);
    } else {
	error("skip segment because lattice is empty");
    }
}

/**
 * SkipEmptyLatticeNode
 */
SkipEmptyLatticeNode::SkipEmptyLatticeNode(const Core::Configuration& c):
    Core::Component(c),
    Precursor(c)
{}

SkipEmptyLatticeNode::~SkipEmptyLatticeNode()
{}

void SkipEmptyLatticeNode::processWordLattice(
    Lattice::ConstWordLatticeRef lattice, Bliss::SpeechSegment *segment)
{
    if (lattice and !Lattice::isEmpty(lattice)) {
	Precursor::processWordLattice(lattice, segment);
    } else {
	error("skip segment because lattice is empty");
    }
}
