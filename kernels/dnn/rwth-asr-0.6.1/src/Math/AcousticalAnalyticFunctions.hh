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
#ifndef _MATH_ACOUSTICAL_ANALYTIC_FUNCTION_HH
#define _MATH_ACOUSTICAL_ANALYTIC_FUNCTION_HH

#include "AnalyticFunction.hh"
#include <Core/Utility.hh>

namespace Math {

    /** Derived Mel warping function core: d f_{Mel} / df = 1.0 / log(10) / (700.0 + f) */
    struct DerivedMelWarpingCore : public UnaryAnalyticFunction {
	virtual Result value(Argument f) const { return 1.0 / log(10) / (700.0 + f); }
    };

    /** Inverse of Mel warping function core: f = (10^(f_{Mel}) - 1) * 700 */
    struct InverseMelWarpingCore : public UnaryAnalyticFunction {
	inline virtual Result value(Argument f) const;
	inline virtual UnaryAnalyticFunctionRef invert() const;
    };

    /** Mel warping function core f_{Mel} = log10 (1 + f / 700) */
    struct MelWarpingCore : public UnaryAnalyticFunction {
	virtual Result value(Argument f) const { return log10(1.0 + f / 700.0); }
	virtual UnaryAnalyticFunctionRef derive() const {
	    return UnaryAnalyticFunctionRef(new DerivedMelWarpingCore);
	}
	virtual UnaryAnalyticFunctionRef invert() const {
	    return UnaryAnalyticFunctionRef(new InverseMelWarpingCore);
	}
    };

    AnalyticFunction::Result InverseMelWarpingCore::value(Argument f) const {
	return (pow(10, f) - 1.0) * 700.0;
    }

    UnaryAnalyticFunctionRef InverseMelWarpingCore::invert() const {
	return UnaryAnalyticFunctionRef(new MelWarpingCore);
    }

    /** Equal Loudness Preemphasis
     *  E(f) = (omega^4 * (omega^2 + 56.8e6)) /
     *           ((omega^2 + 6.3e6))^2 * (omega^2 + 0.38e9) * (omega^6 + 9.58e26))
     *  E(f) *= 9.58e26 (last pole's omega) to get values in order of magnitude of 1 at 4000 Hz.
     */
    struct EqualLoudnessPreemphasis : public UnaryAnalyticFunction {
	virtual Result value(Argument f) const;
    };

    /** Equal Loudness Preemphasis optimized for 4KHz bandwidth
     *  E(f) = (omega^4 * (omega^2 + 56.8e6)) / ((omega^2 + 6.3e6))^2 * (omega^2 + 0.38e9))
     */
    struct EqualLoudnessPreemphasis4Khz : public UnaryAnalyticFunction {
	virtual Result value(Argument f) const;
    };

    /** 40dB equal Loudness Preemphasis (taken from Sprachcore/ICSI)
     *  E(f) = (f^2 / (f^2 + 1.6e5)) * (f^2 / (f^2 + 1.6e5))
     *  	* ( (f^2 + 1.44e6) / (f^2 + 9.61e6) )
     */
    struct EqualLoudnessPreemphasis40dB : public UnaryAnalyticFunction {
	virtual Result value(Argument f) const;
    };

    /**
     *  Bilinear Transform.
     *  x_{B} = x - T / pi * arctan(a * sin(2 * pi / T * x) / (1 + a * cos(2 * pi / T * x))),
     *  where 'a' controls curvature, and 'T' is the periodicity of the arctan term.
     *  Hint: pont T/2 is mapped on itself.
     */
    struct BilinearTransform : public UnaryAnalyticFunction {
	Argument a_;
	Argument T_;
    public:
	BilinearTransform(Argument a, Argument T) : a_(a), T_(T) {
	    require(Core::abs(a) < 1);
	    require(T > 0);
	}

	virtual Result value(Argument x) const {
	    return x - T_ / M_PI * atan(a_ * sin(2 * M_PI / T_ * x) / (1 + a_ * cos(2 * M_PI / T_ * x)));
	}
	virtual UnaryAnalyticFunctionRef invert() const {
	    return UnaryAnalyticFunctionRef(new BilinearTransform(-a_, T_));
	}
    };

} // namespace Math

#endif //_MATH_ACOUSTICAL_ANALYTIC_FUNCTION_HH
