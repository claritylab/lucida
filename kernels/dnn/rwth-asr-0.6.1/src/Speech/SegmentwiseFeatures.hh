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
#ifndef _SPEECH_SEGMENTWISE_FEATURES_HH
#define _SPEECH_SEGMENTWISE_FEATURES_HH

#include "Feature.hh"

namespace Speech
{
    class SegmentwiseFeatures :
	public std::vector<Core::Ref<Feature> >,
	public Core::ReferenceCounted
    {
    public:
	void feed(Core::Ref<Feature> f) { push_back(f); }
    };
    typedef Core::Ref<SegmentwiseFeatures> SegmentwiseFeaturesRef;
    typedef Core::Ref<const SegmentwiseFeatures> ConstSegmentwiseFeaturesRef;

} // namespace Speech

namespace Core {

    /*! @todo the string representation of Speech::ConstSegmentwiseFeaturesRef should be speech-... */
    template <>
    class NameHelper<Speech::ConstSegmentwiseFeaturesRef> : public std::string {
    public:
	NameHelper() : std::string("flow-const-segmentwise-features-ref") {}
    };

} // namespace Core


#endif // _SPEECH_SEGMENTWISE_FEATURES_HH
