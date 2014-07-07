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
#include "RpropOptimization.hh"

using namespace Mm;

/**
 * RpropSettings
 */
template <class T>
const Core::ParameterFloat RpropSettings<T>::paramMinimumStepSize(
    "minimum-step-size",
    "minimum-step-size for update",
    0.0,
    0.0);

template <class T>
const Core::ParameterFloat RpropSettings<T>::paramMaximumStepSize(
    "maximum-step-size",
    "maximum-step-size for update",
    Core::Type<f32>::max,
    0.0);

template <class T>
const Core::ParameterFloat RpropSettings<T>::paramFactorToIncreaseStepSize(
    "factor-to-increase-step-size",
    "factor to increase step size if no sign flip in gradient",
    1.2,
    1.0);

template <class T>
const Core::ParameterFloat RpropSettings<T>::paramFactorToDecreaseStepSize(
    "factor-to-decrease-step-size",
    "factor to decrease step size in case of sign flip in gradient",
    0.5,
    0.0,
    1.0);

template <class T>
const Core::ParameterBool RpropSettings<T>::paramUseWeightBacktracking(
    "use-weight-backtracking",
    "weight is set to previous value in case of hold",
    false);

template <class T>
RpropSettings<T>::RpropSettings() :
    minStepSize(0),
    maxStepSize(Core::Type<T>::max),
    increasingFactor(1.2),
    decreasingFactor(0.5),
    useWeightBacktracking(false)
{}

template <class T>
RpropSettings<T>::RpropSettings(const Core::Configuration &configuration) :
    minStepSize(paramMinimumStepSize(configuration)),
    maxStepSize(paramMaximumStepSize(configuration)),
    increasingFactor(paramFactorToIncreaseStepSize(configuration)),
    decreasingFactor(paramFactorToDecreaseStepSize(configuration)),
    useWeightBacktracking(paramUseWeightBacktracking(configuration))
{}

/**
 * RpropOptimization
 */
template <class T>
RpropOptimization<T>::RpropOptimization()
{}

template <class T>
void RpropOptimization<T>::apply(std::vector<T> &result)
{
    for (ComponentIndex cmp = 0; cmp < result.size(); ++ cmp) {
	switch (action(cmp)) {
	case increase:
	    increaseStepSize(cmp);
	    result[cmp] += direction(cmp) * stepSize(cmp);
	    break;
	case decrease:
	    decreaseStepSize(cmp);
	    if (useWeightBacktracking()) {
		result[cmp] = previousToPrevious_[cmp];
	    }
	    break;
	case hold:
	    unsetHold(cmp);
	    result[cmp] += direction(cmp) * stepSize(cmp);
	    break;
	}
    }
}
