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
#ifndef _MATH_ANALYTIC_FUNCTION_HH
#define _MATH_ANALYTIC_FUNCTION_HH

#include <Core/Assertions.hh>
#include <Core/ReferenceCounting.hh>

namespace Math {

    /** Interface class for analytic functions */
    class AnalyticFunction : public Core::ReferenceCounted {
    public:
	typedef f64 Argument;
	typedef f64 Result;
    public:
	AnalyticFunction() {}
    };

    class UnaryAnalyticFunction;
    typedef Core::Ref<const UnaryAnalyticFunction> UnaryAnalyticFunctionRef;

    /** Interface class for unary analytic functions */
    class UnaryAnalyticFunction : public AnalyticFunction {
    public:
	UnaryAnalyticFunction() {}
	virtual ~UnaryAnalyticFunction() {}

	virtual Result value(Argument) const = 0;
	virtual UnaryAnalyticFunctionRef derive() const {
	    return UnaryAnalyticFunctionRef(); /* not implemented in derived class */
	}
	virtual UnaryAnalyticFunctionRef invert() const {
	    return UnaryAnalyticFunctionRef(); /* not implemented in derived class */
	}
    };

    /** Base class for Composition of two analytic functions */
    class AnalyticUnaryComposition : public UnaryAnalyticFunction {
    protected:
	UnaryAnalyticFunctionRef f_;
	UnaryAnalyticFunctionRef g_;
    public:
	AnalyticUnaryComposition(UnaryAnalyticFunctionRef f, UnaryAnalyticFunctionRef g) : f_(f), g_(g) {
	    require(f_); require(g_);
	}
    };

    /** f(x)+g(x) */
    class AnalyticSummation : public AnalyticUnaryComposition {
    public:
	AnalyticSummation(UnaryAnalyticFunctionRef f, UnaryAnalyticFunctionRef g) :
	    AnalyticUnaryComposition(f, g) {}

	virtual Result value(Argument x) const { return f_->value(x) + g_->value(x); }
	virtual UnaryAnalyticFunctionRef derive() const {
	    UnaryAnalyticFunctionRef fDerived = f_->derive(), gDerived = g_->derive();
	    return (!fDerived || !gDerived) ?  UnaryAnalyticFunctionRef() :
		UnaryAnalyticFunctionRef(new AnalyticSummation(fDerived, gDerived));
	}
	virtual UnaryAnalyticFunctionRef invert() const {
	    return UnaryAnalyticFunctionRef(); /* no general rule exists, try to use nesting if possible */
	}
    };
    /** Creates an AnalyticSummation object */
    inline UnaryAnalyticFunctionRef operator+(UnaryAnalyticFunctionRef f, UnaryAnalyticFunctionRef g) {
	return UnaryAnalyticFunctionRef(new AnalyticSummation(f, g));
    }

    /** f(x)*g(x) */
    class AnalyticMultiplication : public AnalyticUnaryComposition {
    public:
	AnalyticMultiplication(UnaryAnalyticFunctionRef f, UnaryAnalyticFunctionRef g) :
	    AnalyticUnaryComposition(f, g) {}

	virtual Result value(Argument x) const { return f_->value(x) * g_->value(x); }
	virtual UnaryAnalyticFunctionRef derive() const {
	    UnaryAnalyticFunctionRef fDerived = f_->derive(), gDerived = g_->derive();
	    return (!fDerived || !gDerived) ?  UnaryAnalyticFunctionRef() :
		UnaryAnalyticFunctionRef(new AnalyticMultiplication(fDerived, g_)) +
		UnaryAnalyticFunctionRef(new AnalyticMultiplication(f_, gDerived));
	}
	virtual UnaryAnalyticFunctionRef invert() const {
	    return UnaryAnalyticFunctionRef(); /* no general rule exists, try to use nesting if possible */
	}
    };
    /** Creates an AnalyticMultiplication object */
    inline UnaryAnalyticFunctionRef operator*(UnaryAnalyticFunctionRef f, UnaryAnalyticFunctionRef g) {
	return UnaryAnalyticFunctionRef(new AnalyticMultiplication(f, g));
    }

    /** f(g(x)) */
    class AnalyticNesting : public AnalyticUnaryComposition {
    public:
	AnalyticNesting(UnaryAnalyticFunctionRef f, UnaryAnalyticFunctionRef g) :
	    AnalyticUnaryComposition(f, g) {}

	virtual Result value(Argument x) const { return f_->value(g_->value(x)); }
	virtual UnaryAnalyticFunctionRef derive() const {
	    UnaryAnalyticFunctionRef fDerived = f_->derive(), gDerived = g_->derive();
	    return (!fDerived || !gDerived) ?  UnaryAnalyticFunctionRef() :
		UnaryAnalyticFunctionRef(new AnalyticNesting(fDerived, g_)) * gDerived;
	}
	virtual UnaryAnalyticFunctionRef invert() const {
	    UnaryAnalyticFunctionRef fInverse = f_->invert(), gInverse = g_->invert();
	    return (!fInverse || !gInverse) ?  UnaryAnalyticFunctionRef() :
		UnaryAnalyticFunctionRef(new AnalyticNesting(gInverse, fInverse));
	}

    };
    /** Creates an AnalyticNesting object. */
    inline  UnaryAnalyticFunctionRef nest(UnaryAnalyticFunctionRef f, UnaryAnalyticFunctionRef g) {
	return UnaryAnalyticFunctionRef(new AnalyticNesting(f, g));
    }

    class BinaryAnalyticFunction;
    typedef Core::Ref<const BinaryAnalyticFunction> BinaryAnalyticFunctionRef;

    /** Interface class for binary analytic functions */
    class BinaryAnalyticFunction : public AnalyticFunction {
    public:
	BinaryAnalyticFunction() {}
	virtual ~BinaryAnalyticFunction() {}

	virtual Result value(Argument, Argument) const = 0;
    };

} // namespace Math

#endif //_MATH_ANALYTIC_FUNCTION_HH
