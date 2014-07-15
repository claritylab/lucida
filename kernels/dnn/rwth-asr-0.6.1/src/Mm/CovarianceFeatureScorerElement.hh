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
#ifndef _MM_COVARIANCE_FEATURE_SCORER_ELEMENT_HH
#define _MM_COVARIANCE_FEATURE_SCORER_ELEMENT_HH

#include "GaussDensity.hh"

namespace Mm {

    class CovarianceFeatureScorerElement {
    protected:
	std::vector<VarianceType> inverseSquareRootDiagonal_;
	/** N * log (2 * pi) + sum_i{log(variance_i)} */
	Score logNormalizationFactor_;
    protected:
	void calculateInverseSquareRootDiagonal(const std::vector<VarianceType> &diagonal);
	void calculateNormalizationFactor(const std::vector<VarianceType> &diagonal);

	bool checkDiagonal(const std::vector<VarianceType> &diagonal);
    public:
	CovarianceFeatureScorerElement() {}

	void operator=(const Covariance &covariance);

	void scale(VarianceType factor);

	const std::vector<VarianceType>& inverseSquareRootDiagonal() const {
	    return inverseSquareRootDiagonal_;
	}

	Score logNormalizationFactor() const { return logNormalizationFactor_; }
    };

} // namespace Mm

#endif // _MM_COVARIANCE_FEATURE_SCORER_ELEMENT_HH
