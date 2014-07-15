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
#ifndef _SPEECH_ALIGNED_FEATURE_PROCESSOR_HH
#define _SPEECH_ALIGNED_FEATURE_PROCESSOR_HH

#include <Am/AcousticModel.hh>
#include "DataExtractor.hh"

namespace Speech {

    /**
     *  AlignedFeatureProcessor is base class for algorithms processing aligned feature vectors.
     *
     *  The AlignedFeatureProcessor is usually driven by Speech::AlignedFeatureExtractor.
     *
     *  Remark:
     *    an instance of this class can be used for dry runs since it does not have any abstract functions.
     */
    class AlignedFeatureProcessor : virtual public Core::Component {
    public:
	AlignedFeatureProcessor(const Core::Configuration &c) : Component(c) {}
	virtual ~AlignedFeatureProcessor() {}

	/** Override this function to sign on to services of the corpus visitor.
	 *  Note: call the signOn function of your predecessor.
	 */
	virtual void signOn(CorpusVisitor &corpusVisitor) {}

	/** Override this function to perform preparations before first segment is processed.
	 */
	virtual void enterCorpus(Bliss::Corpus*) {}
	/** Override this function to perform post processing after the last segment is processed.
	 */
	virtual void leaveCorpus(Bliss::Corpus*) {}
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

	/** Override this function to implement the processing of Viterbi-aligned feature vectors.
	 */
	virtual void processAlignedFeature(Core::Ref<const Feature> f, Am::AllophoneStateIndex e) {}

	/** Override this function to implement the processing of Baum-Welch-aligned feature vectors.
	 */
	virtual void processAlignedFeature(Core::Ref<const Feature>, Am::AllophoneStateIndex, Mm::Weight) {}

	/** Override this function to implement the processing of feature vectors with two weights.
	 */
	virtual void processAlignedFeature(Core::Ref<const Feature>, Am::AllophoneStateIndex, Mm::Weight, Mm::Weight) {}

	/** Override this function to obtain the data source of corpus processor which
	 *  produces the labels.
	 */
	virtual void setDataSource(Core::Ref<DataSource>) {}

	/** Override this function to achieve the attributes of the feature streams.
	 *  This function is called once before the first feature vector is processed.
	 */
	virtual void setFeatureDescription(const Mm::FeatureDescription &) {}
    };


} // namespace Speech

#endif // _SPEECH_ALIGNED_FEATURE_PROCESSOR_HH
