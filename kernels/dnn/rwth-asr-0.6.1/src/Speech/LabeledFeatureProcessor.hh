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
#ifndef _SPEECH_LABELED_FEATURE_PROCESSOR_HH
#define _SPEECH_LABELED_FEATURE_PROCESSOR_HH

#include "DataExtractor.hh"

namespace Speech {

    /** LabeledFeatureProcessor is base class for algorithms processing labeled feature vectors.
     */
    class LabeledFeatureProcessor : virtual public Core::Component {
    public:
	typedef u32 LabelIndex;
    public:
	LabeledFeatureProcessor(const Core::Configuration &c) : Component(c) {}
	virtual ~LabeledFeatureProcessor() {}

	/** Override this function to sign on to services of the corpus visitor.
	 *  Note: call the signOn function of your predecessor.
	 */
	virtual void signOn(CorpusVisitor &corpusVisitor) {}

	/** Override this function to perform preparations before first aligned features arrive.
	 */
	virtual void enterSegment(Bliss::Segment*) {}
	/** Override this function to perform post processing after the last aligned features of the segment.
	 */
	virtual void leaveSegment(Bliss::Segment*) {}
	/** Override this function to perform preparations before first aligned features arrive.
	 */
	virtual void enterSpeechSegment(Bliss::SpeechSegment*) {}
	/** Override this function to perform post processing after the last aligned features of the segment.
	 */
	virtual void leaveSpeechSegment(Bliss::SpeechSegment*) {}

	/** Override this function to implement the processing of aligned feature vectors.
	 */
	virtual void processLabeledFeature(Core::Ref<const Feature>, LabelIndex) {}

	/** Override this function to obtain the data source of corpus processor which
	 *  produces the labels.
	 */
	virtual void setDataSource(Core::Ref<DataSource>) {}
	/** Override this function to achieve the attributes of the feature streams.
	 *  This function is called once before the first feature vector is processed.
	 */
	virtual void setFeatureDescription(const Mm::FeatureDescription &) {}
	/** Override this function to achieve the labels. */
	virtual void setLabels(const std::vector<std::string>&) {}
    };

} // namespace Speech

#endif // _SPEECH_LABELED_FEATURE_PROCESSOR_HH
