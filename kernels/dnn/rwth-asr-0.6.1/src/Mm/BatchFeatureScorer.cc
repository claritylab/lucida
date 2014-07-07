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
#include <Core/Allocator.hh>
#include <Mm/CovarianceFeatureScorerElement.hh>
#include <Mm/DensityClustering.hh>
#include <Mm/SSE2CodeGenerator.hh>
#include <Mm/Utilities.hh>
#include <functional>
#include <algorithm>
#include <xmmintrin.h>
#include <emmintrin.h>
#include "BatchFeatureScorer.hh"

using namespace Mm;

const Core::ParameterInt BatchFeatureScorerBase::paramBufferSize(
	"buffer-size", "number of pre-calculated scores", 4, 0);

BatchFeatureScorerBase::BatchFeatureScorerBase(const Core::Configuration &c)
  : Core::Component(c), FeatureScorer(c),
    scores_(0),
    paddedDimension_(0),
    dimension_(0),
    nMixtures_(0),
    nDensities_(0),
    currentFeature_(0),
    buffered_(0),
    bufferSize_(paramBufferSize(c))
{
    log("batch feature scorer using buffer size %d", bufferSize_);
}

BatchFeatureScorerBase::~BatchFeatureScorerBase()
{
    ::free(scores_);
}

void BatchFeatureScorerBase::reset() const
{
    std::fill(cached_.begin(), cached_.end(), false);
    currentFeature_ = 0;
    buffered_ = 0;
}


void BatchFeatureScorerBase::addFeature(const FeatureVector &f) const
{
    require(!bufferFilled());
    setFeature(buffered_, f);
    ++buffered_;
}

/**
 * Mark the scores for all mixtures as invalid for the
 * given buffer position.
 */
void BatchFeatureScorerBase::invalidateCache(size_t pos) const
{
    for (size_t m = 0; m < nMixtures_; ++m) {
	cached_[m * bufferSize_ + pos] = false;
    }
}

void BatchFeatureScorerBase::initialize(const MixtureSet &mixtureSet)
{
    require(paddedDimension_ > 0);
    nMixtures_ = mixtureSet.nMixtures();
    offsets_.resize(nMixtures_ + 1, 0);
    nDensities_ = 0;
    for (size_t m = 0; m < nMixtures_; ++m) {
	const Mixture &mixture = *mixtureSet.mixture(m);
	offsets_[m] = nDensities_;
	nDensities_ += mixture.nDensities();
    }
    offsets_[nMixtures_] = nDensities_;
    Core::allocateAlignedVector<f32>(&scores_, nMixtures_ * bufferSize_, 0.0, 16);
    cached_.resize(nMixtures_ * bufferSize_, false);
}

FeatureScorer::Scorer BatchFeatureScorerBase::getScorer(const FeatureVector &f) const
{
    require(bufferFilled());
    s32 posToAdd = currentFeature_ ?
		   (currentFeature_ - 1) % bufferSize_ : bufferSize_ - 1;
    setFeature(posToAdd, f);
    ++buffered_;
    invalidateCache(posToAdd);
    Scorer result(new ContextScorer(this, currentFeature_, buffered_));
    currentFeature_ = (currentFeature_ + 1) % bufferSize_;
    --buffered_;
    return result;
}

FeatureScorer::Scorer BatchFeatureScorerBase::flush() const
{
    verify(buffered_ > 0);
    require(!bufferEmpty());
    Scorer result(new ContextScorer(this, currentFeature_, buffered_));
    currentFeature_ = (currentFeature_ + 1) % bufferSize_;
    --buffered_;
    return result;
}

Score BatchFeatureScorerBase::getScore(EmissionIndex e, u32 featureIndex, u32 length) const
{
    const size_t posOffset = e * bufferSize_;
    const size_t pos = posOffset + featureIndex;
    if (cached_[pos]) return scores_[pos];
    fillScoreCache(e, featureIndex, length);
    return scores_[pos];
}

struct BatchFeatureScorerBase::AllDensitySelector
{
    bool operator()(u32 bufferIndex, size_t density) const {
	return true;
    }
    void seek(u32, size_t) const {}
    bool value() const { return true; }
    void next() const {}
};


// ================================================================

const size_t BatchFloatFeatureScorer::BlockSize = 8;

BatchFloatFeatureScorer::BatchFloatFeatureScorer(
	const Core::Configuration &c)
  : Core::Component(c), BatchFeatureScorerBase(c),
    features_(0),
    means_(0),
    variance_(0),
    constants_(0)
{}


BatchFloatFeatureScorer::BatchFloatFeatureScorer(
	const Core::Configuration &c,
	Core::Ref<const MixtureSet> mixtureSet)
  : Core::Component(c), BatchFeatureScorerBase(c),
    features_(0),
    means_(0),
    variance_(0),
    constants_(0)
{
    init(*mixtureSet);
}

BatchFloatFeatureScorer::~BatchFloatFeatureScorer()
{
    ::free(features_);
    ::free(means_);
    ::free(variance_);
    ::free(constants_);
}

void BatchFloatFeatureScorer::setFeature(size_t pos, const FeatureVector &f) const
{
    verify(pos < bufferSize_);
    f32 *feature = features_ + (pos * paddedDimension_);
    memset(feature, 0, sizeof(f32) * paddedDimension_);
    std::transform(f.begin(), f.end(), variance_, feature,
		   std::multiplies<f32>());
}

void BatchFloatFeatureScorer::init(const MixtureSet &mixtureSet)
{
    if (mixtureSet.nCovariances() != 1) {
	criticalError("feature scorer supports only globally pooled covariance");
    }
    dimension_ = mixtureSet.dimension();
    paddedDimension_ = ((dimension_ + BlockSize - 1) / BlockSize) * BlockSize;
    initialize(mixtureSet);

    Core::allocateAlignedVector<f32>(&variance_, paddedDimension_, 0.0, 16);
    CovarianceFeatureScorerElement covariance;
    covariance = *mixtureSet.covariance(0);
    std::copy(covariance.inverseSquareRootDiagonal().begin(),
	      covariance.inverseSquareRootDiagonal().end(),
	      variance_);
    const float logNormFactor = covariance.logNormalizationFactor();

    Core::allocateAlignedVector<f32>(&features_, bufferSize_ * paddedDimension_, 0.0, 16);
    Core::allocateAlignedVector<f32>(&means_, nDensities_ * paddedDimension_, 0.0, 16);
    Core::allocateAlignedVector<f32>(&constants_, nDensities_, 0.0, 16);
    for (size_t m = 0; m < nMixtures_; ++m) {
	const Mixture &mixture = *mixtureSet.mixture(m);
	f32 *mean = means_ + offsets_[m] * paddedDimension_;
	f32 *c = constants_ + offsets_[m];
	for (size_t dns = 0; dns < mixture.nDensities(); ++dns) {
	    const GaussDensity &density = *mixtureSet.density(mixture.densityIndex(dns));
	    verify(density.covarianceIndex() == 0);
	    const Mean &dnsMean = *mixtureSet.mean(density.meanIndex());
	    std::transform(dnsMean.begin(), dnsMean.end(), variance_, mean,
			   std::multiplies<float>());
	    mean += paddedDimension_;
	    *c = logNormFactor - 2 * mixture.logWeight(dns);
	    ++c;
	}
    }
}

void BatchFloatFeatureScorer::fillScoreCache(EmissionIndex e, u32 featureIndex, u32 length) const
{
    fillScoreCacheTpl<AllDensitySelector>(e, featureIndex, length, AllDensitySelector());
}

/**
 * Get the min score for all densities of mixture e, using the
 * given feature vectors.
 */
template<class DensitySelector>
void BatchFloatFeatureScorer::fillScoreCacheTpl(EmissionIndex e, u32 featureIndex, u32 length, const DensitySelector &selector) const
{
    const size_t startIdx = featureIndex;
    const size_t endIdx = featureIndex + length;
    const size_t posOffset = e * bufferSize_;
    for (size_t t = startIdx; t < endIdx; ++t) {
	size_t pos = posOffset + (t % bufferSize_);
	scores_[pos] = Core::Type<f32>::max;
	cached_[pos] = true;
    }
    __m128 x1, x2, s1, s2, c;
    const size_t endDns = offsets_[e + 1];
    f32 *scoreBase = scores_ + posOffset;
    for (size_t dns = offsets_[e]; dns < endDns; ++dns) {
	const f32 *mean = means_ + dns * paddedDimension_;
	const f32 *dnsConst = constants_ + dns;
	c = _mm_load_ss(dnsConst);
	for (u32 t = startIdx; t < endIdx; ++t) {
	    const size_t rp = (t % bufferSize_);
	    if (!selector(rp, dns)) continue;
	    const f32 *feature = features_ + (rp * paddedDimension_);
	    s1 = c;
	    s2 = _mm_setzero_ps();
	    for (int d = 0; d < paddedDimension_; d += BlockSize) {
		x1 = _mm_sub_ps(_mm_load_ps(mean + d), _mm_load_ps(feature + d));
		s1 = _mm_add_ps(s1, _mm_mul_ps(x1, x1));
		x2 = _mm_sub_ps(_mm_load_ps(mean + d + 4), _mm_load_ps(feature + d + 4));
		s2 = _mm_add_ps(s2, _mm_mul_ps(x2, x2));
	    }
	    s1 = _mm_add_ps(s1, s2);
	    s2 = s1;
	    s1 = _mm_shuffle_ps(s1, s2, _MM_SHUFFLE(1, 0, 3, 2));
	    s1 = _mm_add_ps(s1, s2);
	    s2 = s1;
	    s1 = _mm_shuffle_ps(s1, s2, _MM_SHUFFLE(2, 3, 0, 1));
	    s1 = _mm_add_ps(s1, s2);
	    f32 *score = scoreBase + rp;
	    _mm_store_ss(score, _mm_min_ps(_mm_load_ss(score), s1));
	}
    }
    for (size_t t = startIdx; t < endIdx; ++t) {
	f32 &s = *(scoreBase + (t % bufferSize_));
	if (s < Core::Type<f32>::max) s *= 0.5;
    }
}

// ================================================================

BatchPreselectionFloatFeatureScorer::BatchPreselectionFloatFeatureScorer(
	const Core::Configuration &c)
  : Core::Component(c), BatchFloatFeatureScorer(c),
    clustering_(new FloatDensityClustering(select("density-clustering"))),
    activeClusters_(new bool[bufferSize_ * clustering_->nClusters()]) {}

BatchPreselectionFloatFeatureScorer::BatchPreselectionFloatFeatureScorer(
	const Core::Configuration &c,
	Core::Ref<const MixtureSet> mixtureSet)
  : Core::Component(c), BatchFloatFeatureScorer(c),
    clustering_(new FloatDensityClustering(select("density-clustering"))),
    activeClusters_(new bool[bufferSize_ * clustering_->nClusters()])
{
    init(*mixtureSet);
}

BatchPreselectionFloatFeatureScorer::~BatchPreselectionFloatFeatureScorer()
{
    delete[] activeClusters_;
    delete clustering_;
}

void BatchPreselectionFloatFeatureScorer::init(const MixtureSet &mixtureSet)
{
    BatchFloatFeatureScorer::init(mixtureSet);
    clustering_->init(paddedDimension_, nDensities_);
    clustering_->build(means_);
    std::fill(activeClusters_, activeClusters_ + bufferSize_ * clustering_->nClusters(), false);
}

void BatchPreselectionFloatFeatureScorer::setFeature(size_t pos, const FeatureVector &f) const
{
    BatchFloatFeatureScorer::setFeature(pos, f);
    clustering_->selectClusters(activeClusters_ + (pos * clustering_->nClusters()),
				features_ + (pos * paddedDimension_));
}

class BatchPreselectionFloatFeatureScorer::ClusterDensitySelection
{
public:
    ClusterDensitySelection(const bool *activeClusters, const DensityClusteringBase &clustering)
	    : activeClusters_(activeClusters), clustering_(clustering) {}

    bool operator()(u32 bufferIndex, size_t density) const {
	return activeClusters_[bufferIndex * clustering_.nClusters() + clustering_.clusterIndexForDensity(density)];
    }
private:
    const bool *activeClusters_;
    const DensityClusteringBase &clustering_;
};

void BatchPreselectionFloatFeatureScorer::fillScoreCache(EmissionIndex e, u32 featureIndex, u32 length) const
{
    fillScoreCacheTpl(e, featureIndex, length, ClusterDensitySelection(activeClusters_, *clustering_));
    f32 *scores = scores_ + e * bufferSize_;
    for (u32 t = featureIndex; t < featureIndex + length; ++t) {
	f32 &s = *(scores + (t % bufferSize_));
	if(s == Core::Type<f32>::max) s = clustering_->backoffScore();
    }
}

// ================================================================

const size_t BatchIntFeatureScorer::BlockSize = 16;

struct BatchIntFeatureScorer::MultiplyAndQuantize
{
    u8 operator()(f32 a, f32 b) const {
	return quantize<f32, u8>()(a * b);
    }
};

BatchIntFeatureScorer::BatchIntFeatureScorer(
	const Core::Configuration &c)
  : Core::Component(c), BatchFeatureScorerBase(c),
    features_(0),
    means_(0),
    variance_(0),
    constants_(0)
{}


BatchIntFeatureScorer::BatchIntFeatureScorer(
	const Core::Configuration &c,
	Core::Ref<const MixtureSet> mixtureSet)
  : Core::Component(c), BatchFeatureScorerBase(c),
    features_(0),
    means_(0),
    variance_(0),
    constants_(0)
{
    init(*mixtureSet);
}

BatchIntFeatureScorer::~BatchIntFeatureScorer()
{
    ::free(features_);
    // means_ use space in features_
    ::free(variance_);
    ::free(constants_);
}

f32 BatchIntFeatureScorer::quantizationScale(const MixtureSet &mixtureSet) const
{
    require(variance_);
    MeanType minMean = Core::Type<MeanType>::max;
    MeanType maxMean = Core::Type<MeanType>::min;

    for (DensityIndex i = 0; i < mixtureSet.nDensities(); ++ i) {
	const GaussDensity *density = mixtureSet.density(i);
	const std::vector<MeanType> *mean = mixtureSet.mean(density->meanIndex());

	for (ComponentIndex cmp = 0; cmp < mean->size(); ++ cmp) {
	    MeanType dividedMean = (*mean)[cmp] * variance_[cmp];
	    minMean = std::min(minMean, dividedMean);
	    maxMean = std::max(maxMean, dividedMean);
	}
    }
    int quantizedIntervalSize =
	(int)Core::Type<QuantizedType>::max - (int)Core::Type<QuantizedType>::min;
    f32 intervalSize = 2 * std::max(Core::abs(minMean), Core::abs(maxMean));
    return static_cast<f32>(quantizedIntervalSize) / (1.25 * intervalSize);
}

void BatchIntFeatureScorer::init(const MixtureSet &mixtureSet)
{
    if (mixtureSet.nCovariances() != 1) {
	criticalError("feature scorer supports only globally pooled variance");
    }
    dimension_ = mixtureSet.dimension();
    paddedDimension_ = ((dimension_ + BlockSize - 1) / BlockSize) * BlockSize;
    initialize(mixtureSet);

    Core::allocateAlignedVector<f32>(&variance_, paddedDimension_, 0.0, 16);
    CovarianceFeatureScorerElement covariance;
    covariance = *mixtureSet.covariance(0);
    std::copy(covariance.inverseSquareRootDiagonal().begin(),
	      covariance.inverseSquareRootDiagonal().end(),
	      variance_);

    f32 scale = quantizationScale(mixtureSet);
    f32 scaleSquared = scale * scale;
    scale_ = 2.0 * scaleSquared;
    std::transform(variance_, variance_ + paddedDimension_, variance_,
		   std::bind2nd(std::multiplies<f32>(), scale));
    const float logNormFactor = covariance.logNormalizationFactor() * scaleSquared;

    // allocate feature buffer and means in the same memory area.
    Core::allocateAlignedVector<QuantizedType>(&features_, bufferSize_ * paddedDimension_ +
					       nDensities_ * paddedDimension_, 0, 16);
    means_ = features_ + bufferSize_ * paddedDimension_;
    Core::allocateAlignedVector<s32>(&constants_, nDensities_, 0, 16);
    for (size_t m = 0; m < nMixtures_; ++m) {
	const Mixture &mixture = *mixtureSet.mixture(m);
	QuantizedType *mean = means_ + offsets_[m] * paddedDimension_;
	s32 *c = constants_ + offsets_[m];
	for (size_t dns = 0; dns < mixture.nDensities(); ++dns) {
	    const GaussDensity &density = *mixtureSet.density(mixture.densityIndex(dns));
	    verify(density.covarianceIndex() == 0);
	    const Mean &dnsMean = *mixtureSet.mean(density.meanIndex());
	    std::transform(dnsMean.begin(), dnsMean.end(), variance_, mean,
			   MultiplyAndQuantize());
	    mean += paddedDimension_;
	    *c = static_cast<s32>(logNormFactor - scale_ * mixture.logWeight(dns));
	    ++c;
	}
    }
}

void BatchIntFeatureScorer::setFeature(size_t pos, const FeatureVector &f) const
{
    verify(pos < bufferSize_);
    QuantizedType *feature = features_ + (pos * paddedDimension_);
    memset(feature, 0, sizeof(QuantizedType) * paddedDimension_);
    std::transform(f.begin(), f.end(), variance_,
		   feature, MultiplyAndQuantize());
}

namespace {
static inline void addDistance(__m128i &x, __m128i &m, __m128i &sum)
{
    x = _mm_or_si128(_mm_subs_epu8(m, x), _mm_subs_epu8(x, m));
    // x = | m - x | = (d15 d14 ... d0)
    m = x;
    x = _mm_unpackhi_epi8(x, _mm_setzero_si128());
    // x = (d15 0 d14 0 d13 0 .. d8 0)
    m = _mm_unpacklo_epi8(m, _mm_setzero_si128());
    // m = (d7  0 d6  0 d5  0 .. d0 0)
    x = _mm_madd_epi16(x, x);
    // x = (d15 * d15 + d14 * d14, .. , d9 * d9 + d8 * d8)
    m = _mm_madd_epi16(m, m);
    // m = (d7 * d7 + d6 * d6, .. , d1 * d1 + d0 * d0)
    // accumulate
    sum = _mm_add_epi32(sum, x);
    sum = _mm_add_epi32(sum, m);
}

static inline s32 horizontalAdd(__m128i &sum)
{
    // sum = (s3 s2 s1 s0)
    __m128i s = _mm_add_epi32(
		   // (s1 s0 s1 s0)
		  _mm_shuffle_epi32(sum, _MM_SHUFFLE(3, 2, 3, 2)),
		   // (s3 s2 s3 s2)
		  _mm_shuffle_epi32(sum, _MM_SHUFFLE(1, 0, 1, 0)));
    // s = (s1+s3 s0+s2 s1+s3 s0+s2)
    return _mm_cvtsi128_si32(_mm_add_epi32(s,
		 // (s0+s3 s1+s2 s0+s2 s1+s3)
		  _mm_shuffle_epi32(s, _MM_SHUFFLE(2, 3, 0, 1))));
    // = (s0+s1+s2+s3)
}
}

template<class DensitySelector>
void BatchIntFeatureScorer::fillScoreCacheTpl(EmissionIndex e, u32 featureIndex, u32 length, const DensitySelector &selector) const
{
    const size_t startIdx = featureIndex;
    const size_t endIdx = featureIndex + length;
    const size_t posOffset = e * bufferSize_;
    for (size_t t = startIdx, i = 0; t < endIdx; ++t, ++i) {
	cached_[posOffset + (t % bufferSize_)] = true;
    }
    f32 *scoreBase = scores_ + posOffset;
    const size_t startDns = offsets_[e];
    const size_t endDns = offsets_[e + 1];
    const QuantizedType *meanStart = means_ + startDns * paddedDimension_;
    const s32 *dnsConstStart = constants_ + startDns;
    for (u32 t = startIdx; t < endIdx; ++t) {
	const size_t rp = (t % bufferSize_);
	selector.seek(rp, startDns);
	const QuantizedType *feature = features_ + (rp * paddedDimension_);
	const QuantizedType *mean = meanStart;
	_mm_prefetch(const_cast<char*>(reinterpret_cast<const char*>(mean)), _MM_HINT_T1);
	const s32 *dnsConst = dnsConstStart;
	s32 best = 2147483647;
	for (size_t dns = startDns; dns < endDns; ++dns) {
	    if (selector.value()) {
		__m128i sum = _mm_setzero_si128();
		for (int d = 0; d < paddedDimension_; d += BlockSize) {
		    __m128i m, x;
		    m = _mm_load_si128(reinterpret_cast<const __m128i*>(mean + d));
		    // m = mean[d .. d+15]
		    x = _mm_load_si128(reinterpret_cast<const __m128i*>(feature + d));
		    // x = feature[d .. d+15]
		    addDistance(x, m, sum);
		}
		{
		    s32 tmp = horizontalAdd(sum);
		    tmp += *dnsConst;
		    if (tmp < best) best = tmp;
		}
	    }
	    mean += paddedDimension_;
	    ++dnsConst;
	    selector.next();
	}
	f32 *result = scoreBase + rp;
	_mm_prefetch(const_cast<char*>(reinterpret_cast<const char*>(result)), _MM_HINT_NTA);
	*result = static_cast<f32>(best) / scale_;
    }
}

void BatchIntFeatureScorer::fillScoreCache(EmissionIndex e, u32 featureIndex, u32 length) const
{
    fillScoreCacheTpl<AllDensitySelector>(e, featureIndex, length, AllDensitySelector());
}

// ================================================================


BatchPreselectionIntFeatureScorer::BatchPreselectionIntFeatureScorer(
	const Core::Configuration &c)
  : Core::Component(c), BatchIntFeatureScorer(c),
    clustering_(new IntDensityClustering(select("density-clustering"))),
    activeClusters_(0) {}

BatchPreselectionIntFeatureScorer::BatchPreselectionIntFeatureScorer(
	const Core::Configuration &c,
	Core::Ref<const MixtureSet> mixtureSet)
  : Core::Component(c), BatchIntFeatureScorer(c),
    clustering_(new IntDensityClustering(select("density-clustering"))),
    activeClusters_(0)
{
    init(*mixtureSet);
}

BatchPreselectionIntFeatureScorer::~BatchPreselectionIntFeatureScorer()
{
    delete[] activeClusters_;
    delete clustering_;
}

void BatchPreselectionIntFeatureScorer::init(const MixtureSet &mixtureSet)
{
    BatchIntFeatureScorer::init(mixtureSet);
    clustering_->init(paddedDimension_, nDensities_);
    clustering_->build(means_);
    activeClusters_ = new bool[bufferSize_ * clustering_->nClusters()];
    std::fill(activeClusters_, activeClusters_ + bufferSize_ * clustering_->nClusters(), false);
}

void BatchPreselectionIntFeatureScorer::setFeature(size_t pos, const FeatureVector &f) const
{
    BatchIntFeatureScorer::setFeature(pos, f);
    clustering_->selectClusters(activeClusters_ + (pos * clustering_->nClusters()),
				features_ + (pos * paddedDimension_));
}

class BatchPreselectionIntFeatureScorer::ClusterIterator
{
public:
    ClusterIterator(const bool *activeClusters, const DensityClusteringBase &clustering)
	    : activeClusters_(activeClusters), clustering_(clustering), offset_(0) {}

    void seek(u32 bufferIndex, size_t firstDensity) const {
	offset_ = bufferIndex * clustering_.nClusters();
	cluster_ = clustering_.clusterIndexIterator(firstDensity);
    }
    bool value() const {
	return activeClusters_[offset_ + *cluster_];
    }
    void next() const { ++cluster_; }
private:
    const bool *activeClusters_;
    const DensityClusteringBase &clustering_;
    mutable DensityClusteringBase::ClusterIndexIterator cluster_;
    mutable u32 offset_;
};


void BatchPreselectionIntFeatureScorer::fillScoreCache(EmissionIndex e, u32 featureIndex, u32 length) const
{
    fillScoreCacheTpl(e, featureIndex, length, ClusterIterator(activeClusters_, *clustering_));
}


// ================================================================


const size_t BatchUnrolledIntFeatureScorer::Dimension = 3 * BatchUnrolledIntFeatureScorer::BlockSize;

BatchUnrolledIntFeatureScorer::BatchUnrolledIntFeatureScorer(
	const Core::Configuration &c)
  : Core::Component(c), BatchIntFeatureScorer(c) {}

BatchUnrolledIntFeatureScorer::BatchUnrolledIntFeatureScorer(
	const Core::Configuration &c,
	Core::Ref<const MixtureSet> mixtureSet)
  : Core::Component(c), BatchIntFeatureScorer(c)
{
    init(*mixtureSet);
}

void BatchUnrolledIntFeatureScorer::init(const MixtureSet &mixtureSet)
{
    BatchIntFeatureScorer::init(mixtureSet);
    if (paddedDimension_ > Dimension) {
	criticalError("This feature scorer supports only features with max. %d components",
		(int)Dimension);
    }
}

void BatchUnrolledIntFeatureScorer::fillScoreCache(EmissionIndex e, u32 featureIndex, u32 length) const
{
    const size_t startIdx = featureIndex;
    const size_t endIdx = featureIndex + length;
    const size_t posOffset = e * bufferSize_;
    for (size_t t = startIdx, i = 0; t < endIdx; ++t, ++i) {
	cached_[posOffset + (t % bufferSize_)] = true;
    }
    f32 *scoreBase = scores_ + posOffset;
    const size_t startDns = offsets_[e];
    const size_t endDns = offsets_[e + 1];
    const QuantizedType *meanStart = means_ + startDns * paddedDimension_;
    const s32 *dnsConstStart = constants_ + startDns;
    for (u32 t = startIdx; t < endIdx; ++t) {
	const size_t rp = (t % bufferSize_);
	const QuantizedType *feature = features_ + (rp * paddedDimension_);
	const QuantizedType *mean = meanStart;
	_mm_prefetch(const_cast<char*>(reinterpret_cast<const char*>(mean)), _MM_HINT_T1);
	const s32 *dnsConst = dnsConstStart;
	s32 best = 2147483647;
	for (size_t dns = startDns; dns < endDns; ++dns) {
	    __m128i sum = _mm_setzero_si128();
	    {
		__m128i m1, m2, m3, x1, x2, x3;
		m1 = _mm_load_si128(reinterpret_cast<const __m128i*>(mean));
		x1 = _mm_load_si128(reinterpret_cast<const __m128i*>(feature));
		m2 = _mm_load_si128(reinterpret_cast<const __m128i*>(mean + BlockSize));
		x2 = _mm_load_si128(reinterpret_cast<const __m128i*>(feature + BlockSize));
		addDistance(m1, x1, sum);
		m3 = _mm_load_si128(reinterpret_cast<const __m128i*>(mean + 2*BlockSize));
		x3 = _mm_load_si128(reinterpret_cast<const __m128i*>(feature + 2*BlockSize));
		addDistance(m2, x2, sum);
		addDistance(m3, x3, sum);
	    }
	    mean += Dimension;
	    {
		s32 tmp = horizontalAdd(sum);
		tmp += *dnsConst;
		if (tmp < best) best = tmp;
	    }
	    ++dnsConst;
	}
	f32 *result = scoreBase + rp;
	_mm_prefetch(const_cast<char*>(reinterpret_cast<const char*>(result)), _MM_HINT_NTA);
	*result = static_cast<f32>(best) / scale_;
    }
}
