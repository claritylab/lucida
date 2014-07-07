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
#ifndef _MM_FEATURESCORER_HH
#define _MM_FEATURESCORER_HH

#include "Types.hh"
#include "Feature.hh"
#include "Utilities.hh"
#include <vector>
#include <Core/Component.hh>
#include <Core/Dependency.hh>

namespace Mm {

    /** Abstract feature scorer interface. */
    class FeatureScorer :
	public virtual Core::Component,
	public Core::ReferenceCounted
    {
    protected:
	/** Implement emission independent precalculations for feature vector */
	class ContextScorer : public Core::ReferenceCounted {
	protected:
	    ContextScorer() {}
	public:
	    virtual ~ContextScorer() {}

	    virtual EmissionIndex nEmissions() const = 0;
	    virtual Score score(EmissionIndex e) const = 0;
	    virtual Score score(EmissionIndex e, u32 modelIndex){ return 0.0;}
	};
	friend class ContextScorer;
    public:
	FeatureScorer(const Core::Configuration &c) : Core::Component(c) {}
	virtual ~FeatureScorer() {}

	virtual EmissionIndex nMixtures() const = 0;
	virtual void getFeatureDescription(FeatureDescription &description) const = 0;
	virtual void getDependencies(Core::DependencySet &dependencies) const {
	    FeatureDescription description(*this);
	    getFeatureDescription(description);
	    description.getDependencies(dependencies);
	}

	typedef Core::Ref<const ContextScorer> Scorer;
	virtual Scorer getScorer(Core::Ref<const Feature>) const = 0;
	virtual Scorer getScorer(const FeatureVector &) const = 0;

	/**
	 * reset should be overloaded/defined in/for
	 * featurescorer related to sign language recognition
	 * especially the tracking part
	 *
	 */
	virtual void reset() const {};

	/**
	 * setSegmentName should be overloaded/defined in classes
	 * using embedded flow networks to create unambiguous ids
	 * for cache nodes
	 */
	virtual void setSegmentName(const std::string name) const {};

	/**
	 * finalize should be overloaded/defined in classes using
	 * embedded flow networks to sent final end of sequence token
	 * if necessary
	 */
	virtual void finalize() const {};

	/**
	 * Return true if the feature scorer buffers features.
	 */
	virtual bool isBuffered() const { return false; }

	/**
	 * Add a feature to the feature buffer.
	 * Implementation required if isBuffered() == true
	 */
	virtual void addFeature(const FeatureVector &f) const {}
	virtual void addFeature(Core::Ref<const Feature> f) const {}

	/**
	 * Return a scorer for the current feature without adding a
	 * new feature to the buffer.
	 * Should be called until bufferEmpty() == true.
	 * Requires bufferEmpty() == false.
	 * Implementation required if isBuffered() == true
	 */
	virtual Scorer flush() const { return Scorer(); }

	/**
	 * Return true if the feature buffer is full.
	 * Implementation required if isBuffered() == true
	 */
	virtual bool bufferFilled() const { return true; }

	/**
	 * Return true if the feature buffer is empty.
	 * Implementation required if isBuffered() == true
	 */
	virtual bool bufferEmpty() const { return true; }

	/**
	 * Return the number of buffered features required to
	 * execute getScorer().
	 * Implementation required if isBuffered() == true
	 */
	virtual u32 bufferSize() const { return 0; }

	/**
	 *
	 */
    private:
	/*
	 * This class is only required because of the feature scorer interface
	 * We need to have a default return value for getTimeIndexedScorer,
	 * although an error is called before.
	 */
	class DummyContextScorer : public ContextScorer {
	public:
	    DummyContextScorer() {}
	    virtual ~DummyContextScorer() {}

	    virtual EmissionIndex nEmissions() const { return 0; }
	    virtual Score score(EmissionIndex e) const { return 0; }
	};
	friend class DummyContextScorer;
    public:
	virtual bool hasTimeIndexedCache() const { return false; }
	virtual Scorer getTimeIndexedScorer(u32 time) const {
	    error("time-indexed scores not available") ;
	    return Scorer(new DummyContextScorer());}
    };

    /** Abstract feature scorer interface with cached scores. */
    class CachedFeatureScorer : public FeatureScorer {
	typedef FeatureScorer Precursor;

    protected:
	/** Implement emission independent precalculations for feature vector */
	class CachedContextScorer : public ContextScorer {
	private:
	    const CachedFeatureScorer *featureScorer_;
	    mutable Cache<Score> cache_;
	protected:
	    CachedContextScorer(const CachedFeatureScorer *featureScorer, EmissionIndex nEmissions) :
		featureScorer_(featureScorer), cache_(nEmissions) {}
	public:
	    virtual ~CachedContextScorer() {}
	    EmissionIndex nEmissions() const { return cache_.size(); }
	    virtual Score score(EmissionIndex e) const {
		require_(0 <= e && e < nEmissions());
		if (!cache_.isCalculated(e))
		    return cache_.set(e, featureScorer_->calculateScore(this, e));
		return cache_[e];
	    }
	};

    public:
	// A cache-overlay which takes a context-scorer as input and adds a cache layer
	/// @todo Eventually replace the cached class above by this one
	class CachedContextScorerOverlay : public FeatureScorer::ContextScorer {
	private:
	    FeatureScorer::Scorer scorer_;
	    mutable Cache<Score> cache_;
	    mutable bool precached_;
	public:
	    CachedContextScorerOverlay(u32 nEmissions) :
		cache_(nEmissions), precached_(false) {}
	    virtual ~CachedContextScorerOverlay() {}
	    virtual EmissionIndex nEmissions() const { return cache_.size(); }
	    virtual Score score(EmissionIndex e) const {
		require_(0 <= e && e < nEmissions());
		if (!cache_.isCalculated(e))
		    return cache_.set(e, scorer_->score(e));
		return cache_[e];
	    }
	    void precache() const {
		if (precached_)
		    return;
		precached_ = true;
		u32 n = nEmissions();
		for (u32 e = 0; e < n; ++e)
		    score(e);
	    }
	    bool precached() const {
		return precached_;
	    }
	    void setScorer(FeatureScorer::Scorer scorer) {
		scorer_ = scorer;
	    }
	    FeatureScorer::Scorer scorer() const {
		return scorer_;
	    }
	};

	virtual Score calculateScore(const CachedContextScorer *, MixtureIndex) const = 0;
    public:
	CachedFeatureScorer(const Core::Configuration &c) : Core::Component(c), Precursor(c) {}
	virtual ~CachedFeatureScorer() {}
    };

    typedef Core::Ref<CachedFeatureScorer::CachedContextScorerOverlay> CachedContextScorer;

    // A thread-safe scorer cache
    class ContextScorerCache : public Core::ReferenceCounted
    {
	struct ScorerHash {
	    size_t operator()(const Core::Ref<const Feature>& feature) const {
		return ((size_t)feature.get()) *  2654435761u;
	    }
	};
    public:
	ContextScorerCache(u64 maxSize) : maxSize_(maxSize), epoch_(0) {
	}

	// Every returned cached scorer must be released by calling uncacheScorer as soon as it is not needed any more
	CachedContextScorer cacheScorer(Core::Ref<const Feature> feature, u32 nEmissions) {
	    Core::MutexLock lock(&mutex_);
	    Core::HashMap<Core::Ref<const Feature>, std::pair<CachedContextScorer, u64>, ScorerHash>::iterator it = map_.find(feature);
	    if( it != map_.end() )
	    {
		CachedContextScorer ret = it->second.first;
		epochMap_.erase(it->second.second);
		map_.erase(it);
		return ret;
	    }
	    return CachedContextScorer(new CachedFeatureScorer::CachedContextScorerOverlay(nEmissions));
	}

	// After this was called, only the returned scorer must be used, instead of the given cached scorer.
	FeatureScorer::Scorer uncacheScorer(Core::Ref<const Feature> feature, CachedContextScorer cachedScorer) {
	    FeatureScorer::Scorer ret = cachedScorer->scorer();
	    const_cast<CachedFeatureScorer::CachedContextScorerOverlay&>(*cachedScorer).setScorer(FeatureScorer::Scorer());
	    Core::MutexLock lock(&mutex_);
	    Core::HashMap<Core::Ref<const Feature>, std::pair<CachedContextScorer, u64>, ScorerHash>::iterator it = map_.find(feature);
	    if( it != map_.end() )
		return ret;
	    map_.insert( std::make_pair(feature, std::make_pair(cachedScorer, epoch_) ) );
	    epochMap_.insert( std::make_pair(epoch_, feature) );
	    epoch_ += 1;
	    if( map_.size() > maxSize_ )
	    {
		map_.erase(epochMap_.begin()->second);
		epochMap_.erase(epochMap_.begin());
	    }
	    return ret;
	}

	void clear() {
	    Core::MutexLock lock(&mutex_);
	    map_.clear();
	    epoch_ = 0;
	}
    private:
	Core::Mutex mutex_;
	u64 maxSize_, epoch_;
	Core::HashMap<Core::Ref<const Feature>, std::pair<CachedContextScorer, u64>, ScorerHash> map_;
	std::map<u64, Core::Ref<const Feature> > epochMap_;
    };
} // namespace Mm

#endif // _MM_FEATURESCORER_HH
