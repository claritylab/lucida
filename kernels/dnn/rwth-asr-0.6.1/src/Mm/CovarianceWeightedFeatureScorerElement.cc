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
#include "CovarianceWeightedFeatureScorerElement.hh"
#include "Utilities.hh"

using namespace Mm;


void CovarianceWeightedFeatureScorerElement::operator=(const Covariance &covariance)
{
    const std::vector<VarianceType> &diagonal = covariance.diagonal();
    const std::vector<f64> &weights = covariance.weights();

    require(checkDiagonal(diagonal));
    calculateInverseSquareRootDiagonal(diagonal);
    calculateNormalizationFactor(diagonal,weights);
}

void CovarianceWeightedFeatureScorerElement::calculateInverseSquareRootDiagonal(
    const std::vector<VarianceType> &diagonal)
{
    inverseSquareRootDiagonal_.resize(diagonal.size());
    std::transform(diagonal.begin(), diagonal.end(),
		   inverseSquareRootDiagonal_.begin(), inverseSquareRoot<VarianceType>());
}

void CovarianceWeightedFeatureScorerElement::calculateNormalizationFactor(
    const std::vector<VarianceType> &diagonal)
{
    logNormalizationFactor_ = gaussLogNormFactor(diagonal.begin(), diagonal.end());
}

void CovarianceWeightedFeatureScorerElement::calculateNormalizationFactor(
    const std::vector<VarianceType> &diagonal, const std::vector<f64> &weights)
{
    logNormalizationFactor_ = gaussLogNormFactor(diagonal.begin(), diagonal.end(), weights.begin(), weights.end());
}

bool CovarianceWeightedFeatureScorerElement::checkDiagonal(const std::vector<VarianceType> &diagonal)
{
    return (std::find_if(diagonal.begin(), diagonal.end(),
			 std::bind2nd(std::less_equal<VarianceType>(), 0)) == diagonal.end());
}

void CovarianceWeightedFeatureScorerElement::scale(VarianceType factor) {

    std::transform(inverseSquareRootDiagonal_.begin(), inverseSquareRootDiagonal_.end(),
		   inverseSquareRootDiagonal_.begin(),
		   std::bind2nd(std::multiplies<VarianceType>(), factor));

    logNormalizationFactor_ *= factor * factor;
}
