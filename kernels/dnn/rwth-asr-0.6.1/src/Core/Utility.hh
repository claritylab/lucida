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
#ifndef _CORE_UTILITY_HH
#define _CORE_UTILITY_HH

#include <cmath>
#include <cstdlib>
#include <complex>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/time.h>
#include "Types.hh"
#include "Assertions.hh"
#include <Modules.hh>

#define TIMER_START(startTime) gettimeofday(&startTime, NULL);
#define TIMER_STOP(startTime, endTime, difference) \
    gettimeofday(&endTime, NULL); \
    difference += Core::timeDiff(startTime, endTime);

namespace Core {

    /**
     * reads from input stream until one of the specified delimiters
     * @param istream input stream to read from
     * @param string resulting string that has been read; will not include a trailing  delimiter
     * @param delim string of delimiters
     * @result returns EOF if nothing has been read, but the end of stream
     * has been reached, returns 0 at the end of the stream, but no delimiter
     * has been found, returns the position + 1 of the delimiter in delim
     * that the resulting string ends with
     **/
    int getline(std::istream&, std::string&, std::string delim = "\n");
    inline int wsgetline(std::istream &is, std::string &str, std::string delim = "\n") {
	is >> std::ws;
	return Core::getline(is, str, delim);
    }

    std::string& itoa(std::string &s, unsigned int val);
    inline std::string itoa(u32 i) {
	std::string s;
	return itoa(s, i);
    }

} // namespace Core

inline size_t __stl_hash_wstring(const wchar_t* __s) {
    unsigned long __h = 0;
    for ( ; *__s; ++__s)
	__h = 5*__h + *__s;

    return size_t(__h);
}

namespace Core {

    /** Generic unary functor for type conversion. */
    template <typename S, typename T>
    struct conversion :
	public std::unary_function<S, T>
    {
	T operator() (S s) const {
	    return T(s);
	}
    };

    /** A helper for conveniently assigning the two values from a pair
     * into separate variables. The idea for this comes from Jaakko
     * J�rvi's Binder/Lambda Library.  Code stolen from Boost, to
     * which it was constributed by Jeremy Siek */

    template <class A, class B>
    class tied {
    public:
	inline tied(A &a, B &b) : a_(a), b_(b) { }
	template <class U, class V>
	inline tied& operator=(const std::pair<U,V> &p) {
	    a_ = p.first;
	    b_ = p.second;
	    return *this;
	}
    protected:
	A &a_;
	B &b_;
    };

    template <class A, class B>
    inline  tied<A,B> tie(A &a, B &b) { return tied<A,B>(a, b); }

    /** Core::abs : wrapper for several absolute value functions */

    inline int abs(int v) { return std::abs(v); }
    inline long int abs(long int v) { return std::labs(v); }
    inline long long int abs(long long int v) { return std::llabs(v); }
    inline float abs(float v) { return std::fabs(v); }
    inline double abs(double v) { return std::fabs(v); }
    inline long double abs(long double v) { return std::fabs(v); }
    template<class T> T abs(const std::complex<T> &v) { return std::abs(v); }

    /** absoluteValue: functor for absolute value */
    template<class T> struct absoluteValue : public std::unary_function<T, T> {
	T operator() (T v) const { return Core::abs(v); }
	T operator() (const std::complex<T> &v) const { return abs(v); }
    };

    /**
     *  Maximal absolute value in @c v is returned in @c m.
     *  If @c v is empty, result is set to Type<T>::min.
     */
    template<class InputIterator, class M>
    void maxAbsoluteElement(InputIterator begin, InputIterator end, M &m) {
	m = Type<M>::min;
	for(; begin != end; ++ begin) {
	    M absValue = (M)Core::abs(*begin);
	    if (absValue > m)
		m = absValue;
	}
    }

    template<class T>
    T maxAbsoluteElement(const std::vector<T> &v) {
	T m; maxAbsoluteElement(v.begin(), v.end(), m); return m;
    }

    template<class T>
    T maxAbsoluteElement(const std::vector<std::complex<T> > &v) {
	T m; maxAbsoluteElement(v.begin(), v.end(), m); return m;
    }

    /** power: functor for pow function */
    template<class T> struct power : public std::binary_function<T, T, T> {
	T operator() (T x, T y) const { return pow(x, y); }
    };

    /** min: functor for min function */
    template<class T> struct minimum : public std::binary_function<T, T, T> {
	T operator() (T x, T y) const { return std::min(x, y); }
    };

    /** max: functor for max function */
    template<class T> struct maximum : public std::binary_function<T, T, T> {
	T operator() (T x, T y) const { return std::max(x, y); }
    };

    /** Core::round : wrapper for several round functions */
    inline float round(float v) { return ::roundf(v); }
    inline double round(double v) { return ::round(v); }
//  inline long double round(long double v) { return ::roundl(v); }

    /** Core::ceil : wrapper for several ceil functions */
    inline float ceil(float v) { return ::ceilf(v); }
    inline double ceil(double v) { return ::ceil(v); }
//  inline long double ceil(long double v) { return ::ceill(v); }

    /** Core::floor : wrapper for several floor functions */
    inline float floor(float v) { return ::floorf(v); }
    inline double floor(double v) { return ::floor(v); }
//  inline long double ceil(long double v) { return ::floorl(v); }

    /**
     * @return is true if [begin..end) interval does not contain any "inf", "nan" etc. value.
     */
    template<class InputIterator>
    bool isNormal(InputIterator begin, InputIterator end) {
	for(; begin != end; ++ begin)
	    if (!std::isnormal(*begin)) return false;
	return true;
    }

    /**
     * @return is true if @param f is "inf" or "nan".
     */
    template<class F> bool isMalformed(F f) { return std::isinf(f) || std::isnan(f); }

    /**
     * Checks if @param x is infinite and clips it to the largest representable value.
     * @return is clipped value of @param x.
     */
    template<class T>
    T clip(T x)
    {
	require(!std::isnan(x));
	if (std::isinf(x))
	    x = (x > 0) ? Type<T>::max : Type<T>::min;
	return x;
    }

    /** Core::log : numerically robust wrapper for log functions */
    inline float log(float v) { require(!isMalformed(v)); return v >= (Type<float>::delta) ? ::log(v) : ::log(Type<float>::delta); }
    inline double log(double v) { require(!isMalformed(v)); return v >= (Type<double>::delta) ? ::log(v) : ::log(Type<double>::delta); }

    /**
     * @return is true if [begin..end) interval contains any malformed value (@see isMalformed(F f)).
     */
    template<class InputIterator>
    bool isMalformed(InputIterator begin, InputIterator end) {
	for(; begin != end; ++ begin)
	    if (isMalformed(*begin)) return true;
	return false;
    }

    /** Functor for f(g(x), h(y)) */
    template <class F, class G, class H>
    class composedBinaryFunction
	: public std::binary_function<typename G::argument_type,
				      typename H::argument_type,
				      typename F::result_type>
    {
    protected:
	F f_;
	G g_;
	H h_;
    public:
	composedBinaryFunction(const F &f, const G &g, const H &h) : f_(f), g_(g), h_(h) {}
	typename F::result_type
	operator()(const typename G::argument_type &x, const typename H::argument_type &y) const {
	    return f_(g_(x), h_(y));
	}
    };

    template <class F, class G, class H>
    inline composedBinaryFunction<F, G, H> composeBinaryFunction(const F &f, const G &g, const H &h)
    {
	return composedBinaryFunction<F, G, H>(f, g, h);
    }

    /**
     * Test for near-equality of floating point numbers.
     * Due to finite precision, the bit-wise test (a == b) is almost
     * always false.  isAlmostEqual() compares the relative difference
     * of a and b to the machine precision (epsilon) times the given
     * tolerance factor.
     *
     * Deprecation warning: For new code you should prefer
     * isAlmostEqualUlp.
     *
     * Remark:
     *   -A similar idea can be found under the name "chordal metric":
     *    chord(a, b) = |a - b| / (sqrt(1+a^2) * sqrt(1 + b^2)).
     *   -sorry for repeating the same implementation for each
     *    floating point type, but specialization to complex numbers
     *    seems to be a hard nut with templates.
     */
    inline bool isAlmostEqual(f32 a, f32 b, f32 tolerance = (f32)1) {
	require_(tolerance > (f32)0);
	f32 d = Core::abs(a - b);
	f32 e = (Core::abs(a) + Core::abs(b) + Type<f32>::delta) * Type<f32>::epsilon * tolerance;
	return (d < e);
    }
    inline bool isAlmostEqual(f64 a, f64 b, f64 tolerance = (f64)1) {
	require_(tolerance > (f64)0);
	f64 d = Core::abs(a - b);
	f64 e = (Core::abs(a) + Core::abs(b) + Type<f64>::delta) * Type<f64>::epsilon * tolerance;
	return (d < e);
    }

    inline bool isAlmostEqual(const std::complex<f32> &a, const std::complex<f32> &b,
			      const std::complex<f32> tolerance = std::complex<f32>((f32)1, (f32)1)) {
	return (isAlmostEqual(a.real(), b.real(), tolerance.real())  &&
		isAlmostEqual(a.imag(), b.imag(), tolerance.imag()));
    }
    inline bool isAlmostEqual(const std::complex<f64> &a, const std::complex<f64> &b,
			      const std::complex<f64> tolerance = std::complex<f64>((f64)1, (f64)1)) {
	return (isAlmostEqual(a.real(), b.real(), tolerance.real())  &&
		isAlmostEqual(a.imag(), b.imag(), tolerance.imag()));
    }

    inline bool isSignificantlyGreater(f32 a, f32 b, f32 tolerance = (f32)1) {
	return a > b && !isAlmostEqual(a, b, tolerance);
    }
    inline bool isSignificantlyGreater(f64 a, f64 b, f64 tolerance = (f64)1) {
	return a > b && !isAlmostEqual(a, b, tolerance);
    }

    inline bool isSignificantlyLess(f32 a, f32 b, f32 tolerance = (f32)1) {
	return a < b && !isAlmostEqual(a, b, tolerance);
    }
    inline bool isSignificantlyLess(f64 a, f64 b, f64 tolerance = (f64)1) {
	return a < b && !isAlmostEqual(a, b, tolerance);
    }

    /**
     * Difference between two floating-point values in units of least
     * precision (ULP).
     *
     * @return The number of distinct floating-point values between @c
     * a and @c b.  I.e. If @c b is the smallest value greater than a,
     * the return value is 1.
     */
    s32 differenceUlp(f32 a, f32 b);
    s64 differenceUlp(f64 a, f64 b);

    /**
     * Test for near-equality of floating point numbers with tolerance
     * given in unit of least precision.  Due to finite precision, the
     * bit-wise test (a == b) should not be use for floating point
     * values.  This function was taken from "Comparing floating point
     * numbers" by Bruce Dawson
     * [http://www.cygnus-software.com/papers/comparingfloats/comparingfloats.htm].
     * You will probably not be able to make sense of this code
     * without reading the article.
     *
     * @param tolerance allowed difference between @c a and @c b
     * measured in units of least precision (Ulp).  This can be
     * thought of as the number of different normalized floating point
     * numbers between @c a and @c b.
     *
     * This method is preferrable to the older isAlmostEqual(),
     * because it is faster and theoretically more sound.  So maybe
     * isAlmostEqual() should be removed some time.
     */
    inline bool isAlmostEqualUlp(f32 a, f32 b, s32 tolerance) {
	require_(tolerance > 0);
	require_(tolerance < 0x400000);
	return (differenceUlp(a, b) <= tolerance);
    }
    inline bool isAlmostEqualUlp(f64 a, f64 b, s64 tolerance) {
	require_(tolerance > 0);
	require_(tolerance < (s64(1) << 62));
	return (differenceUlp(a, b) <= tolerance);
    }
    inline bool isAlmostEqualUlp(f64 a, f64 b, s32 tolerance) {
	return isAlmostEqualUlp(a, b, s64(tolerance));
    }

    inline bool isSignificantlyLessUlp(f32 a, f32 b, s32 tolerance) {
	return a < b && !isAlmostEqualUlp(a, b, tolerance);
    }
    inline bool isSignificantlyLessUlp(f64 a, f64 b, s64 tolerance) {
	return a < b && !isAlmostEqualUlp(a, b, tolerance);
    }

    // returns the time between start and end in seconds
    inline double timeDiff(timeval &start, timeval &end){
	double result = 0.0;
	result = (end.tv_sec - start.tv_sec);
	result += (end.tv_usec - start.tv_usec) / 1000000.0;
	return result;
    }

} // namespace Core


/**
 * \todo This seems to be obsolete for GCC >= 3.3.4
 */
/*
#include <functional>
namespace std {
    template <class _Pair> struct select1st : public std::_Select1st<_Pair> {};
    template <class _Pair> struct select2nd : public std::_Select2nd<_Pair> {};
}
*/

/**
 * Note: constexpr exists since C++11. In C++11 with a strict compiler, float-expression
 * constants need to be declared with constexpr in addition to const. Example:
 *   static constexpr const f64 inf = 1e9;
 * So, we just always use constexpr for such cases, but for <C++11, we just make it an empty dummy.
 */
// Check for before-C++11.
#if __cplusplus <= 199711L
// Just ignore.
#define constexpr
#endif


#endif // _CORE_UTILITY_HH
