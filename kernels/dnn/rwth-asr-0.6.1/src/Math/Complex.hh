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
#ifndef _MATH_COMPLEX_HH
#define _MATH_COMPLEX_HH

#include <Core/BinaryStream.hh>
#include <Core/Utility.hh>
#include <complex>
#include <functional>

namespace Math {

    /** functor for the template constructor std::complex<T>::complex */
    template<class T>
    struct makeComplex : public std::binary_function<T, T, const std::complex<T> > {
	std::complex<T> operator()(T real, T imaginary) const { return std::complex<T>(real, imaginary); }
    };


    /** function pointer functor to the template function abs(const std::complex<T> &) */
    template<class T> struct pointerAbs : public std::unary_function<const std::complex<T>, T> {
	T operator() (const std::complex<T> &v) const { return Core::abs(v); }
    };


    /** function pointer functor to the template function arg(const std::complex<T> &) */
    template<class T> struct pointerArg : public std::unary_function<const std::complex<T>, T> {
	T operator() (const std::complex<T> &v) const { return arg(v); }
    };


    /** function pointer functor to the template function real(const std::complex<T> &) */
    template<class T> struct pointerReal : public std::unary_function<const std::complex<T>, T> {
	T operator() (const std::complex<T> &v) const { return real(v); }
    };

    /** function pointer functor to the template function imag(const std::complex<T> &) */
    template<class T> struct pointerImag : public std::unary_function<const std::complex<T>, T> {
	T operator() (const std::complex<T> &v) const { return imag(v); }
    };

    /** tranforms alternating complex vector into an arbitrary type
     * (Alternating complex vectors are stored in a standard container
     * by storing the real and the imaginary parts alternating.)
     *
     *	Alternating complex numbers are first transformed to a complex<InputType> object.
     *  Operation is transforms the complex<InputType> object:
     *     operation must support the function: OutputType operator()(const complex<InputType> &).
     * Operation is a unary function (derived from std::unary_function):
     *  Argument must have a constructor with two parameters
     *  Result is of arbitrary type
     */
    template<class InputIterator, class OutputIterator, class Operation>
    void transformAlternatingComplex(InputIterator first, InputIterator last,
				     OutputIterator result, Operation operation) {
	for(InputIterator real = first, imaginary = first + 1;
	    real != last; ++ result) {

	    *result = operation(typename Operation::argument_type(*real, *imaginary));
	    real += 2;
	    if (real != last) imaginary += 2;
	}
    }

    /** tranforms a vector of arbitrary type into an alternating complex vector
     * (Alternating complex vectors are stored in a standard container
     * by storing the real and the imaginary parts alternating.)
     * Operation is a unary function (derived from std::unary_function):
     *  Argument is of arbitrary type
     *  For the result type the functions real and imag must exist.
     *
     */
    template<class InputIterator, class OutputIterator, class Operation>
    void transformToAlternatingComplex(InputIterator first, InputIterator last,
				       OutputIterator result, Operation operation) {

	for(; first != last; result += 2, ++ first) {

	    typename Operation::result_type r = operation(*first);

	    *result = real(r);
	    *(result + 1) = imag(r);
	}
    }

    /** tranforms an alternating complex vector into an alternating complex vector
     * (Alternating complex vectors are stored in a standard container
     * by storing the real and the imaginary parts alternating.)
     *
     * Operation is a binary function derived from std::binary_function:
     *  Arguments must have a constructor with two parameters
     *  For the result type the functions real and imag must exist.
     */
    template<class InputIterator, class OutputIterator, class Operation>
    void transformAlternatingComplexToAlternatingComplex(InputIterator first1, InputIterator last1,
							 InputIterator first2, OutputIterator result,
							 Operation operation) {

	for(InputIterator real1 = first1, imaginary1 = first1 + 1,
		real2 = first2, imaginary2 = first2 + 1;
	    real1 != last1;
	    result += 2, real1 += 2, imaginary1 += 2, real2 += 2, imaginary2 += 2) {

	    typename Operation::result_type r = operation(
		typename Operation::first_argument_type(*real1, *imaginary1),
		typename Operation::second_argument_type(*real2, *imaginary2));

	    *result = real(r);
	    *(result + 1) = imag(r);
	}
    }

    /** functors for x * conjugate(y) */
    template<class T> struct conjugateMultiplies :
	public std::binary_function<std::complex<T>, std::complex<T>, std::complex<T> >{

	std::complex<T> operator()(const std::complex<T> &x, const std::complex<T> &y) const {
	    return x * conj(y);
	}
    };
}

namespace Core {

    /** binary output for std::complex<T> */
    template<class T>
    Core::BinaryOutputStream& operator<<(Core::BinaryOutputStream& o, const std::complex<T> &c) {
	return o << real(c) << imag(c);
    }

    /** binary input for std::complex<T> */
    template<class T>
    Core::BinaryInputStream& operator>>(Core::BinaryInputStream& i, std::complex<T> &c) {
	T re, im;
	i >> re >> im;
	c = std::complex<T>(re, im);
	return i;
    }
}

#endif // _MATH_COMPLEX_HH
