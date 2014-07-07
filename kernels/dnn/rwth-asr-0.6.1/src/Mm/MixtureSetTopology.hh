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
#ifndef _MM_MIXTURE_SET_TOPOLOGY_HH
#define _MM_MIXTURE_SET_TOPOLOGY_HH

#include "Types.hh"
#include <Core/Hash.hh>
#include <Core/StringUtilities.hh>
#include <Core/TextStream.hh>

namespace Mm {

    /**
     *  GaussDensityTopology
     */
    class GaussDensityTopology {
    private:
	MeanIndex meanIndex_;
	CovarianceIndex covarianceIndex_;
    public:
	GaussDensityTopology(MeanIndex meanIndex, CovarianceIndex covarianceIndex) :
	    meanIndex_(meanIndex), covarianceIndex_(covarianceIndex) {}
	virtual ~GaussDensityTopology() {}

	void setMeanIndex(MeanIndex index) { meanIndex_ = index; }
	MeanIndex meanIndex() const { return meanIndex_; }

	void setCovarianceIndex(CovarianceIndex index) { covarianceIndex_ = index; }
	CovarianceIndex covarianceIndex() const { return covarianceIndex_; }

	bool operator==(const GaussDensityTopology &t) {
	    return meanIndex_ == t.meanIndex_ && covarianceIndex_ == t.covarianceIndex_;
	}
	virtual bool write(std::ostream& o) const;
	virtual bool read(std::istream& i);
    };
    inline std::ostream& operator<< (std::ostream& o, const GaussDensityTopology& gdt) {
	gdt.write(o);
	return o;
    };
    inline std::istream& operator>> (std::istream& i, GaussDensityTopology& gdt) {
	gdt.read(i);
	return i;
    };

    /**
     *  MixtureTopology
     */
    class MixtureTopology {
    protected:
	std::vector<DensityIndex> densityIndices_;
    protected:
#if 1
	virtual void removeDensity(DensityIndex index) {
	    densityIndices_.erase(densityIndices_.begin() + index);
	}
#endif
    public:
	virtual ~MixtureTopology() { clear(); }
	void addDensity(DensityIndex index) { densityIndices_.push_back(index); }

	size_t nDensities() const { return densityIndices_.size(); }
	DensityIndex densityIndex(size_t densityInMixture) const {
	    return densityIndices_[densityInMixture];
	}
	const std::vector<DensityIndex>& densityIndices() const { return densityIndices_; }
	virtual void clear() { densityIndices_.clear(); }
    };

} // namespace Mm

#endif //_MM_MIXTURE_SET_TOPOLOGY_HH
