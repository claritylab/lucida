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
#include "Vector.hh"

#include <Math/Complex.hh>

namespace Flow {

template class Vector<s8>;
template class Vector<s16>;
template class Vector<s32>;
template class Vector<f32>;
template class Vector<f64>;
template class Vector<std::complex<f32> >;
template class Vector<std::complex<f64> >;
template class Vector<bool>;

}
