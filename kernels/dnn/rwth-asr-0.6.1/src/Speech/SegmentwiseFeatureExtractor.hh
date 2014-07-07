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
#ifndef _SPEECH_SEGMENTWISE_FEATURE_EXTRACTOR_HH
#define _SPEECH_SEGMENTWISE_FEATURE_EXTRACTOR_HH

#include "DataExtractor.hh"
#include "SegmentwiseFeatures.hh"
#include <Am/AcousticModel.hh>


namespace Speech
{

    /**
     * SegmentwiseFeatureExtractor is a corpus visitor application for segmentwise feature extraction.
     */
    class SegmentwiseFeatureExtractor :
	public DataExtractor, public Core::ReferenceCounted
    {
	typedef DataExtractor Precursor;
    protected:
	typedef Core::hash_map<Flow::PortId, SegmentwiseFeaturesRef> FeatureStreams;
    private:
	FeatureStreams featureStreams_;
	static const Core::ParameterBool paramNoDependencyCheck;
	const bool noDependencyCheck_;
    public:
	SegmentwiseFeatureExtractor(const Core::Configuration &c);
	virtual ~SegmentwiseFeatureExtractor() {}

	virtual void signOn(CorpusVisitor &corpusVisitor);

	Flow::PortId addPort(const std::string &);
	void checkCompatibility(Flow::PortId, Core::Ref<const Am::AcousticModel>) const;
	ConstSegmentwiseFeaturesRef features(Flow::PortId) const;
	bool valid(Flow::PortId port) const { return (featureStreams_.find(port) != featureStreams_.end()) && !featureStreams_.find(port)->second->empty(); }
	bool valid() const;
	virtual void processSegment(Bliss::Segment *);
    };
}

#endif // _SPEECH_SEGMENTWISE_FEATURE_EXTRACTOR_HH
