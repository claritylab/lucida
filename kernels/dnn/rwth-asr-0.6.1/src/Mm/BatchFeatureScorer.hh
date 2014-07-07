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
#ifndef _MM_BATCH_FEATURE_SCORER_HH
#define _MM_BATCH_FEATURE_SCORER_HH

#include <Mm/FeatureScorer.hh>
#include <Mm/MixtureSet.hh>
#include <Mm/SimdFeatureScorer.hh>

namespace Mm {

// forward declaration for density clustering
template<class F, class D> class DensityClustering;

/**
 * Feature scorer computing scores for multiple feature vectors in advance.
 *
 * Design is a bit crude, but this code has to be very fast and design aspects
 * are therefore neglected.
 */
class BatchFeatureScorerBase : public FeatureScorer
{
protected:
    /**
     * Stores the current feature and the number of buffered features.
     * All computations are done in BatchFeatureScorer.
     * This class is used only because it is required by the
     * FeatureScorer interface.
     */
    class ContextScorer : public FeatureScorer::ContextScorer
    {
    public:
	ContextScorer(const BatchFeatureScorerBase *parent,
		      u32 currentFeature, u32 bufferedFeatures)
	    : parent_(parent),
	      currentFeature_(currentFeature),
	      bufferedFeatures_(bufferedFeatures) {}
	virtual ~ContextScorer() {}
	virtual EmissionIndex nEmissions() const { return parent_->nMixtures(); }
	virtual Score score(EmissionIndex e) const {
	    return parent_->getScore(e, currentFeature_, bufferedFeatures_);
	}
    private:
	const BatchFeatureScorerBase *parent_;
	u32 currentFeature_, bufferedFeatures_;
    };

public:
   BatchFeatureScorerBase(const Core::Configuration &c);
   virtual ~BatchFeatureScorerBase();

   /**
    * Initialize the internal datastructures.
    * mixtureSet is not required afterwards and may be deleted.
    */
   virtual void init(const MixtureSet &mixtureSet) = 0;

   virtual EmissionIndex nMixtures() const { return nMixtures_; }
   virtual void getFeatureDescription(FeatureDescription &description) const {
       description.mainStream().setValue(FeatureDescription::nameDimension, dimension_);
   }

   typedef Core::Ref<const ContextScorer> Scorer;

   /**
    * Return a scorer for the current feature and append the
    * given feature to the buffer.
    * The current feature may not be the same as f
    * because of the feature buffering.
    * Requires bufferFilled() == true.
    */
   virtual FeatureScorer::Scorer getScorer(Core::Ref<const Feature> f) const {
       return getScorer(*f->mainStream());
   }
   virtual FeatureScorer::Scorer getScorer(const FeatureVector &f) const;

   /**
    * Clear the feature buffer and the score cache.
    * Must be called before a new feature segment is entered.
    */
   virtual void reset() const;

   /**
    * Add a feature to the feature buffer.
    * Requires bufferFilled() == false.
    */
   virtual void addFeature(const FeatureVector &f) const;
   virtual void addFeature(Core::Ref<const Feature> f) const {
       addFeature(*f->mainStream());
   }

   /**
    * Return a scorer for the current feature without adding a
    * new feature to the buffer.
    * Should be called until bufferEmpty() == true.
    * Requires bufferEmpty() == false.
    */
   virtual FeatureScorer::Scorer flush() const;

   /**
    * This feature scorer is buffered.
    */
   virtual bool isBuffered() const { return true; }

   /**
    * Return true if the feature buffer is full.
    */
   virtual bool bufferFilled() const {
       return buffered_ >= bufferSize_ - 1;
   }

   /**
    * Return true if the feature buffer is empty.
    */
   virtual bool bufferEmpty() const {
       return buffered_ <= 0;
   }

   /**
    * Return the number of required buffered features
    */
   virtual u32 bufferSize() const {
       return bufferSize_;
   }

protected:
   static const Core::ParameterInt paramBufferSize;
   void invalidateCache(size_t pos) const;
   /**
    * Fill offsets_, cached_
    * allocate scores_,
    * set nMixtures_, nDensities_
    */
   void initialize(const MixtureSet &mixtureSet);
   Score getScore(EmissionIndex e, u32 featureIndex, u32 length) const;
   /**
    * Preprocess the feature vector and store it in the feature buffer
    * at the given position.
    */
   virtual void setFeature(size_t pos, const FeatureVector &f) const = 0;
   virtual void fillScoreCache(EmissionIndex e, u32 featureIndex, u32 length) const = 0;
protected:
   struct AllDensitySelector;

   // many member variables have to be mutable because the FeatureScorer
   // interface has only const member functions (reason?).

   /**
    * mapping from mixture to densities:
    * densities for mixture m: [offsets_[m] .. offsets_[m+1])
    */
   std::vector<size_t> offsets_;
   /**
    * is score cached?
    * layout: (mixture 0, pos 0), ..., (mixture_0, pos bufferSize_),
    *         (mixture 1, pos 0), ..., (mixture_1, pos bufferSize_),
    *         ....
    */
   mutable std::vector<bool> cached_;
   /**
    * cached scores for each mixture and buffer position.
    * same layout as cached_
    */
   mutable f32 *scores_;
   u32 paddedDimension_, dimension_;
   u32 nMixtures_, nDensities_;

   /**
    * position of the current feature
    */
   mutable s32 currentFeature_;
   /**
    * number of buffered features
    */
   mutable s32 buffered_;
   /**
    * total buffer size.
    */
   s32 bufferSize_;
};


/**
 * Batched feature scorer using floats.
 *
 * Requires an acoustic model with a single (globally pooled) diagonal covariance.
 * Scores are calculated using the maximum approximation (max. over densities).
 * The distance computation uses SIMD instructions (SSE).
 * Using floats without quantization.
 *
 * Data is stored in raw arrays with 16-byte alignment.
 * (required by the SSE instructions).
 *
 * Mean vectors and features are preprocessed by multiplication with the inverse
 * square root of the variance.
 */
class BatchFloatFeatureScorer : public BatchFeatureScorerBase
{
public:
    BatchFloatFeatureScorer(const Core::Configuration &c);
    BatchFloatFeatureScorer(const Core::Configuration &c, Core::Ref<const MixtureSet> mixtureSet);
    virtual ~BatchFloatFeatureScorer();

    virtual void init(const MixtureSet &mixtureSet);

protected:
    virtual void setFeature(size_t pos, const FeatureVector &f) const;
    virtual void fillScoreCache(EmissionIndex e, u32 featureIndex, u32 length) const;
    template<class DensitySelector>
    inline void fillScoreCacheTpl(EmissionIndex e, u32 featureIndex, u32 length, const DensitySelector &selector) const;

protected:
    /**
     * Number of floats that are processed in one iteration
     * of the distance computation.
     */
    static const size_t BlockSize;

    /**
     * feature buffer.
     * features are stored sequentially with paddedDimension_ floats.
     */
    mutable f32 *features_;
    /**
     * pre-processed mean vectors of all densities.
     * means are stored sequentially for each mixture.
     * mapping from a mixture to a mean using offsets_:
     */
    f32 *means_;
    /**
     * the reciprocal square root of the variance vector.
     */
    f32 *variance_;
    /**
     * density weight + global normalization factor
     * for each density.
     */
    f32 *constants_;
};


class BatchPreselectionFloatFeatureScorer : public BatchFloatFeatureScorer
{
public:
    BatchPreselectionFloatFeatureScorer(const Core::Configuration &c);
    BatchPreselectionFloatFeatureScorer(const Core::Configuration &c, Core::Ref<const MixtureSet> mixtureSet);
    virtual ~BatchPreselectionFloatFeatureScorer();
    virtual void init(const MixtureSet &mixtureSet);

protected:
    virtual void setFeature(size_t pos, const FeatureVector &f) const;
    virtual void fillScoreCache(EmissionIndex e, u32 featureIndex, u32 length) const;

private:
    typedef DensityClustering<f32, f32> FloatDensityClustering;
    class ClusterDensitySelection;
    FloatDensityClustering *clustering_;
    mutable bool *activeClusters_;
};

/**
 * Batched feature scorer using integer computations.
 *
 * Requires an acoustic model with a single (globally pooled) diagonal covariance.
 * Scores are calculated using the maximum approximation (max. over densities).
 * The distance computation uses SIMD instructions (SSE2).
 * Uses quantization to one byte for each vector component.
 *
 * Data is stored in raw arrays with 16-byte alignment.
 * (required by the SSE instructions).
 *
 * Mean vectors and features are preprocessed by multiplication with the inverse
 * square root of the variance, followed by quantization.
 */
class BatchIntFeatureScorer : public BatchFeatureScorerBase
{
public:
    BatchIntFeatureScorer(const Core::Configuration &c);
    BatchIntFeatureScorer(const Core::Configuration &c, Core::Ref<const MixtureSet> mixtureSet);
    virtual ~BatchIntFeatureScorer();

    virtual void init(const MixtureSet &mixtureSet);

protected:
    virtual void setFeature(size_t pos, const FeatureVector &f) const;
    template<class DensitySelector>
    inline void fillScoreCacheTpl(EmissionIndex e, u32 featureIndex, u32 length, const DensitySelector &selector) const;
    virtual void fillScoreCache(EmissionIndex e, u32 featureIndex, u32 length) const;
protected:
    typedef u8 QuantizedType;
    /**
     * Number of ints that are processed in one iteration
     * of the distance computation.
     */
    static const size_t BlockSize;
    struct MultiplyAndQuantize;

    f32 quantizationScale(const MixtureSet &mixtureSet) const;

    /**
     * feature buffer
     */
    mutable QuantizedType *features_;
    /**
     * means dived by variance and quantized to one byte
     */
    QuantizedType *means_;
    /**
     * the reciprocal square root of the variance vector.
     */
    f32 *variance_;
    /**
     * quantized density weight + normalization factor for each density.
     */
    s32 *constants_;
    /**
     * 2 * squared quantization scaling factor
     */
    f32 scale_;
};

class BatchPreselectionIntFeatureScorer : public BatchIntFeatureScorer
{
public:
    BatchPreselectionIntFeatureScorer(const Core::Configuration &c);
    BatchPreselectionIntFeatureScorer(const Core::Configuration &c, Core::Ref<const MixtureSet> mixtureSet);
    virtual ~BatchPreselectionIntFeatureScorer();

    virtual void init(const MixtureSet &mixtureSet);

protected:
    virtual void setFeature(size_t pos, const FeatureVector &f) const;
    virtual void fillScoreCache(EmissionIndex e, u32 featureIndex, u32 length) const;

private:
    typedef DensityClustering<QuantizedType, s32> IntDensityClustering;
    class ClusterIterator;
    IntDensityClustering *clustering_;
    mutable bool *activeClusters_;
};

/**
 * Batched feature scorer using integer computations for a
 * fixed size feature vector.
 *
 * See BatchIntFeatureScorer.
 */
class BatchUnrolledIntFeatureScorer : public BatchIntFeatureScorer
{
public:
    BatchUnrolledIntFeatureScorer(const Core::Configuration &c);
    BatchUnrolledIntFeatureScorer(const Core::Configuration &c, Core::Ref<const MixtureSet> mixtureSet);
    virtual ~BatchUnrolledIntFeatureScorer() {}

    virtual void init(const MixtureSet &mixtureSet);

protected:
    virtual void fillScoreCache(EmissionIndex e, u32 featureIndex, u32 length) const;

protected:
    static const size_t Dimension;
};

} // namespace Mm

#endif /* _MM_BATCH_FEATURE_SCORER_HH */
