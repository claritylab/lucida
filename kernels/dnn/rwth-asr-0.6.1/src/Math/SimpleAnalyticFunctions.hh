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
#ifndef _MATH_SIMPLE_ANALYTIC_FUNCTION_HH
#define _MATH_SIMPLE_ANALYTIC_FUNCTION_HH

#include "AnalyticFunction.hh"
#include <algorithm>

namespace Math {

    /** f(x) = c */
    class ConstantFunction : public UnaryAnalyticFunction {
    protected:
	Argument c_;
    public:
	ConstantFunction(Argument c) : c_(c) {}

	virtual Result value(Argument x) const { return c_; }
	virtual UnaryAnalyticFunctionRef derive() const {
	    return UnaryAnalyticFunctionRef(new ConstantFunction(0));
	}
	virtual UnaryAnalyticFunctionRef invert() const { defect(); /* not existent */ }
    };
    /** Creates a ConstantFunction object. */
    inline UnaryAnalyticFunctionRef createConstant(UnaryAnalyticFunction::Argument c) {
	return UnaryAnalyticFunctionRef(new ConstantFunction(c));
    }

    /** Piecewise constant function */
    class PiecewiseConstantFunction :
	public UnaryAnalyticFunction,
	public std::vector<std::pair<UnaryAnalyticFunction::Argument, UnaryAnalyticFunction::Result> >
    {
	typedef std::vector<std::pair<UnaryAnalyticFunction::Argument, UnaryAnalyticFunction::Result> > Precursor;
    private:
	typedef Precursor::value_type LimitAndValue;

	struct greaterOrEqual : public std::binary_function<LimitAndValue, Argument, bool> {
	    bool operator()(const LimitAndValue &x, const Argument &limit) const {
		return x.first >= limit;
	    }
	};
	std::vector<LimitAndValue>::const_iterator findSegment(const Argument &limit) const {
	    return std::find_if(begin(), end(), std::bind2nd(greaterOrEqual(), limit));
	}
    public:
	PiecewiseConstantFunction() {}
	PiecewiseConstantFunction(const Precursor &limitsAndValues) : Precursor(limitsAndValues) {}

	void insert(const LimitAndValue& limitAndValue) {
	    iterator pos = std::find_if(begin(), end(), std::bind2nd(greaterOrEqual(), limitAndValue.first));
	    Precursor::insert(pos, limitAndValue);
	}

	virtual Result value(Argument x) const {
	    const_iterator result = std::find_if(begin(), end(), std::bind2nd(greaterOrEqual(), x));
	    ensure(result != end());
	    return result->second;
	}
	virtual UnaryAnalyticFunctionRef derive() const { return createConstant(0); }
	virtual UnaryAnalyticFunctionRef invert() const { defect(); /* not existent */ }
    };

    /** f(x) = x */
    struct IdentityFunction : public UnaryAnalyticFunction {
	virtual Result value(Argument x) const { return x; }
	virtual UnaryAnalyticFunctionRef derive() const { return createConstant(1); }
	virtual UnaryAnalyticFunctionRef invert() const {
	    return UnaryAnalyticFunctionRef(this);
	}
    };

    /** f(x) = a * x */
    class ScalingFunction : public UnaryAnalyticFunction {
	Argument a_;
    public:
	ScalingFunction(Argument a) : a_(a) {}

	virtual Result value(Argument x) const { return a_ * x; }
	virtual UnaryAnalyticFunctionRef derive() const { return createConstant(a_); }
	virtual UnaryAnalyticFunctionRef invert() const {
	    return UnaryAnalyticFunctionRef(new ScalingFunction(1 / a_));
	}
    };

    /** f(x) = x + b */
    class OffsetFunction : public UnaryAnalyticFunction {
	Argument b_;
    public:
	OffsetFunction(Argument b) : b_(b) {}

	virtual Result value(Argument x) const { return x + b_; }
	virtual UnaryAnalyticFunctionRef derive() const { return createConstant(1); }
	virtual UnaryAnalyticFunctionRef invert() const {
	    return UnaryAnalyticFunctionRef(new OffsetFunction(-b_));
	}
    };

    /** f(x) = floorf(x) */
    struct FloorFloatFunction : public UnaryAnalyticFunction {
	virtual Result value(Argument x) const { return floorf(x); }
    };

    /** f(x) = sinh(x) */
    struct Sinh : public UnaryAnalyticFunction {
	inline virtual Result value(Argument x) const;
	inline virtual UnaryAnalyticFunctionRef derive() const;
	inline virtual UnaryAnalyticFunctionRef invert() const;
    };

    /** f(x) = asinh(x)
     *  Remarks:
     *    -asinh(x) = log(x + sqrt(x^2 + 1)).
     *     Proof: t = sinh(log(x + sqrt(x^2 + 1))) and apply definition of sinh.
     *    -derivative: 1 / sqrt(x^2 + 1).
     *    -inverse: sinh(t).
     */
    struct ArcSinh : public UnaryAnalyticFunction {
	inline virtual Result value(Argument x) const;
	inline virtual UnaryAnalyticFunctionRef derive() const;
	inline virtual UnaryAnalyticFunctionRef invert() const;
    };

    /** f(x) = d asinh(x) / dx = 1 / sqrt(x^2 + 1) */
    struct DerivedArcSinh : public UnaryAnalyticFunction {
	virtual Result value(Argument x) const { return (Argument)1 / sqrt(x * x + (Argument)1); }
    };

    /** f(x) = cosh(x) */
    struct Cosh : public UnaryAnalyticFunction {
	inline virtual Result value(Argument x) const;
	inline virtual UnaryAnalyticFunctionRef derive() const;
	inline virtual UnaryAnalyticFunctionRef invert() const;
    };

    /** f(x) = acosh(x)
     *  Remarks:
     *    -acosh(x) = log(x + sqrt(x^2 - 1)). For proof @see ArcSinh
     *    -derivative: 1 / sqrt(x^2 - 1).
     *    -inverse: cosh(t).
     */
    struct ArcCosh : public UnaryAnalyticFunction {
	inline virtual Result value(Argument x) const;
	inline virtual UnaryAnalyticFunctionRef derive() const;
	inline virtual UnaryAnalyticFunctionRef invert() const;
    };

    /** f(x) = d acosh(x) / dx = 1 / sqrt(x^2 - 1) */
    struct DerivedArcCosh : public UnaryAnalyticFunction {
	virtual Result value(Argument x) const { return (Argument)1 / sqrt(x * x - (Argument)1); }
    };

    AnalyticFunction::Result Sinh::value(Argument x) const { return sinh(x); }
    UnaryAnalyticFunctionRef Sinh::derive() const { return UnaryAnalyticFunctionRef(new Cosh); }
    UnaryAnalyticFunctionRef Sinh::invert() const { return UnaryAnalyticFunctionRef(new ArcSinh); }

    AnalyticFunction::Result ArcSinh::value(Argument x) const { return asinh(x); }
    UnaryAnalyticFunctionRef ArcSinh::derive() const { return UnaryAnalyticFunctionRef(new DerivedArcSinh); }
    UnaryAnalyticFunctionRef ArcSinh::invert() const { return UnaryAnalyticFunctionRef(new Sinh); }

    AnalyticFunction::Result Cosh::value(Argument x) const { return cosh(x); }
    UnaryAnalyticFunctionRef Cosh::derive() const { return UnaryAnalyticFunctionRef(new Sinh); }
    UnaryAnalyticFunctionRef Cosh::invert() const { return UnaryAnalyticFunctionRef(new ArcCosh); }

    AnalyticFunction::Result ArcCosh::value(Argument x) const { return acosh(x); }
    UnaryAnalyticFunctionRef ArcCosh::derive() const { return UnaryAnalyticFunctionRef(new DerivedArcCosh); }
    UnaryAnalyticFunctionRef ArcCosh::invert() const { return UnaryAnalyticFunctionRef(new Cosh); }

    /** f(x, y) = x + y */
    struct AdditionFunction : public BinaryAnalyticFunction {
	virtual Result value(Argument x, Argument y) const { return x + y; };
    };

    /** f(x, y) = x - y */
    struct SubstractionFunction : public BinaryAnalyticFunction {
	virtual Result value(Argument x, Argument y) const { return x - y; };
    };

    /** f(x, y) = x * y */
    struct MultiplicationFunction : public BinaryAnalyticFunction {
	virtual Result value(Argument x, Argument y) const { return x * y; };
    };

    /** f(x, y) = x / y */
    struct DivisionFunction : public BinaryAnalyticFunction {
	virtual Result value(Argument x, Argument y) const { return x * y; };
    };

} // namespace Math

#endif //_MATH_SIMPLE_ANALYTIC_FUNCTION_HH
