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
#ifndef _MC_UTILITIES_HH
#define _MC_UTILITIES_HH

#include <Core/Utility.hh>

namespace Mc {

    /** x + y*factor */
    template<class T>
    struct plusWeight : public std::binary_function<T, T, T> {
	const T weight;
	plusWeight<T>(const T &_weight) : weight(_weight) {}
	T operator()(T x, T y) const { return x + y * weight; }
    };

    /** min(x,y) */
    template<class T>
    struct minimum : public std::binary_function<T, T, T> {
	T operator()(T x, T y) const { return std::min(x, y); }
    };

    /** max(x,y) */
    template<class T>
    struct maximum : public std::binary_function<T, T, T> {
	T operator()(T x, T y) const { return std::max(x, y); }
    };
}

#endif // _MC_UTILITIES_HH
