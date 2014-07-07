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
#ifndef _SPEECH_TYPES_HH
#define _SPEECH_TYPES_HH

#include <Core/Types.hh>

namespace Speech {

    typedef f32 Score;
    typedef u32 TimeframeIndex;
    static TimeframeIndex InvalidTimeframeIndex = Core::Type<u32>::max;

}

#endif // _SPEECH_TYPES_HH
