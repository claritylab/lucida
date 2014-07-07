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
#ifndef _SPEECH_SCATTER_MATRICES_ESTIMATOR_HH
#define _SPEECH_SCATTER_MATRICES_ESTIMATOR_HH

#include "AcousticModelTrainer.hh"
#include "LabeledFeatureProcessor.hh"
#include <Signal/EigenTransform.hh>

namespace Speech {

    /** Text Dependent Lda Estimator */
    class TextDependentScatterMatricesEstimator :
	public AcousticModelTrainer
    {
	typedef Core::Ref<Signal::ScatterMatricesEstimator> EstimatorRef;
    protected:
	 EstimatorRef estimator_;

	virtual void setFeatureDescription(const Mm::FeatureDescription &);

	virtual void processAlignedFeature(
	    Core::Ref<const Feature> feature, Am::AllophoneStateIndex allophoneStateIndex) {
	    processAlignedFeature(feature, allophoneStateIndex, 1.0);
	}
	virtual void processAlignedFeature(
	    Core::Ref<const Feature> feature, Am::AllophoneStateIndex allophoneStateIndex, Mm::Weight w) {
	    if(w != 0)
		estimator_->accumulate(acousticModel()->emissionIndex(allophoneStateIndex), *feature->mainStream(), w);
	}
    public:
	TextDependentScatterMatricesEstimator(const Core::Configuration&);
	TextDependentScatterMatricesEstimator(const Core::Configuration&,
					      Core::Ref<Signal::ScatterMatricesEstimator>);
	virtual ~TextDependentScatterMatricesEstimator() {}

	EstimatorRef getEstimator() const { return estimator_; }
    };

    /** Text Independent Lda Estimator */
    class TextIndependentScatterMatricesEstimator :
	public LabeledFeatureProcessor
    {
	typedef Core::Ref<Signal::ScatterMatricesEstimator> EstimatorRef;
    protected:
	 EstimatorRef estimator_;

	virtual void setFeatureDescription(const Mm::FeatureDescription &);
	virtual void setLabels(const std::vector<std::string> &labels) {
	    estimator_->setNumberOfClasses(labels.size());
	}

	virtual void processLabeledFeature(
	    Core::Ref<const Feature> feature, LabelIndex labelIndex) {
	    estimator_->accumulate(labelIndex, *feature->mainStream());
	}
	virtual void processLabeledFeature(
	    Core::Ref<const Feature> feature, LabelIndex labelIndex, Mm::Weight w) {
	    if(w != 0) {
		if(w != 1)
			criticalError("Processing of weighted alignments is not supported.");
		estimator_->accumulate(labelIndex, *feature->mainStream());
	    }
	}
    public:
	TextIndependentScatterMatricesEstimator(const Core::Configuration &configuration);
	TextIndependentScatterMatricesEstimator(const Core::Configuration &configuration,
						Core::Ref<Signal::ScatterMatricesEstimator>);
	virtual ~TextIndependentScatterMatricesEstimator() {}

	EstimatorRef getEstimator() const { return estimator_; }
    };

}  // namespace Speech

#endif // _SPEECH_SCATTER_MATRICES_ESTIMATOR_HH
