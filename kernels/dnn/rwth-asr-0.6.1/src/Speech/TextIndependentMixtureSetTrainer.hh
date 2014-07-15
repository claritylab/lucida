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
#ifndef _SPEECH_TEXT_INDEPENDENT_MIXTURE_SET_TRAINER_HH
#define _SPEECH_TEXT_INDEPENDENT_MIXTURE_SET_TRAINER_HH

#include "LabeledFeatureProcessor.hh"
#include "MixtureSetTrainer.hh"

namespace Speech {

    /** LabelingFeatureExtractor
     */
    class TextIndependentMixtureSetTrainer :
	public LabeledFeatureProcessor,
	public MlMixtureSetTrainer
    {
    private:
	LabelIndex nLabels_;
	Mm::FeatureDescription featureDescription_;
	bool initialized_;
    protected:
	virtual void setFeatureDescription(const Mm::FeatureDescription &description);
	virtual void setLabels(const std::vector<std::string> &labels) { nLabels_ = labels.size(); }

	virtual void processLabeledFeature(Core::Ref<const Feature> feature, LabelIndex labelIndex) {
	    accumulate(feature->mainStream(), labelIndex);
	}
    public:
	TextIndependentMixtureSetTrainer(const Core::Configuration &c);
	~TextIndependentMixtureSetTrainer();
    };

} // namespace Speech

#endif // _SPEECH_TEXT_INDEPENDENT_MIXTURE_SET_TRAINER_HH
