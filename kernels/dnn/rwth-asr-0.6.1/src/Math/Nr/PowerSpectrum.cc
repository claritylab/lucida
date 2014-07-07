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
#include "nr.h"

namespace Math { namespace Nr {

    float evlmem(float normalizedFrequency, const std::vector<float> &a, float gain)
    {
	float sumr=1.0, sumi=0.0;
	double theta = 2 * M_PI * normalizedFrequency;
	double wpr = cos(theta);
	double wpi = sin(theta);
	double wr=1.0, wi=0.0, wtemp;
	size_t m = a.size();
	for (size_t i= 0; i < m; ++ i) {
	    wr = (wtemp = wr) * wpr - wi * wpi;
	    wi = wi * wpr + wtemp * wpi;
	    sumr += a[i] * wr;
	    sumi += a[i] * wi;
	}
	return gain * gain / (sumr * sumr + sumi * sumi);
    }

} } // namespace Math::Nr
