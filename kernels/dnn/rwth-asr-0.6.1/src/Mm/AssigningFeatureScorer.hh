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
#ifndef _MM_ASSIGNING_FEATURESCORER_HH
#define _MM_ASSIGNING_FEATURESCORER_HH

#include "FeatureScorer.hh"
#include "MixtureFeatureScorerElement.hh"

namespace Mm {

    /** Abstract feature scorer interface with assignment to the best density. */
    class AssigningFeatureScorer : public FeatureScorer {
	typedef FeatureScorer Precursor;
    public:
	/** Container for the best score and its density in a mixture */
	struct ScoreAndBestDensity {
	    Score score;
	    DensityInMixture bestDensity;
	};

    protected:
	/** Implement emission independent precalculations for feature vector */
	class AssigningContextScorer : public ContextScorer {
	protected:
	    AssigningContextScorer() {}
	public:
	    virtual ~AssigningContextScorer() {}

	    virtual EmissionIndex nEmissions() const = 0;
	    virtual Score score(EmissionIndex e) const = 0;
	    virtual DensityInMixture bestDensity(EmissionIndex e) const = 0;
	    virtual Score score(EmissionIndex e, DensityIndex dnsInMix) const = 0;
	    virtual void getDensityPosteriorProbabilities(EmissionIndex e, std::vector<Mm::Weight> &result) const = 0;
	};
	friend class ContextScorer;
    public:
	AssigningFeatureScorer(const Core::Configuration &c) : Core::Component(c), Precursor(c) {}
	virtual ~AssigningFeatureScorer() {}

	virtual void getFeatureDescription(FeatureDescription &description) const {
	    description.mainStream().setValue(FeatureDescription::nameDimension, dimension());
	}

	virtual Scorer getScorer(Core::Ref<const Feature> f) const {
	    return getAssigningScorer(f);
	}
	virtual Scorer getScorer(const FeatureVector &featureVector) const {
	    return getAssigningScorer(featureVector);
	}
	typedef Core::Ref<const AssigningContextScorer> AssigningScorer;
	virtual AssigningScorer getAssigningScorer(Core::Ref<const Feature> feature) const {
	    return getAssigningScorer(*feature->mainStream());
	}
	virtual AssigningScorer getAssigningScorer(const FeatureVector &) const = 0;
	virtual ComponentIndex dimension() const = 0;
	virtual DensityIndex nDensities() const { criticalError("nDensities() not available"); return 0; }
	virtual const MixtureFeatureScorerElement& mixture(MixtureIndex) const { criticalError("mixture() not available"); static MixtureFeatureScorerElement tmp; return tmp; }
	virtual const std::vector<DensityIndex>& densitiesInMixture(MixtureIndex) const { criticalError("densitiesInMixture() not available"); static std::vector<DensityIndex> tmp; return tmp; }
    };


    /** Abstract feature scorer interface with cached scores and assignment. */
    class CachedAssigningFeatureScorer : public AssigningFeatureScorer {
	typedef AssigningFeatureScorer Precursor;
    protected:
	/** Implement emission independent precalculations for feature vector */
	class CachedAssigningContextScorer : public AssigningContextScorer {
	protected:
	    const CachedAssigningFeatureScorer *featureScorer_;
	    mutable Cache<ScoreAndBestDensity> cache_;
	protected:
	    CachedAssigningContextScorer(const CachedAssigningFeatureScorer *featureScorer,
					 EmissionIndex nEmissions) :
		featureScorer_(featureScorer),
		cache_(nEmissions)
	    {}
	public:
	    virtual ~CachedAssigningContextScorer() {}
	    EmissionIndex nEmissions() const { return cache_.size(); }
	    virtual Score score(EmissionIndex e) const {
		require_(0 <= e && e < nEmissions());
		if (!cache_.isCalculated(e))
		    return cache_.set(e, featureScorer_->calculateScoreAndDensity(this, e)).score;
		return cache_[e].score;
	    }
	    virtual DensityInMixture bestDensity(EmissionIndex e) const {
		require_(0 <= e && e < nEmissions());
		if (!cache_.isCalculated(e))
		    return cache_.set(e, featureScorer_->calculateScoreAndDensity(this, e)).bestDensity;
		return cache_[e].bestDensity;
	    }
	    virtual Score score(EmissionIndex e, DensityIndex dnsInMix) const {
		require_(0 <= e && e < nEmissions());
		return featureScorer_->calculateScore(this, e, dnsInMix);
	    }
	    virtual void getDensityPosteriorProbabilities(EmissionIndex e, std::vector<Mm::Weight> &result) const {
		featureScorer_->calculateDensityPosteriorProbabilities(this, score(e), e, result);
	    }
	};
	virtual ScoreAndBestDensity calculateScoreAndDensity(const CachedAssigningContextScorer*, MixtureIndex) const = 0;
	virtual Score calculateScore(const CachedAssigningContextScorer*, MixtureIndex, DensityIndex) const {
	    criticalError("This feature scorer does not support the calculation of scores given a density in the mixture");
	    return Core::Type<Score>::max;
	}
	virtual void calculateDensityPosteriorProbabilities(const CachedAssigningContextScorer*, Score logDenominator,
							    EmissionIndex e, std::vector<Mm::Weight> &result) const {
	    criticalError("This feature scorer does not support the calculation of density posterior probabilities");
	}
    public:
	CachedAssigningFeatureScorer(const Core::Configuration &c) : Core::Component(c), Precursor(c) {}
	virtual ~CachedAssigningFeatureScorer() {}
    };


    class DensitySpecificCachedAssigningFeatureScorer: public CachedAssigningFeatureScorer
    {
	typedef CachedAssigningFeatureScorer Precursor;
    protected:
	class ContextScorer: public CachedAssigningContextScorer
	{
	protected:
	    const DensitySpecificCachedAssigningFeatureScorer *featureScorer_;
	    mutable Cache2D<Score> scoreCache_;
	protected:
	    ContextScorer(const DensitySpecificCachedAssigningFeatureScorer *featureScorer, EmissionIndex nEmissions) :
		CachedAssigningContextScorer(featureScorer, nEmissions), featureScorer_(featureScorer), scoreCache_(nEmissions)
	    {
	    }
	public:
	    virtual ~ContextScorer()
	    {
	    }
	    virtual Score score(EmissionIndex e, DensityIndex dnsInMix) const
	    {
		require_(0 <= e && e < nEmissions());
		if (!scoreCache_.isInitialized(e)) {
		    const MixtureFeatureScorerElement& mixture = featureScorer_->mixture(e);
		    scoreCache_.initEmission(e, mixture.nDensities());
		}
		if (!scoreCache_.isCalculated(e, dnsInMix)) {
		    return scoreCache_.set(e, dnsInMix, featureScorer_->calculateScore(this, e, dnsInMix));
		}
		return scoreCache_(e, dnsInMix);
	    }
	};
	virtual Score calculateScore(const CachedAssigningContextScorer*, MixtureIndex, DensityIndex) const = 0;
    public:
	DensitySpecificCachedAssigningFeatureScorer(const Core::Configuration &c) :
	    Core::Component(c), Precursor(c)
	{
	}
	virtual ~DensitySpecificCachedAssigningFeatureScorer()
	{
	}
    };

} // namespace Mm

#endif // _MM_ASSIGNING_FEATURESCORER_HH
