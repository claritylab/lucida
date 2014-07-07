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
#ifndef _MC_TYPES_HH
#define _MC_TYPES_HH

#include <Core/Types.hh>
#include <Core/Hash.hh>

namespace Mc {

    typedef f32 Score;
    typedef Core::StringHashMap<Score> ScaleMap;

    typedef u32 EditDistance;
    typedef f32 Scale;
    typedef f64 Probability;
    typedef f64 Feature;
    typedef u32 Count;
    typedef std::vector<Feature> FeatureVector;
    typedef std::pair<std::string, Scale> NamedScale;

} // namespace Mc

#endif // _MC_TYPES_HH
