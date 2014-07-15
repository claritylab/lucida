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
#include "LevinsonLse.hh"
#include <Core/Utility.hh>

using namespace Math;


void LevinsonLeastSquares::setOrder(size_t N)
{
    E_.resize(N + 1);
    k_.resize(N + 1);
    alpha_.resize(N + 1, N + 1);
}

/* Changes compared to original Rabiner-Schafer version:
 * -Initialisation changed due to Hermansky/Morgan:
 *    Instead of starting the loop with i = 1,
 *    avoid dividing by square energy in E_[1] = (1 + k(1)^2) * E(0) = (1 + R(1)^2 / E(0)^2) * E(0)
 *    yeilds in  E_[1] = (R(0) + R(1) * k(1). Afterwords start loop with i = 2.
 * - Instead of y[n] = gain delta(t) + a1 y[n-1] + ... + aN y[n-N] model here the model with minuses
 *     y[n] = gain delta(t) - a1 y[n-1] - ... - aN y[n-N] is calculated.
 */
bool LevinsonLeastSquares::work(const std::vector<InputData> &R)
{
    require(!R.empty());

    setOrder(R.size() - 1);
    size_t i, j;
    size_t N = order();

    E_[0] = R[0];
    if (Core::isAlmostEqual(E_[0], (Data)0.0)) return false;

    alpha_[1][1] = k_[1] = - R[1] / R[0];
    E_[1] = R[0] + R[1] * k_[1];
    for(i = 2; i <= N; i++) {
	// k[i]
	k_[i]  = R[i];
	for(j = 1; j <= (i - 1); j++)
	    k_[i] += alpha_[j][i - 1] * R[i - j];

	if (Core::isAlmostEqual(E_[i - 1], (Data)0.0)) return false;
	k_[i] = - k_[i] / E_[i - 1];

	// a[j][i]
	alpha_[i][i] = k_[i];
	for(j = 1; j <= (i - 1); j++)
	    alpha_[j][i] = alpha_[j][i - 1] + k_[i] * alpha_[i - j][i - 1];

	// E[i]
	E_[i] = (1.0 - k_[i] * k_[i]) *  E_[i - 1];
    }
    return true;
}

/* Levinson recursion taken from Rabiner-Schafer 1978: Digital Processing of Speech Signal
bool LevinsonLeastSquares::work(const std::vector<InputData> &R)
{
    require(!R.empty());

    setOrder(R.size() - 1);
    size_t i, j;
    size_t N = order();

    E_[0] = R[0];
    for (i = 1; i <= N; i++) {
	k_[i] = R[i];
	for (j = 1; j <= (i - 1); j++) {
	    k_[i] -= alpha_[j][i - 1] * R[i - j];
	}
	k_[i] /= E_[i - 1];

	alpha_[i][i] = k_[i];
	for (j = 1; j <= (i - 1); ++ j) {
	    alpha_[j][i] = alpha_[j][i - 1] - k_[i] * alpha_[i - j][i - 1];
	}
	E_[i] = (1.0 - (k_[i] * k_[i])) * E_[i - 1];
    }
    return true;
}
*/
