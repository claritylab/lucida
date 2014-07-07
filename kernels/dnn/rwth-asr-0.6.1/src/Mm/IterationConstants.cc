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
#include "IterationConstants.hh"
#include "MixtureSet.hh"

using namespace Mm;

namespace InternalIc {

    /**
     * AugmentedDensityEstimator
     */
    struct AugmentedDensityEstimator
    {
	Core::Ref<EbwDiscriminativeGaussDensityEstimator> densityEstimator;
	Core::Ref<AbstractMixtureEstimator> mixtureEstimator;
	Weight previousMixtureWeight;
    };

    /**
     * DensitySet
     */
    struct AugmentedDensityEstimatorHash :
	public std::unary_function<size_t, AugmentedDensityEstimator>
    {
	size_t operator() (const AugmentedDensityEstimator &d) const {
	    return reinterpret_cast<size_t>(d.densityEstimator.get());
	}
    };
    struct AugmentedDensityEstimatorEquality :
	public std::unary_function<size_t, AugmentedDensityEstimator>
    {
	bool operator() (const AugmentedDensityEstimator &lhs,
			 const AugmentedDensityEstimator &rhs) const {
	    return lhs.densityEstimator.get() == rhs.densityEstimator.get();
	}
    };
    typedef Core::hash_set<AugmentedDensityEstimator,
			   AugmentedDensityEstimatorHash,
			   AugmentedDensityEstimatorEquality> DensitySet;

    /**
     * ReferenceValueMap: base class
     */
    template <class Object, class Value>
    class ReferenceValueMap {
    public:
	typedef Core::Ref<Object> ObjectRef;
	typedef Core::hash_map<ObjectRef, Value, hashReference<Object>, std::equal_to<ObjectRef> > Map;
    protected:
	Map map_;
    public:
	size_t size() const { return map_.size(); }

	void insert(ObjectRef o, Value v) { map_[o] = v; }
	typename Map::const_iterator find(ObjectRef o) const { return map_.find(o); }
	Value operator[](ObjectRef o) const {
	    typename Map::const_iterator result = map_.find(o);
	    ensure(result != map_.end());
	    return result->second;
	}
	Value& operator[](ObjectRef o) { return map_[o]; }

	typename Map::const_iterator begin() const { return map_.begin(); }
	typename Map::const_iterator end() const { return map_.end(); }
    };

    /**
     * CovarianceToDensitySetMap
     * map: AbstractCovarianceEstimator -> AugmentedDensityEstimator
     */
    class CovarianceToDensitySetMap :
	public ReferenceValueMap<AbstractCovarianceEstimator, DensitySet>
    {
	typedef AbstractMixtureSetEstimator::MixtureEstimators MixtureEstimators;
    public:
	typedef Map::const_iterator const_iterator;
    public:
	CovarianceToDensitySetMap(const MixtureEstimators &);

	const_iterator begin() const { return map_.begin(); }
	const_iterator end() const { return map_.end(); }
    };

    CovarianceToDensitySetMap::CovarianceToDensitySetMap(
	const AbstractMixtureSetEstimator::MixtureEstimators &mixtureEstimators)
    {
	MixtureEstimators::const_iterator m = mixtureEstimators.begin();
	for(; m != mixtureEstimators.end(); ++ m) {
	    Core::Ref<EbwDiscriminativeMixtureEstimator> mixtureEstimator(
		required_cast(EbwDiscriminativeMixtureEstimator*, m->get()));
	    for (DensityIndex dnsInMix = 0; dnsInMix < mixtureEstimator->nDensities(); ++ dnsInMix) {
		AugmentedDensityEstimator augmented;
		augmented.densityEstimator = Core::Ref<EbwDiscriminativeGaussDensityEstimator>(
		    required_cast(EbwDiscriminativeGaussDensityEstimator*,
				  mixtureEstimator->densityEstimators()[dnsInMix].get()));
		augmented.mixtureEstimator = mixtureEstimator;
		augmented.previousMixtureWeight = mixtureEstimator->previousMixtureWeight(dnsInMix);
		DensitySet &set = map_[augmented.densityEstimator->covariance()];
		/**
		 * Not all tying schemes are supported yet.
		 * More precisely, a density cannot be shared
		 * by more than one mixture.
		 */
		require(set.find(augmented) == set.end());
		set.insert(augmented);
	    }
	}
    }

    /*
     * Xi
     *
     *         / beta/[1+(Gamma_max-1)·Gamma_max/gamma_max] : Gamma_max < gamma_max
     * xi_s = <
     *         \ beta                                       : otherwise
     */
    class Xi : public Core::Component
    {
	typedef AbstractMixtureSetEstimator::MixtureEstimators MixtureEstimators;
	typedef AbstractMixtureEstimator::DensityEstimators DensityEstimators;
	typedef ReferenceValueMap<AbstractMixtureEstimator, IterationConstant> Map;
    private:
	static const Core::ParameterFloat paramBeta;
    private:
	Map map_;
	IterationConstant beta_;
    private:
	IterationConstant denominator(Core::Ref<AbstractMixtureEstimator>);
    public:
	Xi(const Core::Configuration &, const MixtureEstimators &);

	IterationConstant operator[](Core::Ref<AbstractMixtureEstimator> mixture) const {
	    return map_[mixture];
	}
    };

    const Core::ParameterFloat Xi::paramBeta(
	"beta",
	"This parameter is part of the heuristic formula that controls convergence \
	 in discriminative training.",
	1, 0);

    Xi::Xi(
	const Core::Configuration &c,
	const MixtureEstimators &mixtureEstimators) :
	Core::Component(c),
	beta_(paramBeta(config))
    {
	for (MixtureIndex mix = 0; mix < mixtureEstimators.size(); ++ mix) {
	    IterationConstant xi = beta_ / denominator(mixtureEstimators[mix]);
	    map_.insert(mixtureEstimators[mix], xi);
	    log("xi[") << mix << "]: " << xi;
	}
    }

    IterationConstant Xi::denominator(
	Core::Ref<AbstractMixtureEstimator> mixtureEstimator)
    {
	verify(mixtureEstimator);
	IterationConstant maxAbsDtWeight = 0;
	IterationConstant maxNumDenWeight = 0;
	DensityEstimators::const_iterator d = mixtureEstimator->densityEstimators().begin();
	for (; d != mixtureEstimator->densityEstimators().end(); ++ d) {
	    Core::Ref<EbwDiscriminativeMeanEstimator> meanEstimator(
		required_cast(EbwDiscriminativeMeanEstimator*, (*d)->mean().get()));
	    const IterationConstant absDtWeight = Core::abs(meanEstimator->dtWeight());
	    if (maxAbsDtWeight < absDtWeight) {
		maxAbsDtWeight = absDtWeight;
		maxNumDenWeight = std::max(meanEstimator->dtWeight() + meanEstimator->denWeight(), meanEstimator->denWeight());
	    }
	}
	if (maxAbsDtWeight < maxNumDenWeight and maxNumDenWeight > 0) {
	    return 1 + (maxAbsDtWeight - 1) * maxAbsDtWeight / maxNumDenWeight;
	} else {
	    return 1;
	}
    }

    /**
     * iteration constants b
     *
     * t1 := -Gamma_{s,l}(x_d^2) + sigma_min·Gamma_{s,l}(1)
     * t2 := 2·Gamma_{s,l}(x_d) - Gamma_{s,l}(1)·mu_{s,l,d}
     * t3 := Gamma_{s,l}(x_d) - Gamma_{s,l}(1)·mu_{s,l,d}
     *
     *                   sum_{s*,l*} [ t1 + t2·mu_{s,l,d} + xi_s·t3^2 ]
     * D_k^min = max_d { ---------------------------------------------- }
     *                   sum_{s*,l*} [ c_{s,l}·(sigma*^2-sigma_min) ]
     *
     *     with sigma*^2 in {sigma_{s,l,d}^2, sigma_{s_d}^2, sigma_d^2}
     */
    class Icb : public Core::Component
    {
	typedef Core::Component Precursor;
    protected:
	typedef AbstractMixtureSetEstimator::MixtureEstimators MixtureEstimators;
    private:
	static const Core::ParameterFloat paramSigmaMinimum;
	static const Core::ParameterFloat paramSigmaMinimumFactor;
    protected:
	const IterationConstant default_;
	IterationConstant sigmaMinimum_;
	IterationConstant stepSize_;
	Xi xi_;
	ComponentIndex dimension_;
    protected:
	VarianceType minimumVariance(const CovarianceToDensitySetMap &) const;
	IterationConstant sigmaMinimum() const { return sigmaMinimum_; }
	void setSigmaMinimum(const CovarianceToDensitySetMap &);
	IterationConstant minimumDk(CovarianceToDensitySetMap::const_iterator, const CovarianceToMeanSetMap::MeanSet &);
    public:
	Icb(const Core::Configuration &, const MixtureEstimators &,
	    const CovarianceToDensitySetMap &, IterationConstant stepSize);
    };

    const Core::ParameterFloat Icb::paramSigmaMinimum(
	"sigma-minimum",
	"This parameter provides a lower limit for the variances in \
	 discriminative training and therefore it depends on the magnitude of \
	 the acoustic features. Thus, a scaling of the features may cause \
	 lower values for this parameter.",
	1, 0);

    const Core::ParameterFloat Icb::paramSigmaMinimumFactor(
	"sigma-minimum-factor",
	"factor to determine sigma-minimum dynamically, \
	 which makes sigma-minimum independent of the magnitude \
	 of the acoustic features (see above)",
	1e-5, 0);

    Icb::Icb(const Core::Configuration &c,
	     const MixtureEstimators &mixtureEstimators,
	     const CovarianceToDensitySetMap &cim,
	     IterationConstant stepSize) :
	Precursor(c),
	default_(1),
	stepSize_(stepSize),
	xi_(select("xi"), mixtureEstimators)
    {
	setSigmaMinimum(cim);
    }

    VarianceType Icb::minimumVariance(const CovarianceToDensitySetMap &cim) const
    {
	Core::XmlWriter &os(clog());
	VarianceType result = Core::Type<VarianceType>::max;
	CovarianceToDensitySetMap::const_iterator it = cim.begin();
	for (; it != cim.end(); ++ it) {
	    Core::Ref<const EbwDiscriminativeCovarianceEstimator> covarianceEstimator(
		required_cast(const EbwDiscriminativeCovarianceEstimator*, it->first.get()));
	    const std::vector<VarianceType> &prevCov = covarianceEstimator->previousCovariance();

	    os << Core::XmlOpen("previous-covariance");
	    std::copy(prevCov.begin(), prevCov.end(), std::ostream_iterator<VarianceType>(os, " "));
	    os << Core::XmlClose("previous-covariance");

	    result = std::min(result, *std::min_element(prevCov.begin(), prevCov.end()));
	}
	return result;
    }

    void Icb::setSigmaMinimum(const CovarianceToDensitySetMap &cim)
    {
	const VarianceType tmp = minimumVariance(cim);
	sigmaMinimum_ = paramSigmaMinimum(config, paramSigmaMinimumFactor(config) * tmp);
	log("sigma-minimum: ") << sigmaMinimum_ << " (" << tmp << ")";
    }

    /**
     * calculate D_k_min
     */
    IterationConstant Icb::minimumDk(
	CovarianceToDensitySetMap::const_iterator covarianceAndDensitySet,
	const CovarianceToMeanSetMap::MeanSet &meanSet)
    {
	Core::Ref<EbwDiscriminativeCovarianceEstimator> covarianceEstimator(
	    required_cast(EbwDiscriminativeCovarianceEstimator*, covarianceAndDensitySet->first.get()));
	const std::vector<Sum> dtSum2 = covarianceEstimator->dtSum(meanSet);
	const std::vector<VarianceType> &previousCovariance = covarianceEstimator->previousCovariance();
	ComponentIndex dimension = previousCovariance.size();
	std::vector<IterationConstant> numerator(dimension, 0);
	std::transform(numerator.begin(), numerator.end(), dtSum2.begin(),
		       numerator.begin(), std::minus<IterationConstant>());
	std::vector<IterationConstant> denominator(dimension, 0);
	const DensitySet &densitySet = covarianceAndDensitySet->second;
	DensitySet::const_iterator d = densitySet.begin();
	for (; d != densitySet.end(); ++ d) {
	    Core::Ref<EbwDiscriminativeMeanEstimator> meanEstimator(
		required_cast(EbwDiscriminativeMeanEstimator*, d->densityEstimator->mean().get()));
	    const std::vector<Sum> dtSum = meanEstimator->dtSum();
	    Weight dtWeight = meanEstimator->dtWeight();
	    const std::vector<MeanType> &previousMean = meanEstimator->previousMean();
	    for (ComponentIndex i = 0; i < dimension; ++ i) {
		const IterationConstant t2 = 2 * dtSum[i] - dtWeight * previousMean[i];
		const IterationConstant t3 = dtSum[i] - dtWeight * previousMean[i];
		numerator[i] += sigmaMinimum_ * dtWeight;
		numerator[i] += t2 * previousMean[i];
		numerator[i] += xi_[d->mixtureEstimator] * t3 * t3;
		denominator[i] += (previousCovariance[i] - sigmaMinimum_) * d->previousMixtureWeight;
	    }
	}
	std::vector<IterationConstant> ratio(dimension);
	std::transform(numerator.begin(), numerator.end(), denominator.begin(),
		       ratio.begin(), std::divides<IterationConstant>());
	IterationConstant ratioMax = *std::max_element(ratio.begin(), ratio.end());
	log("D-k-min-raw: ") << Core::form("%.2f", ratioMax);
	IterationConstant D_k_min = ratioMax > 0 ? ratioMax : 0;
	log("D-k-min: ") << Core::form("%.2f", D_k_min);
	return D_k_min;
    }

    /**
     * global iteration constants b
     */
    class GlobalIcb : public Icb
    {
	typedef Icb Precursor;
	typedef ReferenceValueMap<AbstractCovarianceEstimator, IterationConstant> Map;
    private:
	Map map_;
    public:
	GlobalIcb(const Core::Configuration &, const MixtureEstimators &,
		  const CovarianceToDensitySetMap &, IterationConstant stepSize,
		  const CovarianceToMeanSetMap &);

	IterationConstant b(Core::Ref<AbstractCovarianceEstimator> covariance) const {
	    return map_[covariance];
	}
    };

    GlobalIcb::GlobalIcb(const Core::Configuration &c,
			 const MixtureEstimators &mixtureEstimators,
			 const CovarianceToDensitySetMap &cim,
			 IterationConstant stepSize,
			 const CovarianceToMeanSetMap &meanSetMap) :
	Precursor(c, mixtureEstimators, cim, stepSize)
    {
	CovarianceToDensitySetMap::const_iterator it = cim.begin();
	for (; it != cim.end(); ++ it) {
	    const CovarianceToMeanSetMap::MeanSet &meanSet =
		meanSetMap[Core::Ref<AbstractCovarianceEstimator>(it->first)];
	    const IterationConstant D_k_min = minimumDk(it, meanSet);
	    IterationConstant ic = default_;

	    /**
	     * D_k = h · max_{s*,l*} { 1/c_{s,l} · (1/xi_s - Gamma_{s,l}(1)), D_k^min }
	     */
	    const DensitySet &densitySet = it->second;
	    DensitySet::const_iterator d = densitySet.begin();
	    for (; d != densitySet.end(); ++ d) {
		Core::Ref<EbwDiscriminativeMeanEstimator> meanEstimator(
		    required_cast(EbwDiscriminativeMeanEstimator*, d->densityEstimator->mean().get()));
		IterationConstant D_k = 1 / xi_[d->mixtureEstimator];
		D_k -= meanEstimator->dtWeight();
		if (d->previousMixtureWeight > 0) {
		    D_k /= d->previousMixtureWeight;
		}
		ic = std::max(ic, std::max(D_k, D_k_min));
	    }
	    map_.insert(it->first, ic * stepSize_);
	    log("icb: ") << b(it->first);
	}
    }

    /**
     * local iteration constants b
     */
    class LocalIcb : public Icb
    {
	typedef Icb Precursor;
	typedef ReferenceValueMap<AbstractMixtureEstimator, IterationConstant> Map;
    private:
	Map map_;
    public:
	LocalIcb(const Core::Configuration &, const MixtureEstimators &,
		 const CovarianceToDensitySetMap &, IterationConstant stepSize,
		 const CovarianceToMeanSetMap &);

	IterationConstant b(Core::Ref<AbstractMixtureEstimator> mixture) const {
	    return map_[mixture];
	}
    };

    LocalIcb::LocalIcb(const Core::Configuration &c,
		       const MixtureEstimators &mixtureEstimators,
		       const CovarianceToDensitySetMap &cim,
		       IterationConstant stepSize,
		       const CovarianceToMeanSetMap &meanSetMap) :
	Precursor(c, mixtureEstimators, cim, stepSize)
    {
	CovarianceToDensitySetMap::const_iterator it = cim.begin();
	for (; it != cim.end(); ++ it) {
	    const CovarianceToMeanSetMap::MeanSet &meanSet =
		meanSetMap[Core::Ref<AbstractCovarianceEstimator>(it->first)];
	    const IterationConstant D_k_min = minimumDk(it, meanSet);

	    /**
	     * D_k = h · max_{s*,l*} { 1/c_{s,l} · (1/xi_s - Gamma_{s,l}(1)), D_k^min }
	     */
	    const DensitySet &densitySet = it->second;
	    DensitySet::const_iterator d = densitySet.begin();
	    for (; d != densitySet.end(); ++ d) {
		if (map_.find(d->mixtureEstimator) == map_.end()) {
		    map_.insert(d->mixtureEstimator, stepSize_ * std::max(default_, D_k_min));
		}
		Core::Ref<EbwDiscriminativeMeanEstimator> meanEstimator(
		    required_cast(EbwDiscriminativeMeanEstimator*, d->densityEstimator->mean().get()));
		IterationConstant D_k = 1 / xi_[d->mixtureEstimator];
		D_k -= meanEstimator->dtWeight();
		if (d->previousMixtureWeight > 0) {
		    D_k /= d->previousMixtureWeight;
		}
		map_[d->mixtureEstimator] = std::max(stepSize_ * D_k, map_[d->mixtureEstimator]);
	    }
	}
    }

} //namespace InternalIc

/**
 * IterationConstants: global RWTH variant
 */
class GlobalRwthIterationConstants : public IterationConstants
{
    typedef IterationConstants Precursor;
private:
    bool usePooledVariances_;
    InternalIc::GlobalIcb *ic_;
protected:
    virtual void initialize(const MixtureEstimators &, const InternalIc::CovarianceToDensitySetMap &,
			    const CovarianceToMeanSetMap &);
    virtual void set(const InternalIc::AugmentedDensityEstimator &);
public:
    GlobalRwthIterationConstants(const Core::Configuration &);
    virtual ~GlobalRwthIterationConstants() { delete ic_; }
};

GlobalRwthIterationConstants::GlobalRwthIterationConstants(const Core::Configuration &c) :
    Precursor(c),
    usePooledVariances_(true),
    ic_(0)
{}

void GlobalRwthIterationConstants::initialize(
    const MixtureEstimators &mixtureEstimators,
    const InternalIc::CovarianceToDensitySetMap &cim,
    const CovarianceToMeanSetMap &meanSetMap)
{
    verify(!ic_);
    ic_ = new InternalIc::GlobalIcb(
	select("icb"), mixtureEstimators, cim, stepSize_, meanSetMap);
}

void GlobalRwthIterationConstants::set(const InternalIc::AugmentedDensityEstimator &d)
{
    IterationConstant ic = ic_->b(d.densityEstimator->covariance());
    if (usePooledVariances_) {
	ic *= d.previousMixtureWeight;
    }
    ic = std::max(default_, ic);
    d.densityEstimator->setIterationConstant(ic);
}

/**
 * IterationConstants: local RWTH variant
 */
class LocalRwthIterationConstants : public IterationConstants
{
    typedef IterationConstants Precursor;
private:
    bool usePooledVariances_;
    InternalIc::LocalIcb *ic_;
protected:
    virtual void initialize(const MixtureEstimators &, const InternalIc::CovarianceToDensitySetMap &,
			    const CovarianceToMeanSetMap &);
    virtual void set(const InternalIc::AugmentedDensityEstimator &);
public:
    LocalRwthIterationConstants(const Core::Configuration &);
    virtual ~LocalRwthIterationConstants() { delete ic_; }
};

LocalRwthIterationConstants::LocalRwthIterationConstants(const Core::Configuration &c) :
    Precursor(c),
    usePooledVariances_(true),
    ic_(0)
{}

void LocalRwthIterationConstants::initialize(
    const MixtureEstimators &mixtureEstimators,
    const InternalIc::CovarianceToDensitySetMap &cim,
    const CovarianceToMeanSetMap &meanSetMap)
{
    verify(!ic_);
    ic_ = new InternalIc::LocalIcb(
	select("icb"), mixtureEstimators, cim, stepSize_, meanSetMap);
}

void LocalRwthIterationConstants::set(const InternalIc::AugmentedDensityEstimator &d)
{
    IterationConstant ic = ic_->b(d.mixtureEstimator);
    if (usePooledVariances_) {
	ic *= d.previousMixtureWeight;
    }
    ic = std::max(default_, ic);
    d.densityEstimator->setIterationConstant(ic);
}

/**
 * IterationConstants: Cambridge variant
 */
class CambridgeIterationConstants : public IterationConstants
{
    typedef IterationConstants Precursor;
private:
    static const Core::ParameterFloat paramMinimumObservationWeight;
    static const Core::ParameterFloat paramE;
private:
    Weight minimumObservationWeight_;
    Weight e_;
    InternalIc::GlobalIcb *ic_;
private:
    Sum maximumRoot(Core::Ref<EbwDiscriminativeMeanEstimator>,
		    Core::Ref<EbwDiscriminativeCovarianceEstimator>) const;
protected:
    virtual void initialize(const MixtureEstimators &, const InternalIc::CovarianceToDensitySetMap &,
			    const CovarianceToMeanSetMap &);
    virtual void set(const InternalIc::AugmentedDensityEstimator &);
public:
    CambridgeIterationConstants(const Core::Configuration &);
    virtual ~CambridgeIterationConstants() { delete ic_; }
};

const Core::ParameterFloat CambridgeIterationConstants::paramMinimumObservationWeight(
    "minimum-observation-weight",
    "If the observation weight is lower than this parameter, the corresponding \
     iteration constant is set to default value.",
    0, 0);

const Core::ParameterFloat CambridgeIterationConstants::paramE(
    "e",
    "This parameter is a global constant that is multiplied by the denominator \
     occupancy Gamma_sl(1) in order to determine the iteration constant \
     in discriminative training. Use this parameter only if objective function \
     oscillates.",
    2, 0);

CambridgeIterationConstants::CambridgeIterationConstants(const Core::Configuration &c) :
    Precursor(c),
    minimumObservationWeight_(paramMinimumObservationWeight(config)),
    e_(paramE(config)),
    ic_(0)
{}

void CambridgeIterationConstants::initialize(
    const MixtureEstimators &mixtureEstimators,
    const InternalIc::CovarianceToDensitySetMap &cim,
    const CovarianceToMeanSetMap &meanSetMap)
{
    verify(!ic_);
    ic_ = new InternalIc::GlobalIcb(
	select("icb"), mixtureEstimators, cim, stepSize_, meanSetMap);
}

Sum CambridgeIterationConstants::maximumRoot(
    Core::Ref<EbwDiscriminativeMeanEstimator> meanEstimator,
    Core::Ref<EbwDiscriminativeCovarianceEstimator> covarianceEstimator) const
{
    verify(meanEstimator and covarianceEstimator);
#if 1
    require(false);
    const std::vector<Sum> Gamma_x2_sl;
    const std::vector<Sum> Gamma_x_sl;
    Weight Gamma_1_sl = 0;
#endif
    const std::vector<MeanType> &mu_sl = meanEstimator->previousMean();
    const std::vector<VarianceType> &sigma_sl = covarianceEstimator->previousCovariance();

    const ComponentIndex dimension = mu_sl.size();
    verify(Gamma_x2_sl.size() == dimension && Gamma_x_sl.size() == dimension);
    verify(mu_sl.size() == dimension);
    Sum rootMax = Core::Type<Sum>::min;
    for (ComponentIndex i = 0; i < dimension; ++ i) {
	const Sum a = sigma_sl[i];
	const Sum b = Gamma_x2_sl[i] + Gamma_1_sl * (sigma_sl[i] + mu_sl[i] * mu_sl[i]) - 2.0 * Gamma_x_sl[i] * mu_sl[i];
	const Sum c = Gamma_1_sl * Gamma_x2_sl[i] - Gamma_x_sl[i] * Gamma_x_sl[i];

	Sum signB = b < 0 ? -1 : 1;
	const Sum root2 = -0.5 * (b + signB * sqrt(b * b - 4.0 * a * c)) / a;
	rootMax = std::max(rootMax, root2);
    }

    int expo;
    return ldexp(frexp(rootMax, &expo) + Core::Type<f32>::epsilon, expo);
}

void CambridgeIterationConstants::set(const InternalIc::AugmentedDensityEstimator &d)
{
    IterationConstant ic = default_;

    // do not take low occupied densities into account
    Core::Ref<EbwDiscriminativeMeanEstimator> meanEstimator(
	required_cast(EbwDiscriminativeMeanEstimator*, d.densityEstimator->mean().get()));
    if (meanEstimator->weight() < minimumObservationWeight_) {
	d.densityEstimator->setIterationConstant(ic);
	return;
    }

    Core::Ref<EbwDiscriminativeCovarianceEstimator> covarianceEstimator(
	required_cast(EbwDiscriminativeCovarianceEstimator*, d.densityEstimator->covariance().get()));
    const Sum rootMax = maximumRoot(meanEstimator, covarianceEstimator);
    ic = ic > rootMax ? ic : rootMax;
    if (meanEstimator->denWeight() == 0) {
	d.densityEstimator->setIterationConstant(ic);
	return;
    }

    IterationConstant eHalfmax = 0.5 * stepSize_ * rootMax / meanEstimator->denWeight();
    verify(eHalfmax >= 0);
    IterationConstant e = (e_ == 0.0) ? eHalfmax : e_;
    e *= meanEstimator->denWeight();
    ic *= stepSize_;
    ic = ic > e ? ic : e;
    d.densityEstimator->setIterationConstant(ic);
}

/**
 * IterationConstants
 */
Core::Choice IterationConstants::choiceType(
    "global-rwth", globalRwth,
    "local-rwth", localRwth,
    "cambridge", cambridge,
    Core::Choice::endMark());

Core::ParameterChoice IterationConstants::paramType(
    "type",
    &choiceType,
    "type of iteration constants",
    globalRwth);

const Core::ParameterFloat IterationConstants::paramStepSize(
    "step-size",
    "This parameter defines the step size of the gradient descent method \
     and the extended Baum algorithm, respectively, in discriminative \
     training. It is is the most important value to control \
     convergence. The lower the value the larger and faster (but perhaps \
     more unstable) the convergence. Typical values are chosen within \
     the left-sided open interval (1.0,...,5.0].",
    1.1, 1);

const Core::ParameterFloat IterationConstants::paramMinimumIcd(
    "minimum-icd",
    "minimum value for the iteration constants",
    1, 0);

IterationConstants::IterationConstants(const Core::Configuration &c) :
    Core::Component(c),
    default_(paramMinimumIcd(config)),
    stepSize_(paramStepSize(config))
{}

/**
 * We would like to have a deterministic output.
 */
void IterationConstants::dumpIterationConstants(
    const MixtureEstimators &mixtureEstimators)
{
    for (MixtureIndex mix = 0; mix < mixtureEstimators.size(); ++ mix) {
	const AbstractMixtureEstimator::DensityEstimators &densityEstimators =
	    mixtureEstimators[mix]->densityEstimators();
	for (DensityIndex dns = 0; dns < densityEstimators.size(); ++ dns) {
	    Core::Ref<EbwDiscriminativeMeanEstimator> meanEstimator(
		required_cast(EbwDiscriminativeMeanEstimator*, densityEstimators[dns]->mean().get()));
	    log("Icd[") << mix << "," << dns << "] set to "
			<< meanEstimator->iterationConstant();
	}
    }
}

void IterationConstants::set(
    const MixtureEstimators &mixtureEstimators,
    const CovarianceToMeanSetMap &meanSetMap)
{
    InternalIc::CovarianceToDensitySetMap cim(mixtureEstimators);
    initialize(mixtureEstimators, cim, meanSetMap);
    InternalIc::CovarianceToDensitySetMap::const_iterator it = cim.begin();
    for (; it != cim.end(); ++ it) {
	const InternalIc::DensitySet &densitySet = it->second;
	InternalIc::DensitySet::const_iterator d = densitySet.begin();
	for (; d != densitySet.end(); ++ d) {
	    set(*d);
	}
    }
    dumpIterationConstants(mixtureEstimators);
}

IterationConstants* IterationConstants::createIterationConstants(const Core::Configuration &c)
{
    IterationConstants *ic = 0;
    switch (paramType(c)) {
    case globalRwth:
	ic = new GlobalRwthIterationConstants(c);
	break;
    case localRwth:
	ic = new LocalRwthIterationConstants(c);
	break;
    case cambridge:
	ic = new CambridgeIterationConstants(c);
	break;
    default:
	defect();
	break;
    }
    return ic;
}
