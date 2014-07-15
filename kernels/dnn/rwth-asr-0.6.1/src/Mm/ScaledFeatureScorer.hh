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
#ifndef _MM_SCALED_FEATURE_SCORER_HH
#define _MM_SCALED_FEATURE_SCORER_HH

#include "FeatureScorer.hh"
#include "AssigningFeatureScorer.hh"
#include <Mc/Component.hh>

namespace Mm {

    /** Feature scorer with log linear scale. */
    class ScaledFeatureScorer :
	public FeatureScorer,
	public Mc::Component
    {
    public:
	ScaledFeatureScorer(const Core::Configuration &c) :
	    Core::Component(c), FeatureScorer(c), Component(c) {}
	virtual ~ScaledFeatureScorer() {}
	virtual void getDependencies(Core::DependencySet &dependencies) const {
	    FeatureScorer::getDependencies(dependencies);
	    Component::getDependencies(dependencies);
	}
    };

    /**
     *  Wraper class for Feature scorer with log linear scale.
     *  @todo: add scale to the basic feature scorer to remove the additional vtable lookup
     *  and function call for each call to score().
     */
    class FeatureScorerScaling : public ScaledFeatureScorer
    {
	public:
	/** Implement emission independent precalculations for feature vector */
	class ScaledContextScorer : public ContextScorer {
	    friend class FeatureScorerScaling;
	private:
	    Scorer scorer_;
	    Score scale_;
	protected:
	    ScaledContextScorer(Scorer scorer, Score scale) : scorer_(scorer), scale_(scale) {}
	public:
	    virtual ~ScaledContextScorer() {}

	    virtual EmissionIndex nEmissions() const { return scorer_->nEmissions(); }
	    virtual Score score(EmissionIndex e) const { return scale_ * scorer_->score(e); }

	    virtual const Score getScale() const
	    {
		return scale_;
	    }

	    virtual Scorer getUnscaledScorer() const
	    {
		return scorer_;
	    }
	};
    private:
	Core::Ref<FeatureScorer> featureScorer_;
    public:
	FeatureScorerScaling(const Core::Configuration &c, Core::Ref<FeatureScorer> featureScorer) :
	    Core::Component(c),
	    ScaledFeatureScorer(c),
	    featureScorer_(featureScorer) {}
	virtual ~FeatureScorerScaling() {}

	virtual EmissionIndex nMixtures() const { return featureScorer_->nMixtures(); }
	virtual void getFeatureDescription(FeatureDescription &description) const {
	    featureScorer_->getFeatureDescription(description);
	}

	virtual Scorer getScorer(Core::Ref<const Feature> f) const {
	    return Scorer(new ScaledContextScorer(featureScorer_->getScorer(f), scale()));
	}
	virtual Scorer getScorer(const FeatureVector &featureVector) const {
	    return Scorer(new ScaledContextScorer(featureScorer_->getScorer(featureVector), scale()));
	}
	Core::Ref<const AssigningFeatureScorer> assigningFeatureScorer() const {
	    return Core::Ref<const AssigningFeatureScorer>(dynamic_cast<const AssigningFeatureScorer*>(featureScorer_.get()));
	}

	virtual void reset() const {
	    featureScorer_->reset();
	}

	virtual void setSegmentName(const std::string name) const {
	    featureScorer_->setSegmentName(name);
	}

	virtual void finalize() const {
	    featureScorer_->finalize();
	}

	virtual bool isBuffered() const {
	    return featureScorer_->isBuffered();
	}

	virtual void addFeature(const FeatureVector &f) const {
	    return featureScorer_->addFeature(f);
	}

	virtual void addFeature(Core::Ref<const Feature> f) const {
	    return featureScorer_->addFeature(f);
	}

	virtual Scorer flush() const {
	    return Scorer(new ScaledContextScorer(featureScorer_->flush(), scale()));
	}

	virtual bool bufferFilled() const {
	    return featureScorer_->bufferFilled();
	}

	virtual bool bufferEmpty() const {
	    return featureScorer_->bufferEmpty();
	}

	virtual u32 bufferSize() const {
	    return featureScorer_->bufferSize();
	}

	virtual bool hasTimeIndexedCache() const {
	    return featureScorer_->hasTimeIndexedCache();
	}

	virtual Scorer getTimeIndexedScorer(u32 time) const {
	    return Scorer(new ScaledContextScorer(featureScorer_->getTimeIndexedScorer(time), scale()));
	}

	Core::Ref<FeatureScorer> getUnscaledFeatureScorer() {
	    return featureScorer_;
	}

    };

} // namespace Mm

#endif // _MM_SCALED_FEATURE_SCORER_HH
