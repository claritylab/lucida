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
#if (defined(PROC_intel) || defined(PROC_x86_64))

#include "IntelOptimization.hh"
#include <Core/Application.hh>

using namespace Mm;

FeatureScorerIntelOptimization::FeatureScorerIntelOptimization(
    const Core::Configuration &c, ComponentIndex dimension) :
    Precursor(c)
#if !defined(DISABLE_SIMD)
    ,l2norm_(c, dimension)
#if !defined(ENABLE_SSE2)
    , reset_(c)
#endif
#endif
{
#if defined(DISABLE_SIMD)
    Core::Application::us()->warning("SIMD is not supported (in Valgrind executables)");
#endif
}

void FeatureScorerIntelOptimization::createDensityElement(
    Score scaledMinus2LogWeight,
    const Mean &mean,
    const CovarianceFeatureScorerElement &covarianceScorerElement,
    MixtureElement::Density &result)
{
    multiplyAndQuantize(mean,
			covarianceScorerElement.inverseSquareRootDiagonal(),
			result.preparedMean_);

    result.constantWeight_ = (s32)(scaledMinus2LogWeight +
				   covarianceScorerElement.logNormalizationFactor());
}

void FeatureScorerIntelOptimization::multiplyAndQuantize(
    const std::vector<FeatureType> &x, const std::vector<VarianceType> &y,
    PreparedFeatureVector &r)
{
    require(x.size() == y.size());

    r.resize(optimalVectorSize(x.size()));

    std::vector<FeatureType>::const_iterator xi = x.begin();
    std::vector<VarianceType>::const_iterator yi = y.begin();
    PreparedFeatureVector::iterator ri = r.begin();

    quantize<FeatureType, QuantizedType> quantize;

    for (; xi != x.end(); ++ xi, ++yi, ++ri)
	*ri = quantize(*xi * *yi);

    std::fill(ri, r.end(), 0);
}


#if defined(DISABLE_SIMD)

int FeatureScorerIntelOptimization::distance(
    const PreparedFeatureVector &mean, const PreparedFeatureVector &featureVector) const
{
    int df, score = 0;
    ComponentIndex cmp = 0;
    ComponentIndex dimension = featureVector.size();
    switch ((dimension - 1) % 8) {
	while (cmp < dimension) {
	case 7: df = (mean[cmp] - featureVector[cmp]); score += df * df; ++cmp;
	case 6: df = (mean[cmp] - featureVector[cmp]); score += df * df; ++cmp;
	case 5: df = (mean[cmp] - featureVector[cmp]); score += df * df; ++cmp;
	case 4: df = (mean[cmp] - featureVector[cmp]); score += df * df; ++cmp;
	case 3: df = (mean[cmp] - featureVector[cmp]); score += df * df; ++cmp;
	case 2: df = (mean[cmp] - featureVector[cmp]); score += df * df; ++cmp;
	case 1: df = (mean[cmp] - featureVector[cmp]); score += df * df; ++cmp;
	case 0: df = (mean[cmp] - featureVector[cmp]); score += df * df; ++cmp;
	}
    }
    verify_(cmp == dimension);
    return score;
}

#endif // DISABLE_SIMD

#endif // PROC_intel
