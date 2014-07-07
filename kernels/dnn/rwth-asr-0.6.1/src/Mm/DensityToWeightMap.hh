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
#ifndef _MM_DENSITY_TO_WEIGHT_MAP_HH
#define _MM_DENSITY_TO_WEIGHT_MAP_HH

#include <Core/Hash.hh>
#include <Mm/Types.hh>

namespace Mm {

    class DensityToWeightMap : public Core::hash_map<DensityIndex, Weight>
    {
	private:
	typedef Core::hash_map<DensityIndex, Weight> Precursor;
    public:
	DensityToWeightMap operator*(Weight factor) const {
	    DensityToWeightMap result = *this;
	    for (iterator it = result.begin(); it != result.end(); ++ it) it->second *= factor;
	    return result;
	}
	DensityToWeightMap& operator*=(Weight factor) {
	    for (iterator it = this->begin(); it != this->end(); ++ it) {
		it->second *= factor;
	    }
	    return *this;
	    }
    };
}

#endif // _MM_POSTERIOR_AND_DENSITIES_HH
