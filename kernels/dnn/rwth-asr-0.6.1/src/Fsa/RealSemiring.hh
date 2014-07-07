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
#ifndef _FSA_REAL_SEMIRING_HH
#define _FSA_REAL_SEMIRING_HH

#include "tRealSemiring.hh"
#include "Semiring.hh"

namespace Fsa {
    typedef Ftl::LogSemiring<Weight, f32> LogSemiring_;
    typedef Ftl::TropicalSemiring<Weight, f32> TropicalSemiring_;
    typedef Ftl::ProbabilitySemiring<Weight, f32> ProbabilitySemiring_;
} // namespace Fsa

#endif // _FSA_REAL_SEMIRING_HH
