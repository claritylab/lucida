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
#ifndef _SPEECH_TIMEFRAME_LABEL_HH
#define _SPEECH_TIMEFRAME_LABEL_HH

#include <Core/Types.hh>

namespace Speech {

    typedef u32 TimeframeIndex;

    class TimeframeLabel {
    public:
	TimeframeIndex time_;
	TimeframeLabel() {}
	TimeframeLabel(TimeframeIndex time) : time_(time) {}
	bool isDefault() const { return false; }
	void writeXml(Core::XmlWriter &os) const {
	    os << Core::XmlFull("time", time_);
	}
    };

} // namespace Speech

#endif //_SPEECH_TIMEFRAME_LABEL_HH
