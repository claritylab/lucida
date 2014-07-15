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
#ifndef _MATH_PIECEWISE_LINEAR_FUNCTION_HH
#define _MATH_PIECEWISE_LINEAR_FUNCTION_HH

#include "SimpleAnalyticFunctions.hh"
#include <algorithm>

namespace Math {

    class PiecewiseLinearFunction : public UnaryAnalyticFunction {
    private:
	Core::Ref<PiecewiseConstantFunction> a_;
	Core::Ref<PiecewiseConstantFunction> b_;
    private:
	void append(Argument limit, Argument a, Argument b);
	bool empty() const { return a_->empty(); }
	Argument lastLimit() const { return a_->back().first; }
    public:
	PiecewiseLinearFunction();

	void add(Argument limit, Argument a);
	/** Appends the last segment such that @param limit is mapped to itself. */
	void normalize(Argument limit);
	void clear() { a_->clear(), b_->clear(); }

	virtual Result value(Argument x) const { return a_->value(x) * x + b_->value(x); }
	virtual UnaryAnalyticFunctionRef derive() const { return a_; }
	virtual UnaryAnalyticFunctionRef invert() const;
    };

} // namespace Math

#endif //_MATH_PIECEWISE_LINEAR_FUNCTION_HH
