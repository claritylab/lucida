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
#ifndef _SPEECH_FEATURE_SCORER_HH
#define _SPEECH_FEATURE_SCORER_HH

#include <Core/Hash.hh>
#include "AcousticModelTrainer.hh"

namespace Speech {

    /** FeatureScorer
     */
    class FeatureScorer : public AcousticModelTrainer {
	typedef AcousticModelTrainer Precursor;
    public:
	static const Core::ParameterString paramWeightPortName;
	static const Core::ParameterInt paramPrecision;
    public:
	typedef Mm::Score Score;
	typedef f32 Weight;

	class Accumulator {
	    f64 weightedSum_;
	    f64 sumOfWeights_;
	public:
	    Accumulator() { reset(); }

	    void accumulate(Score value, Weight weight) {
		weightedSum_ += value * weight; sumOfWeights_ += weight;
	    }
	    void accumulate(const Accumulator &a) {
		weightedSum_ += a.weightedSum_; sumOfWeights_ += a.sumOfWeights_;
	    }

	    void reset() { weightedSum_ = 0; sumOfWeights_ = 0; }

	    f64 weightedSum() const { return weightedSum_; }
	    f64 sumOfWeights() const { return sumOfWeights_; }
	    f64 weightedAverage() const { return weightedSum_ / sumOfWeights_; }
	};
    private:
	bool weightScores_;
	Core::Ref<DataSource> weightSource_;
	Flow::PortId weightPort_;

	Core::Ref<Bliss::CorpusKey> corpusKey_;

	typedef Core::StringHashMap<Accumulator> CorpusKeyToScoreMap;
	CorpusKeyToScoreMap corpusKeyToScoreMap_;

	Accumulator segmentAccumulator_;

	Core::XmlChannel outputChannel_;
    private:
	Weight featureScoreWeight(const Flow::Timestamp &featureTimestamp);
    public:

	FeatureScorer(const Core::Configuration &configuration);
	virtual ~FeatureScorer();

	virtual void signOn(CorpusVisitor &corpusVisitor);

	virtual void enterSpeechSegment(Bliss::SpeechSegment*);
	virtual void leaveSpeechSegment(Bliss::SpeechSegment*);
	virtual void processAlignedFeature(Core::Ref<const Feature>, Am::AllophoneStateIndex);

	virtual void setDataSource(Core::Ref<DataSource>);
	virtual void setFeatureDescription(const Mm::FeatureDescription &description);
	virtual void setNumberOfLabels(size_t nLabels);

	const CorpusKeyToScoreMap& scores() const { return corpusKeyToScoreMap_; }
	void reset() { corpusKeyToScoreMap_.clear(); }

	void write();
    };

} // namespace Speech

#endif // _SPEECH_FEATURE_SCORER_HH
