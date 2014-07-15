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
#ifndef _MATH_FASTVECTOR_HH_
#define _MATH_FASTVECTOR_HH_

#include <cmath>
#include <algorithm>
#include <functional>
#include <numeric>

#include <Core/Types.hh>
#include <Math/Blas.hh>

#include <iostream>		/** to use std::cout */
#include <Core/Assertions.hh>	/** to use require() */
#include <Math/Vector.hh>	/** for convert functions */

#include <Math/FastMatrix.hh>
#include <Math/Utilities.hh>    // for isnan()

namespace Math {

template<typename T>
class FastMatrix;

/*
 * fast vector implementation
 * vectors are assumed to be column vectors
 * note: BLAS interface assumes int as size type, therefore size is limited by Core::Type<int>::max
 */

template<typename T>
class FastVector {
    friend class FastVector<f32>;
    friend class FastVector<f64>;
protected:
    u32 nRows_;
    T *elem_;
public:
    FastVector(u32 nRows = 0);			// constructor with memory allocation

    FastVector(const FastVector<T> &vector);	// (deep) copy constructor

    // destructor
    virtual ~FastVector() { clear(); }
public:
    u32 nRows() const { return nRows_; }
    u32 nColumns() const { return 1; }
    u32 size() const { return (*this).nRows(); }
    bool empty() const { return ((*this).size() == 0); }

public:
    // resize & allocate
    // side effect: after resize content is meaningless
    void resize(u32 newSize, T value=0, bool allocOnly=false);
    void clear() { resize(0); }
public:
    // copy
    template<typename S>
    void copy(const FastVector<S>& vector);

    // copy matrix from conventional sprint Vector
    template<typename S>
    void copy(const Vector<S> &vector);

    // convert to Math::Vector
    template<typename S>
    void convert(Math::Vector<S> &vector) const;

    void copyStructure(const FastVector<T> &vector);

public:		// iterators
    typedef T* iterator;
    typedef const T* const_iterator;
    iterator begin() { return elem_; }
    const_iterator begin() const { return elem_; }
    iterator end() { return &elem_[nRows_]; }
    const_iterator end() const { return &elem_[nRows_]; }

public:
    bool isFinite() const;

public:
    // addition of a vector (scaling of the vector possible)
    template<typename S>
    void add(const FastVector<S> &vector, S scale = 1) {
	Math::axpy<S,T>((*this).size(), (T) scale, vector.begin(), 1, (*this).begin(), 1);
    }

    // add a constant to each element of the vector
    void addConstantElementwise(T c) { std::transform(begin(), end(), begin(), std::bind2nd(std::plus<T>(), c)); }

    // scaling of the vector
    void scale(T value) { Math::scal<T>(nRows_, value, elem_, 1); }

    // @return sum of squared matrix entries
    T sumOfSquares() const { return dot (*this); }

    // vector dot product (result = this^T * v)
    T dot(const FastVector<T>& vector) const;

    // set i-th component of vector to inner product of i-th column of A and i-th column of B
    void columnwiseInnerProduct(const FastMatrix<T>& A, const FastMatrix<T>& B);

    // multiply corresponding elements (this = this .* v)
    void elementwiseMultiplication(const FastVector<T>& v);

    // divide corresponding elements (this = this ./ v)
    void elementwiseDivision(const FastVector<T>& v);

    // division by a constant
    void divide(T value) { scale((T) 1 / value); }

    // set all elements to a constant value (e.g. zero)
    void setToZero();
    void fill(T value);

    // set all values < threshold to threshold
    void ensureMinimalValue(const T threshold);

    FastVector<T> &operator=(FastVector<T> rhs);
    // vector access
    T& operator() (u32 index) { return elem_[index]; }
    T& operator[] (u32 index) { return (*this)(index); }
    T& operator() (u32 index) const { return elem_[index]; }
    T& operator[] (u32 index) const { return (*this)(index); }
    T& at(u32 index);
    const T& at(u32 index) const;

    // l1-norm of vector
    T asum() const { return Math::asum<T>((*this).size(), (*this).begin(), 1); }
    // just an alias
    T l1norm() const { return asum(); }

    // *this = (*this) + scale * matrixRowSum
    template<typename S>
    void addSummedColumns(const FastMatrix<S>& matrix, const S scale = 1.0);

    // like addSummedColumns, but squares each matrix entry before summation
    void addSquaredSummedColumns(const FastMatrix<T>& matrix, const T scale = 1.0);

    // *this = (*this) + scale * matrixColumnSum
    void addSummedRows(const FastMatrix<T>& matrix, const T scale = 1.0);

    void getMaxOfColumns(const FastMatrix<T> &X);

    void l1clipping(const T value);

    // euclidean norm => ?nrm2 s, d, sc, dz Vector 2-norm (Euclidean norm) a normal
    T normEuclidean() const { return Math::nrm2<T>((*this).size(), (*this).begin(), 1); }

    // swap two vectors
    void swap(FastVector<T>& vector);

    void show() const;

    template<typename S>
    void cast(const Math::Vector<S>& vector);

};

// ----------------------------------------------------------------------------
//		Vector constructors
// ----------------------------------------------------------------------------
template<typename T>
FastVector<T>::FastVector(u32 size) :
nRows_(0),
elem_(0) {
    resize(size, 0, true);
}

/**	Copy constructor
 */
template<typename T>
FastVector<T>::FastVector(const FastVector<T> &vector) :
  nRows_(0),
  elem_(0)
{
    // (deep) copy structure
    resize(vector.size(), 0, true);

    // copy all elements of the vector
    copy(vector);
}

// ----------------------------------------------------------------------------
//		Vector resize
// ----------------------------------------------------------------------------
template<typename T>
void FastVector<T>::resize(u32 newSize, T value, bool allocOnly) {
    u32 oldSize = nRows_;
    nRows_ = newSize;
    if (newSize == oldSize)
	return;
    else if (newSize == 0) { // free memory
	if (elem_) {
	    delete [] elem_;
	}
	elem_ = 0;
    } else {
	// allocate new array
	T* newElem = 0;
	try {
	    newElem = new T[newSize];
	}
	catch (std::bad_alloc& ba) {
	    Core::Application::us()->criticalError("failed to allocate memory for vector of size ") << nRows_;
	}
	if (!allocOnly){
	    // ... copy old elements and free old memory
	    u32 nElements = std::min(oldSize, newSize);
	    std::copy(elem_, elem_ + nElements, newElem);
	    if (elem_)
		delete [] elem_;
	    // initialize new memory
	    if (newSize > oldSize)
		std::fill(newElem + oldSize, newElem + newSize, value);
	}
	elem_ = newElem;


    }
}

/**
 * Copy the vector.
 */
template<typename T>
template<typename S>
void FastVector<T>::copy(const FastVector<S>& vector) {
    require(nRows_ == vector.nRows());
    Math::copy<S,T>(nRows_, vector.elem_, 1, elem_, 1);
}

template<typename T>
template<typename S>
void FastVector<T>::copy(const Vector<S> &vector){
    resize(vector.size(), 0, true);
    if (nRows_ > 0)
	Math::copy<S,T>(nRows_, &vector.at(0), 1, elem_, 1);
}


template<typename T>
template<typename S>
void FastVector<T>::convert(Math::Vector<S> &vector) const {
    vector.resize(nRows_);
    if (nRows_ > 0)
	Math::copy<S,T>(nRows_, elem_, 1, &vector.at(0), 1);
}


template<typename T>
void FastVector<T>::copyStructure(const FastVector<T>& vector) {
    resize(vector.nRows(), 0, true);
}


// ----------------------------------------------------------------------------
//		Vector check - verify the data in the vector (inf or nan)
// ----------------------------------------------------------------------------
template<typename T>
bool FastVector<T>::isFinite() const {
    for (u32 index = 0; index < nRows_; ++index) {
	T val = at(index);
	if (Math::isnan(val) || val > Core::Type<T>::max || val < Core::Type<T>::min)
	    return false;
    }
    return true;
}

template<typename T>
T& FastVector<T>::at(u32 index) {
    require_lt(index, nRows_);
    return (*this)(index);
}

template<typename T>
const T& FastVector<T>::at(u32 index) const {
    require_lt(index, nRows_);
    return elem_[index];
}

template<typename T>
T FastVector<T>::dot(const Math::FastVector<T> &v) const {
    require_eq(nRows_, v.nRows());
    return Math::dot(nRows_, elem_, 1, v.elem_, 1);
}

template<typename T>
void FastVector<T>::columnwiseInnerProduct(const Math::FastMatrix<T>& A, const Math::FastMatrix<T>& B) {
    require_eq(A.nRows(), B.nRows());
    require_eq(A.nColumns(), B.nColumns());
    require_eq(nRows_, A.nColumns());
    u32 matrixRows = A.nRows();
    // TODO: for now only parallelized within the columns, implement a better parallelization
    for (u32 column = 0; column < A.nColumns(); column++) {
	at(column) = Math::dot(matrixRows, A.begin() + column * matrixRows, 1, B.begin() + column * matrixRows, 1);
    }
}

template<typename T>
void FastVector<T>::elementwiseMultiplication(const FastVector<T>& v) {
    require_eq(nRows_, v.nRows());
    std::transform(begin(), end(), v.begin(), begin(), std::multiplies<T>());
}

template<typename T>
void FastVector<T>::elementwiseDivision(const FastVector<T>& v) {
    require_eq(nRows_, v.nRows());
    std::transform(begin(), end(), v.begin(), begin(), std::divides<T>());
}

/**	Set all elements to 0.
 *
 *	Set all elements in the vector to 0.
 */
template<typename T>
void FastVector<T>::setToZero() {
	std::memset(elem_, 0, nRows_ * sizeof(T));
}

/**	Set all elements to a constant value.
 *
 *	Set all elements in the vector to a constant value.
 */
template<typename T>
void FastVector<T>::fill(const T value) {
	//fill the array with the constant
	std::fill(begin(), end(), value);
}

/**
 *	Set all elements to be >= threshold
 */
template<typename T>
void FastVector<T>::ensureMinimalValue(const T threshold) {
    for (u32 row = 0; row < nRows_; row++) {
	if (at(row) < threshold)
	    at(row) = threshold;
    }
}

template<typename T>
FastVector<T> &FastVector<T>::operator=(FastVector<T> rhs) {
    swap(rhs);
    return *this;
}

template<typename T>
template<typename S>
void FastVector<T>::addSummedColumns(const FastMatrix<S>& matrix, const S scale) {
    require_eq(matrix.nRows(), nRows_);
    // TODO parallelize
    for (u32 row = 0; row < matrix.nRows(); row++) {
	for (u32 column = 0; column < matrix.nColumns(); column++)
	    at(row) += scale * matrix.at(row, column);
    }
}

template<typename T>
void FastVector<T>::addSquaredSummedColumns(const FastMatrix<T>& matrix, const T scale) {
    require_eq(matrix.nRows(), nRows_);
    // TODO parallelize
    for (u32 row = 0; row < matrix.nRows(); row++) {
	for (u32 column = 0; column < matrix.nColumns(); column++)
	    at(row) += scale * matrix.at(row, column) * matrix.at(row, column);
    }
}

template<typename T>
void FastVector<T>::addSummedRows(const FastMatrix<T>& matrix, const T scale) {
    require_eq(matrix.nColumns(), nRows_);
    // TODO parallelize
    for (u32 column = 0; column < matrix.nColumns(); column++) {
	for (u32 row = 0; row < matrix.nRows(); row++) {
	    at(column) += scale * matrix.at(row, column);
	}
    }
}

template<typename T>
void FastVector<T>::getMaxOfColumns(const FastMatrix<T> &X){
    // TODO parallelize
    require_eq(X.nColumns(), nRows_);
    for (u32 j = 0; j < X.nColumns(); j++)
	at(j) = *std::max_element(&X.at(0,j), &X.at(0,j) + X.nRows());
}

template<typename T>
void FastVector<T>::l1clipping(const T value) {
    for (u32 row = 0; row < nRows_; row++) {
	if (at(row) > 0) {
	    at(row) = std::max((T)0.0, at(row) - value);
	}
	else if (at(row) < 0) {
	    at(row) = std::min((T)0.0, at(row) + value);
	}
    }
}

template<typename T>
void FastVector<T>::swap(FastVector<T> &vector){
    std::swap(nRows_, vector.nRows_);
    std::swap(elem_, vector.elem_);
}

template<typename T>
void FastVector<T>::show() const {
    for (u32 j = 0; j < nRows_; j++)
	std::cout << at(j) << std::endl;
}

// ----------------------------------------------------------------------------
//		Cast functions
// ----------------------------------------------------------------------------
template<typename T> template<typename S>
void FastVector<T>::cast(const Math::Vector<S>& vector) {
	// resize the current vector
	resize(vector.size(), 0, true);

	// cast/copy the elements
	for (u32 index = 0; index < vector.size(); ++index) {
		(*this)(index) = (T) vector[index];
	}
}

} // namespace Math

#endif /* _MATH_FASTVECTOR_HH_ */
