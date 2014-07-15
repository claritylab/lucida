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
#include <Flow/DataAdaptor.hh>
#include "FeatureScorer.hh"
#include "MixtureSetTrainer.hh"

using namespace Speech;


const Core::ParameterString FeatureScorer::paramWeightPortName(
    "weight-port-name", "port name of flow network providing the score weights");

const Core::ParameterInt FeatureScorer::paramPrecision(
    "output-precision", "precision of the output channel", 20);

FeatureScorer::FeatureScorer(const Core::Configuration &c) :
    Component(c),
    Precursor(c, Am::AcousticModel::complete),
    weightScores_(false),
    weightPort_(Flow::IllegalPortId),
    corpusKey_(new Bliss::CorpusKey(select("corpus-key"))),
    outputChannel_(c, "output", Core::Channel::disabled)
{}

FeatureScorer::~FeatureScorer()
{}

void FeatureScorer::signOn(CorpusVisitor &corpusVisitor)
{
    corpusVisitor.signOn(corpusKey_);
    Precursor::signOn(corpusVisitor);
}

void FeatureScorer::setDataSource(Core::Ref<DataSource> dataSource)
{
    if (!paramWeightPortName(config).empty()) {

	require(dataSource);
	weightSource_ = dataSource;

	weightPort_ = weightSource_->getOutput(paramWeightPortName(config));
	if (weightPort_ == Flow::IllegalPortId) {
	    criticalError("Flow network does not have an output named \"%s\"",
				  paramWeightPortName(config).c_str());
	}
	weightScores_ = true;
    }
}

void FeatureScorer::enterSpeechSegment(Bliss::SpeechSegment *segment)
{
    Precursor::enterSpeechSegment(segment);
    segmentAccumulator_.reset();
}

void FeatureScorer::leaveSpeechSegment(Bliss::SpeechSegment *segment)
{
    std::string key;
    corpusKey_->resolve(key);

    corpusKeyToScoreMap_[key].accumulate(segmentAccumulator_);
    log("Average weighted score for key '%s': for this segment=%f; for all segments %f",
	key.c_str(), segmentAccumulator_.weightedAverage(), corpusKeyToScoreMap_[key].weightedAverage());

    Precursor::leaveSpeechSegment(segment);
}

void FeatureScorer::processAlignedFeature(Core::Ref<const Feature> f, Am::AllophoneStateIndex e)
{
    Mm::FeatureScorer::Scorer scorer = acousticModel()->featureScorer()->getScorer(f);
    segmentAccumulator_.accumulate(
	scorer->score(acousticModel()->emissionIndex(e)), featureScoreWeight(f->timestamp()));
}

FeatureScorer::Weight FeatureScorer::featureScoreWeight(const Flow::Timestamp &featureTimestamp)
{
    if (weightScores_) {
	Flow::DataPtr<Flow::DataAdaptor<Weight> > weight;
	if (weightSource_->getData(weightPort_, weight)) {
	    if (featureTimestamp.contains(*weight)) {
		if ((*weight)() >= 0)
		    return (*weight)();
		else
		    criticalError("Weight (%f) smaller then zero.", (*weight)());
	    }

	    criticalError("Timestamp of feature (%f-%f) does not contain the one of score-weight (%f-%f)",
			  featureTimestamp.startTime(), featureTimestamp.endTime(),
			  weight->startTime(), weight->endTime());
	}
	criticalError("Score-weight stream is shorter than the feature weight");
    }
    return 1;
}

void FeatureScorer::setFeatureDescription(const Mm::FeatureDescription &description)
{
    if (!acousticModel()->isCompatible(description))
	acousticModel()->respondToDelayedErrors();
    Precursor::setFeatureDescription(description);
}

void FeatureScorer::setNumberOfLabels(size_t nLabels)
{
    if (nLabels != acousticModel()->nEmissions()) {
	criticalError("Number of labels of alignment (%zd) do not match the number of mixtures (%d).",
		      nLabels, acousticModel()->nEmissions());
    }
}

void FeatureScorer::write()
{
    if (outputChannel_.isOpen()) {
	outputChannel_.precision(paramPrecision(config));

	outputChannel_ << Core::XmlOpen("corpus-key-to-score-accumulator-map");
	CorpusKeyToScoreMap::const_iterator i;
	for(i = corpusKeyToScoreMap_.begin(); i != corpusKeyToScoreMap_.end(); ++ i) {
	    outputChannel_ << Core::XmlOpen("score-accumulator") +
		Core::XmlAttribute("corpus-key", i->first);
	    outputChannel_ << Core::XmlOpen("weighted-sum-of-scores");
	    outputChannel_ << setiosflags(std::ios::scientific) << i->second.weightedSum();
	    outputChannel_ << Core::XmlClose("weighted-sum-of-scores");
	    outputChannel_ << Core::XmlOpen("sum-of-weights");
	    outputChannel_ << setiosflags(std::ios::scientific) << i->second.sumOfWeights();
	    outputChannel_ << Core::XmlClose("sum-of-weights");
	    outputChannel_ << Core::XmlClose("score-accumulator");
	}
	outputChannel_ << Core::XmlClose("corpus-key-to-score-accumulator-map");
    } else {
	criticalError("Could not dump corpus-key-to-score-map since channel \"output\" is not open.");
    }
}
