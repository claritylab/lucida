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
#ifndef _MATH_VECTOR_HH
#define _MATH_VECTOR_HH

#include <iostream>
#include <sstream>
#include <complex>
#include <vector>
#include <iterator>
#include <algorithm>
#include <iomanip>
#include <cmath>
//#include <stdiostream.h>
#include <Core/BinaryStream.hh>
#include <Core/Assertions.hh>
#include <Core/Types.hh>
#include <Core/Utility.hh>
#include <Core/ReferenceCounting.hh>
#include <Core/XmlStream.hh>

namespace Math {

    const double MINLOGEPS=11.513f;

    struct VectorRangeCheckOnPolicy {
	template<class T> void checkRange(const T &v, size_t n) {
	    require_(n>=0 && n<v.size());
	}
    };

    struct VectorRangeCheckOffPolicy {
	template<class T> void checkRange(const T &v, size_t n) {}
    };


    template<typename T, class P = VectorRangeCheckOffPolicy>
    class Vector:
	public std::vector<T>
    {
    public:
	using std::vector<T>::size;
	using std::vector<T>::resize;
	using std::vector<T>::begin;
	using std::vector<T>::end;

	Vector() : std::vector<T>() {};
	Vector(size_t n, const T& value) : std::vector<T>(n,value) {};
	explicit Vector(size_t n) : std::vector<T>(n) {};
	Vector(const Vector<T>& x) : std::vector<T>(x) {};
	Vector(const std::vector<T>& x) : std::vector<T>(x) {};
	template <typename S>
	Vector(const std::vector<S>& x) :
	    std::vector<T>(x.size())
	{
	    for (size_t i=0; i<size(); ++i) (*this)[i] = T(x[i]);
	}
#ifdef __STL_MEMBER_TEMPLATES
	template <class InputIterator>
	Vector(InputIterator first, InputIterator last) : std::vector<T>(first,last) {};
#else
	Vector(typename std::vector<T>::const_iterator first,
	       typename std::vector<T>::const_iterator last) : std::vector<T>(first,last) {};
#endif /* __STL_MEMBER_TEMPLATES */


	T& operator[] (size_t n){
	    P().checkRange(*this, n);
	    return *(begin() + n);
	}

	const T& operator[] (size_t n) const{
	    P().checkRange(*this, n);
	    return *(begin() + n);
	}

	/** Inner product of two vectors (dot product). */
	T operator* (const Vector<T>& r) const {
	    require_(r.size() == size());
	    T result = T();
	    for(u32 i=0; i < size(); ++i)
		result += (*this)[i] * r[i];
	    return result;
	}


	const Vector<T,P> operator* (const T value) const {
	    Vector<T,P> result = Vector((*this));
	    for  (u32 i=0; i < size(); ++i)
		result[i] = (*this)[i] * value;
	    return result;
	}
	Vector<T,P>& operator*= (const T weight){
	    for (u32 i=0; i < size(); ++i)
		(*this)[i] *= weight;
	    return (*this);
	}


	const Vector<T,P> operator+ (const Vector<T>&r) const {
	    require(r.size() == size());
	    Vector<T,P> result = Vector((*this));
	    for (u32 i=0; i < size(); ++i)
		result[i] = (*this)[i]+ r[i];
	    return result;
	}
	Vector<T,P>& operator+= (const Vector<T>&r){
	    require(r.size() == size());
	    for (u32 i=0; i < size(); ++i)
		(*this)[i] += r[i];
	    return (*this);
	}


	const Vector<T,P> operator+ (const T value) const {
	    Vector<T,P> result = Vector((*this));
	    for  (u32 i=0; i < size(); ++i)
		result[i] = (*this)[i] + value;
	    return result;
	}
	Vector<T,P>& operator+= (const T value){
	    for (u32 i=0; i < size(); ++i)
		(*this)[i] += value;
	    return (*this);
	}


	const Vector<T,P> operator- (const Vector<T>&r) const {
	    require(r.size() == size());
	    Vector<T,P> result = Vector((*this));
	    for (u32 i=0; i < size(); ++i)
		result[i] = (*this)[i] - r[i];
	    return result;
	}
	Vector<T,P>& operator-= (const Vector<T>&r){
	    require(r.size() == size());
	    for (u32 i=0; i < size(); ++i)
		(*this)[i] -= r[i];
	    return (*this);
	}


	const Vector<T,P> operator- (const T value) const {
	    Vector<T,P> result = Vector((*this));
	    for  (u32 i=0; i < size(); ++i)
		result[i] = (*this)[i] - value;
	    return result;
	}
	Vector<T,P>& operator-= (const T value){
	    for (u32 i=0; i < size(); ++i)
		(*this)[i] -= value;
	    return (*this);
	}


	const Vector<T,P> operator/ (const T value) const {
	    require(value != 0);
	    Vector<T,P> result = Vector((*this));
	    for  (u32 i=0; i < size(); ++i)
		result[i] = (*this)[i] / value;
	    return result;
	}
	Vector<T,P>& operator/= (const T value) {
	    require(value != 0);
	    for  (u32 i=0; i < size(); ++i)
		(*this)[i] /= value;
	    return (*this);
	}


	void squareVector(const Vector<T>&r){
	    resize(r.size());
	    for (u32 i=0; i< size(); i++)
		(*this)[i] = r[i] * r[i];
	}

	T logSum(){
	    /* ???????? LOG SUM NOT IMPLEMENTED*/
	    T sum = (*this)[0];
	    for (u32 i=1; i < size(); i++){
		T diff = sum - (*this)[i];
		if ( diff > MINLOGEPS) {}
		else if (diff < -MINLOGEPS)
		    sum = (*this)[i];
		else if (diff < 0)
		    sum = (*this)[i] + log(1.0 + exp(diff));
		else
		    sum += log(1.0 + exp(-diff));
	    }
	    return(sum);
	}

	void exponent(){
	    for (u32 i = 0; i < size(); ++ i)
		(*this)[i] = exp(((*this)[i]));
	}

	/**
	 *  For each element x = 1 / x.
	 */
	Vector<T, P> &takeReciprocal() {
	    for (size_t i = 0; i < size(); ++ i)
		(*this)[i] = (T)1 / (*this)[i];
	    return *this;
	}

	/**
	 *  For each element x = 1 / x.
	 *  Elements |x| < @c zeroTolerance are set to 1.
	 */
	Vector<T, P> &takeReciprocal(T zeroTolerance) {
	    for (size_t i = 0; i < size(); ++ i) {
		T &v((*this)[i]);
		if (Core::abs(v) < zeroTolerance)
		    v = 1;
		else
		    v = (T)1 / (*this)[i];
	    }
	    return *this;
	}

	/**
	 *  For each element x = sqrt(x).
	 */
	Vector<T, P> &takeSquareRoot() {
	    for (size_t i = 0; i < size(); ++ i)
		(*this)[i] = sqrt((*this)[i]);
	    return *this;
	}

	/**
	 *  For each element x = sqrt(x).
	 *  Elements |x| < @c zeroTolerance are set to 0.
	 */
	Vector<T, P> &takeSquareRoot(T zeroTolerance) {
	    for (size_t i = 0; i < size(); ++ i) {
		T &v((*this)[i]);
		if (Core::abs(v) < zeroTolerance)
		    v = 0;
		else
		    v = sqrt(v);
	    }
	    return *this;
	}

	void permuteElements(const std::vector<size_t> &permutation) {
	    require(size() == permutation.size());
	    Vector<T> tmp = *this;
	    for(size_t i = 0; i < size(); ++ i) {
		require_(permutation[i] < tmp.size());
		operator[](i) = tmp[permutation[i]];
	    }
	}

	void normalize(T norm = 1, f32 p = 2) {
	    T sum = 0;
	    for(size_t i = 0; i < size(); ++ i) {
		sum += pow(Core::abs(operator[](i)), p);
	    }
	    *this *= norm / pow(sum, 1 / p);
	}

	void print(std::ostream &os) const {
	    for (u32 i=0; i < size(); i++)
		os << std::setw(8) << std::setprecision(3)<< (*this)[i] << " ";
	    os << std::endl;
	}

	void print_raw(std::ostream &os) const {
	    for (u32 i=0; i < size(); i++)
		os << (*this)[i] << " ";
	    os << std::endl;
	}


	bool write(Core::BinaryOutputStream &o) const {
	    o << (u32)size();
	    std::copy(begin(), end(), Core::BinaryOutputStream::Iterator<T>(o));
	    return o.good();
	}

	bool read(Core::BinaryInputStream &i) {
	    u32 s; i >> s; resize(s);
	    for(typename std::vector<T>::iterator it = begin(); it != end(); it ++) i >> (*it);
	    return i.good();
	}

	bool read(std::istream &is){
	    u32 s; is >> s; resize(s);
	    for(typename std::vector<T>::iterator it = begin(); it != end(); it ++) is >> (*it);
	    return is.good();
	}

    };


    template<class T, class P>
    Vector<T,P> operator* (const T weight, const Vector<T,P>&v ){
	Vector<T,P> result = v;
	for (u32 i=0; i < v.size(); ++i)
	    result[i] *= weight;
	return result;
    }



    template<class T, class P>
    Core::BinaryOutputStream& operator<< (Core::BinaryOutputStream& o, const Vector<T,P> &v) {
	v.write(o); return o;
    }

    template<class T, class P>
    Core::BinaryInputStream& operator>> (Core::BinaryInputStream& i, Vector<T,P> &v) {
	v.read(i); return i;
    }



    template <typename T, class P> Vector<T,P> real(const Vector<std::complex<T>,P>& v)
    {
	Vector<T,P> vector(v.size());

	for (u32 i=0; i<v.size(); ++i)
		vector[i] = v[i].real();

	return vector;
    }

    template <typename T, class P>
    Vector<T,P> imag(const Vector<std::complex<T>,P>& v) {
	Vector<T,P> vector(v.size());
	for (u32 i=0; i<v.size(); ++i)
	    vector[i] = v[i].imag();
	return vector;
    }


    template <typename V>
    std::istream& operator>>(std::istream &is, Vector<V> &v) {
	v.clear();
	std::copy(std::istream_iterator<V>(is), std::istream_iterator<V>(), std::back_inserter(v));

	return is;
    }

    template <typename T, class P>
    Core::XmlWriter &operator<<(Core::XmlWriter &os, const Vector<T,P> &v) {
	os << Core::XmlOpen(std::string("vector-") + Core::Type<T>::name)
	    + Core::XmlAttribute("size", v.size());

	os << setiosflags (std::ios::scientific);
	v.print_raw(os);

	os << Core::XmlClose(std::string("vector-") + Core::Type<T>::name);

	return os;
    }


    template<typename T, class P = VectorRangeCheckOffPolicy>
    class ReferenceCountedVector:
	public Core::ReferenceCounted, public Vector<T,P>
    {
    public:
	typedef Vector<T,P> Base;
	using Base::size;

	template <typename S>
	ReferenceCountedVector(const std::vector<S>& x) :
	    Vector<T,P>(x.size())
	{
	    for (size_t i=0; i<size(); ++i) (*this)[i]=T(x[i]);
	}
	ReferenceCountedVector() : Vector<T,P>() {};
	ReferenceCountedVector(size_t n, const T& value) : Vector<T,P>(n,value) {};
	explicit ReferenceCountedVector(size_t n) : Vector<T,P>(n) {};
	ReferenceCountedVector(const ReferenceCountedVector<T>& x) : Vector<T,P>(x) {};
	ReferenceCountedVector(const Vector<T,P>& x) : Vector<T,P>(x) {};
#ifdef __STL_MEMBER_TEMPLATES
	template <class InputIterator>
	ReferenceCountedVector(InputIterator first, InputIterator last) : Vector<T,P>(first,last) {};
#else
	ReferenceCountedVector(typename Base::const_iterator first, typename Base::const_iterator last) : Vector<T,P>(first,last) {};
#endif /* __STL_MEMBER_TEMPLATES */
    };
}

namespace Core {
    template<class T, class P>
    class NameHelper<Math::Vector<T, P> > : public std::string {
    public:
	NameHelper() : std::string(std::string("vector-") + NameHelper<T>()) {}
    };
}

#endif // _MATH_VECTOR_HH
