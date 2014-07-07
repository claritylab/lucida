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
#ifndef _MM_MIXTURE_HH
#define _MM_MIXTURE_HH

#include "MixtureSetTopology.hh"
#include "MixtureSet.hh"


namespace Mm {

    class MixtureSet;

    /**
     * Could be derived from Density...
     */
    class Mixture : public MixtureTopology {
	typedef MixtureTopology Precursor;
    private:
	/** sum of weights is expected to be 1.
	 */
	std::vector<Weight> logWeights_;
    protected:
	DensityIndex densityIndexWithMaxWeight() const;
#if 1
	virtual void removeDensity(DensityIndex);
#endif
    public:
	Mixture() {}
	Mixture(const MixtureTopology &mixtureTopology);
	Mixture(const Mixture &mixture);
	Mixture(const MixtureSet &mixtureSet);
	virtual ~Mixture();

	Mixture& operator=(const Mixture& mixture);
	void addLogDensity(DensityIndex index, Weight logWeight = Core::Type<Weight>::min);
	void addDensity(DensityIndex index, Weight weight = 0);

	void normalizeWeights();

	Weight logWeight(size_t densityInMixture) const { return logWeights_[densityInMixture]; }
	Weight weight(size_t densityInMixture) const { return exp(logWeights_[densityInMixture]); }
	const std::vector<Weight>& logWeights() const { return logWeights_; }
	virtual void clear();
	virtual bool write(std::ostream& o) const;
	virtual bool read(std::istream& i, f32 version);

	void removeDensitiesWithLowWeight(Weight minWeight, bool normalizeWeights = true);
	void map(const std::vector<DensityIndex> &);
    };
    inline std::ostream& operator<< (std::ostream& o, const Mixture& ms) {
	ms.write(o);
	return o;
    };
    //     inline std::istream& operator>> (std::istream& i, Mixture& ms) {
    // 	ms.read(i);
    // 	return i;
    //     };

} // namespace Mm

#endif // _MM_MIXTURE_HH
