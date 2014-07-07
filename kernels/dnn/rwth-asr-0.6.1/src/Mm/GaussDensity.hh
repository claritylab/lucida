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
#ifndef _MM_GAUSS_DENSITY_HH
#define _MM_GAUSS_DENSITY_HH

#include "MixtureSetTopology.hh"
#include "Utilities.hh"
#include <Core/Application.hh>

namespace Mm {

    /** AbstractDensity
     */
    class AbstractDensity {
    };

    /** GaussDensity
     */
    class GaussDensity :
	public AbstractDensity, public GaussDensityTopology {
	typedef GaussDensityTopology Precursor;
    public:
	GaussDensity(MeanIndex meanIndex = Core::Type<MeanIndex>::max,
		     CovarianceIndex covarianceIndex = Core::Type<CovarianceIndex>::max) :
	    Precursor(meanIndex, covarianceIndex) {}
	void map(const std::vector<MeanIndex>& meanMap, const std::vector<CovarianceIndex> &covarianceMap);
    };

    /** LogLinearDensity
     */
    class LogLinearDensity : public AbstractDensity {
    private:
	Scales scales_;
    public:
	LogLinearDensity(ComponentIndex dimension = 0) : scales_(dimension) {}
	LogLinearDensity(const Scales &v) : scales_(v) {}
	const Scales& scales() const { return scales_; }
	Scales& scales() { return scales_; }
	Weight pseudoLogPrior(const std::vector<VarianceType> &variances) const {
	    //      require(variances.size() == (scales_.size() - 1));
	    Weight result = scales_.back();
	    for (ComponentIndex cmp = 0; cmp < variances.size(); ++ cmp) {
		result += 0.5 * (scales_[cmp] * scales_[cmp] * variances[cmp] + log(2 * M_PI * variances[cmp]));
	    }
	    return result;
	}
    };

    /** Mean
     */
    class Mean : public std::vector<MeanType> {
	typedef std::vector<MeanType> Precursor;
    public:
	Mean(ComponentIndex dimension = 0, MeanType value = 0) : Precursor(dimension, value) {}
	Mean(const Mean &v) : Precursor(v) {}
	Mean(const Mean &v, const Count &obs) : Precursor(v) {}
	Mean(const Precursor &v) : Precursor(v) {}
	virtual ~Mean() {};
	virtual bool write(std::ostream& o) const;
	virtual bool read(std::istream& i);
    };
    inline std::ostream& operator<< (std::ostream& o, const Mean& m) {
		m.write(o);
		return o;
	};
    inline std::istream& operator>> (std::istream& i, Mean& m) {
		m.read(i);
		return i;
	};


    /** Interface class for covariance implementations
     */
    class Covariance {
    public:
	virtual ~Covariance() {}

	/**
	 *  Returns diagonal of covariance.
	 *  Remark: output type is const reference for the moment, might need to get changed.
	 */
	virtual const std::vector<VarianceType> &diagonal() const = 0;
	/**
	 *  Returns weights of the features.
	 */
	virtual const std::vector<f64> &weights() const = 0;
	/**
	 *  set weights of the features.
	 */
	virtual void setFeatureWeights(const std::vector<f64> &w) = 0;
	/**
	 *  1 / 2 { N * log(2 * pi) + log(determinant) }.
	 *  Remark: default implementation uses only the diagonal elements.
	 */
	virtual Score halfeLogNormFactor() const {
	    return 0.5 * gaussLogNormFactor(diagonal().begin(), diagonal().end());
	}

	virtual Covariance* clone() const = 0;
	virtual void setOffset(ComponentIndex offset) { verify(0); /* not implemented in this covariance type */ }
	virtual void setDimension(ComponentIndex size) = 0;
	virtual ComponentIndex dimension() const = 0;
	virtual bool isPositiveDefinite() const = 0;
	/**
	 * print/scan methods for std::iostreams
	 * used in operator<< resp. operator>>
	 * overwrite these methodes in inherited classes
	 */
	virtual bool write(std::ostream& o) const {
		Core::Application::us()->error("write() not yet implented for (inherited) Covariance");
		return false;
	}
	virtual bool read(std::istream& i) {
		Core::Application::us()->error("read() not yet implented for (inherited) Covariance");
		return false;
	}
    };
	inline std::ostream& operator<< (std::ostream& o, const Covariance& c) {
		if (!c.write(o))
			Core::Application::us()->error("an error occures while printing Covariance on stream..");
		return o;
	};
	inline std::istream& operator>> (std::istream& i, Covariance& c) {
		if (!c.read(i))
			Core::Application::us()->error("an error occures while scaning Covariance from stream..");
		return i;
	};


    /** DiagonalCovariance
     */
    class DiagonalCovariance : public Covariance {
	typedef Covariance Precursor;
	friend class CovarianceEstimator;
    protected:
	std::vector<VarianceType> buffer_;
	std::vector<f64> featureWeights_;
    public:
	DiagonalCovariance(ComponentIndex dimension = 0) { setDimension(dimension); }
	DiagonalCovariance(const std::vector<VarianceType> &v, const std::vector<f64> &w) : buffer_(v), featureWeights_(w) {}
	DiagonalCovariance(const std::vector<VarianceType> &v) : buffer_(v) {featureWeights_.resize(v.size(), 1.0);}
	virtual ~DiagonalCovariance() {}

	Covariance* clone() const { return new DiagonalCovariance(*this); }

	DiagonalCovariance& operator=(std::vector<VarianceType> &v) { buffer_ = v; featureWeights_.resize(v.size(), 1.0); return *this; }
	virtual void setFeatureWeights(const std::vector<f64> &w) { featureWeights_ = w; }
	virtual void setOffset(ComponentIndex offset) {
	    buffer_.erase(buffer_.begin(), buffer_.begin()+offset);
	    featureWeights_.erase(featureWeights_.begin(), featureWeights_.begin()+offset);
	}
	virtual void setDimension(ComponentIndex size) {
	    buffer_.resize(size, 1.0);
	    featureWeights_.resize(size, 1.0);
	}
	virtual ComponentIndex dimension() const { return buffer_.size(); }

	virtual const std::vector<VarianceType>& diagonal() const { return buffer_; }
	virtual const std::vector<f64>& weights() const { return featureWeights_; }
	virtual bool isPositiveDefinite() const {
	    return std::find_if(diagonal().begin(), diagonal().end(),
				std::bind2nd(std::less_equal<VarianceType>(), 0)) == diagonal().end();
	}
	virtual bool write(std::ostream& o) const;
	virtual bool read(std::istream& i);
    };

} // namespace Mm

#endif //_MM_GAUSS_DENSITY_HH
