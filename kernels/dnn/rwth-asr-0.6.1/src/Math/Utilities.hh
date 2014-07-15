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
#ifndef _MATH_UTILITIES_HH
#define _MATH_UTILITIES_HH

#include <numeric>
#include <functional>
#include <cmath>
#include <Core/Utility.hh>

namespace Math {

    /** absolute difference function
     * @return |x - y|
     */
    template<class T>
    struct absoluteDifference : public std::binary_function<T, T, T> {
	T operator()(T x, T y) { return Core::abs(x - y); }
    };


    /** absolute difference to the power function
     * @return |x - y|^power
     */
    template<class T>
    class absoluteDifferencePower : public std::binary_function<T, T, T> {
    private:
	f64 power_;
    public:
	absoluteDifferencePower(const f64 power) : power_(power) {}

	T operator()(T x, T y) { return (T)pow(Core::abs(x - y), power_); }
    };

    /** absolute difference square-root function
     * @return |x - y|^0.5
     */
    template<class T>
    struct absoluteDifferenceSquareRoot : public std::binary_function<T, T, T> {
	T operator()(T x, T y) { return (T)sqrt(Core::abs(x - y)); }
    };

    /** absolute difference square function
     * @return |x - y|^2
     */
    template<class T>
    struct absoluteDifferenceSquare : public std::binary_function<T, T, T> {
	T operator()(T x, T y) { return (x - y) * (x - y); }
    };

    /*
     * returns true, if the quadratic equation has a real solution
     */
    template<typename T>
    bool solveQuadraticEquation(T p, T q, T& xplus, T &xminus){
	T discriminant = 0.25 * p * p - q;
	if (discriminant < 0)
	    return false;
	else{
	    T delta = std::sqrt(discriminant);
	    xminus = -0.5 * p - delta;
	    xplus = -0.5 * p + delta;
	    return true;
	}
    }

    /**
     * isnan(val) returns true if val is nan.
     * We cannot rely on std::isnan or x!=x, because GCC may wrongly optimize it
     * away when compiling with -ffast-math (default in RASR), even with volatile,
     * because -ffast-math implies -ffinite-math-only.
     * Earlier, we checked the binary representation here, but that is of course
     * architecture dependent.
     * Most platforms have __isnan, which is not handled by the compiler, but
     * implemented in libm (C stdlib).
     * Some references:
     *   http://refspecs.linuxbase.org/LSB_3.1.1/LSB-Core-generic/LSB-Core-generic/baselib---isnan.html
     *   http://stackoverflow.com/questions/22931147/stdisinf-does-not-work-with-ffast-math-how-to-check-for-infinity
     **/
    template<typename T>
    bool isnan(T val);

    template<> inline bool isnan<float>(float val) { return __isnanf(val); }
    template<> inline bool isnan<double>(double val) { return __isnan(val); }
    template<> inline bool isnan<long double>(long double val) { return __isnanl(val); }


} // namespace Math

#endif // _MATH_UTILITIES_HH
