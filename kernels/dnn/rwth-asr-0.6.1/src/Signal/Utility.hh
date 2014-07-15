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
#ifndef _SIGNAL_UTILITY_HH
#define _SIGNAL_UTILITY_HH

#include <math.h>

namespace Signal {


    inline double frequency2MelFrequency(double f) {
	return (2595.0 * log10(1.0 + f / 700.0));
    }

    inline double melFrequency2Frequency(double m) {
	return (700.0 * (pow(10.0, (m / 2595.0)) - 1.0));
    }

    inline double sinc(double x) {
	return (x > (double)-1e-10 && x < (double)1e-10) ? (1 - x * x / (double)6.0) : (sin(x) / x);
    }


} // namespace Signal

#endif // _SIGNAL_UTILITY_HH
