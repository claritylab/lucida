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
#ifndef _MM_RPROP_OPTIMIZATION_HH
#define _MM_RPROP_OPTIMIZATION_HH

#include "Types.hh"
#include <Core/Utility.hh>
#include <Core/Configuration.hh>
#include <Core/Parameter.hh>

namespace Mm {

    /**
     * RpropSettings
     */
    template <class T>
    class RpropSettings
    {
    private:
	static const Core::ParameterFloat paramMinimumStepSize;
	static const Core::ParameterFloat paramMaximumStepSize;
	static const Core::ParameterFloat paramFactorToIncreaseStepSize;
	static const Core::ParameterFloat paramFactorToDecreaseStepSize;
	static const Core::ParameterBool paramUseWeightBacktracking;
    public:
	T minStepSize;
	T maxStepSize;
	T increasingFactor;
	T decreasingFactor;
	bool useWeightBacktracking;
    public:
	RpropSettings();
	RpropSettings(const Core::Configuration &);
    };

    template class RpropSettings<f32>;
    template class RpropSettings<f64>;

    /**
     * RpropOptimization
     */
    template <class T>
    class RpropOptimization
    {
    public:
	typedef RpropSettings<T> Settings;
    private:
	enum Action { decrease, increase, hold };
    private:
	Settings settings_;
    protected:
	std::vector<T> previousToPrevious_;
	std::vector<T> stepSizes_;
    private:
	T minStepSize() const { return settings_.minStepSize; }
	T maxStepSize() const { return settings_.maxStepSize; }
	T increasingFactor() const { return settings_.increasingFactor; }
	T decreasingFactor() const { return settings_.decreasingFactor; }
	bool useWeightBacktracking() const { return settings_.useWeightBacktracking; }
	s8 sign(T x) const { return (x == 0) ? 0 : ((x > 0) ? 1 : -1); }
	s8 direction(ComponentIndex cmp) const { return sign(gradient(cmp)); }
	void increaseStepSize(ComponentIndex cmp) {
	    stepSizes_[cmp] = std::min(stepSizes_[cmp] * increasingFactor(), maxStepSize());
	}
	void decreaseStepSize(ComponentIndex cmp) {
	    stepSizes_[cmp] = std::max(stepSizes_[cmp] * decreasingFactor(), minStepSize());
	    setHold(cmp);
	}
	void setHold(ComponentIndex cmp) {
	    if (stepSizes_[cmp] > 0) stepSizes_[cmp] = -stepSizes_[cmp];
	}
	void unsetHold(ComponentIndex cmp) {
	    if (stepSizes_[cmp] < 0) stepSizes_[cmp] = -stepSizes_[cmp];
	}
	bool isHold(ComponentIndex cmp) const { return (stepSizes_[cmp] < 0); }
	T stepSize(ComponentIndex cmp) const { return Core::abs(stepSizes_[cmp]); }
	Action action(ComponentIndex cmp) const {
	    const T prevG = previous(cmp) - previousToPrevious_[cmp];
	    const T curG = gradient(cmp);
	    if (isHold(cmp)) return hold;
	    else if ((prevG * curG) >= 0) return increase;
	    else return decrease;
	}
    protected:
	virtual T previous(ComponentIndex) const = 0;
	virtual T gradient(ComponentIndex) const = 0;
    public:
	RpropOptimization();
	virtual ~RpropOptimization() {}

	void apply(std::vector<T> &result);
	void setStepSizes(const std::vector<T> &stepSizes) { stepSizes_ = stepSizes; }
	void setStepSizes(u32 dimension, T stepSize) {
	    stepSizes_.clear(); stepSizes_.resize(dimension, stepSize);
	}
	const std::vector<T>& stepSizes() const { return stepSizes_; }
	void setPreviousToPrevious(const std::vector<T> &p2p) { previousToPrevious_ = p2p; }
	void setSettings(const Settings &settings) { settings_ = settings; }
    };

#include "RpropOptimization.tcc"

} //namespace Mm

#endif //_MM_RPROP_OPTIMIZATION_HH
