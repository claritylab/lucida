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
#ifndef FASTMATRIX_HH_
#define FASTMATRIX_HH_

#include <Core/Assertions.hh>			// to use different require() functions
#include <Core/Application.hh>
#include <Core/OpenMPWrapper.hh>

#include <Math/Blas.hh>
#include <Math/Matrix.hh>			// for convert functions
#include <Math/FastVector.hh>   		// for matrix-vector operations (Blas 2)
#include <Math/FastVectorOperations.hh>
#include <Math/Nr/Random.hh>			// random number generator
#include <Math/Utilities.hh>			// for isnan()

#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <functional>
#include <limits>
#include <iostream>				// to use std::cout
#include <typeinfo>
#include <sstream>

namespace Math {

template<typename T>
class FastVector;

/*
 * matrix with col-major storage
 * use of BLAS routines
 *
 */

template<typename T>
class FastMatrix {
    friend class FastMatrix<f64>;
    friend class FastMatrix<f32>;
protected:
    u32 nRows_;
    u32 nColumns_;
    T *elem_;
    u32 capacity_;
protected:
    static bool initialized;
    static s32 maxThreads;
    static s32 initialize();
    s32 nThreads_;
public:
    s32 getNumberOfThreads(){ return nThreads_; }
public:
    // iterators
    typedef T* iterator;
    typedef const T* const_iterator;
    iterator begin() { return elem_; }
    const_iterator begin() const { return elem_; }
    iterator end() { return &(elem_[nRows_ * nColumns_]); }
    const_iterator end() const { return &(elem_[nRows_ * nColumns_]); }
public:
    // constructor with memory allocation
    FastMatrix(u32 nRows = 0, u32 nColumns = 0);

    // (deep) copy constructor
    FastMatrix(const FastMatrix<T> &X);

    // constructor for creating sub-matrices via copyBlockFromMatrix()
    FastMatrix(const FastMatrix<T> &X, u32 rowIndexX, u32 colIndexX,
	u32 thisRowIndex, u32 thisColIndex, u32 nRows, u32 nCols);

    // destructor
    virtual ~FastMatrix();

    T& operator() (s32 row, s32 column) { return at(row, column); }

private:
    bool allocate(bool minimalSize=false);

public:
    // free memory
    void clear();

    // required for assignment operator
    void swap(FastMatrix<T> &X);

    void setNumberOfThreads(int nThreads) { nThreads_ = nThreads;};

    // need assignment operator, because we have a copy constructor
    // pass by value ! (needed for temporary object creation)
    FastMatrix<T> & operator=(FastMatrix<T> X);

    // resize & allocate
    // side effect: after resize content is meaningless
    // minimalSize: allocate smaller array if large array is not needed
    void resize(u32 nRows, u32 nColumns, bool minimalSize=false);

    // set dimensions to those of X and allocate
    template <typename S>
    void copyStructure(const FastMatrix<S> &X);

    // returns the number of rows
    u32 nRows() const { return nRows_; }

    // copy method
    // this = X
    // for matrices with same dimensions
    template<typename S>
    void copy(const FastMatrix<S> &X);

    // copy method
    // this = X
    // array X is assumed to be of size nRows_ * nColumns
    template<typename S>
    void copy(const S *X, u32 rowOffset = 0, u32 colOffset = 0);

    // copy matrix from conventional (slow) sprint matrices
    template<typename S>
    void copy(const Matrix<S> &matrix);

    // copy from std::vector
    template<typename S>
    void copy(const std::vector<S> &X, u32 rowOffset = 0, u32 colOffset = 0);

    // convert matrix to conventional (slow) sprint matrices
    template<typename S>
    void convert(Matrix<S> &matrix) const;

    // returns the number of columns
    u32 nColumns() const { return nColumns_; }

    // returns whether the matrix is empty
    bool empty() const { return nRows() == 0 || nColumns() == 0; }

    // returns whether all matrix entries are finite
    bool isFinite() const;

    // returns the total number of entries
    u32 size() const { return nRows_*nColumns_; }

    // fills the matrix with the given value
    void fill(T value) { std::fill(elem_, elem_ + nRows_ * nColumns_, value); } // TODO parallelize

    // set all values < threshold to threshold
    void ensureMinimalValue(const T threshold);

    // get reference to element in row i, column j
    T& at(u32 i, u32 j){
	require_lt(i,nRows_);
	require_lt(j,nColumns_);
	return *(elem_ + j*nRows_ + i);
    }

    // get const reference to element in row i, column j
    const T& at(u32 i, u32 j) const {
	require_lt(i, nRows_);
	require_lt(j, nColumns_);
	return *(elem_ + j*nRows_ + i);
    }

    // print matrix to std::cout
    void show() const;

    // get row with index rowIndex
    void getRow(u32 rowIndex, Math::FastVector<T> &row) const;

    // get column with index columnIndex
    void getColumn(u32 columnIndex, Math::FastVector<T> &column) const;

    // copy block from matrix to specific position
    void copyBlockFromMatrix(const Math::FastMatrix<T> &X, u32 rowIndexX, u32 colIndexX, u32 thisRowIndex, u32 thisColIndex, u32 nRows, u32 nCols = 1);

    // this = 0
    void setToZero() {	std::memset(elem_, 0, nRows_ * nColumns_ * sizeof(T)); }



public:

    /*
     * MATH OPERATIONS
     *
     */

    /*
     * Blas1-like methods
     */

    // this += alpha * X
    template<typename S>
    void add(const FastMatrix<S> &X, S alpha = 1.0);

    // @return l1-norm of matrix
    T l1norm() const { return Math::mt_asum(nRows_ * nColumns_, elem_, nThreads_);}

    // @return sum of squared matrix entries
    T sumOfSquares() const { return dot (*this); }

    // dot product
    // return this' * X
    // for matrices with multiple columns: interpret matrix as vector
    T dot(const FastMatrix<T> &X) const;

    // scale elements
    // this *= alpha
    void scale(T alpha);

    /*
     * Blas2-like methods
     */

    // matrix vector product
    // Y := alpha * this * X + beta * Y,   or   Y := alpha * this**T * X + beta * Y
    void multiply(const FastVector<T> &x, FastVector<T> &y,
	    bool transposed = false, T alpha = 1.0, T beta = 0.0, u32 lda = 0) const;

    // rank-1 update: this += alpha * x y^T
    void addOuterProduct(const FastVector<T>& x, const FastVector<T> &y, T alpha, u32 lda = 0);

    /*
     * Blas3-like methods
     */

    // (*this) = (scaleA * matrixA) * matrixB + scaleC * (*this)
    template<typename S>
    void addMatrixProduct(const FastMatrix<S> &matrixA, const FastMatrix<S> &matrixB,
	    T scaleC = 0, S scaleA = 1, bool transposeA = false, bool transposeB = false);

    /*
     * special methods required for neural network computations
     */

    // apply sigmoid function to each element of matrix
    // this = 1.0 / (1.0 + exp(-gamma * this))
    void sigmoid(T gamma = 1.0);

    // apply softmax to each column of matrix
    void softmax();

    // this = this .* (X .* (1 - X))
    void elementwiseMultiplicationWithSigmoidDerivative(const FastMatrix<T> &X);

    // this = this .* (1 - X .^ 2)
    void elementwiseMultiplicationWithTanhDerivative(const FastMatrix<T> &X);

    // for each column i: this(_,i) = diag(softmax(_,i) - softmax(_,i)*softmax(_,i)^T) * this(_,i)
    void multiplicationWithSoftmaxDerivative(const FastMatrix<T> &softmax);

    // this = this .* sign(X)
    void elementwiseMultiplicationWithRectifiedDerivative(const FastMatrix<T> &X);

    // add kronecker delta (based on alignment) to each column of the matrix
    template<typename S>
    void addKroneckerDelta(const FastVector<S>& alignment, const T scale = 1.0);

    // return number of classifications errors; each column of *this is interpreted as a probability distribution
    template<typename S>
    u32 nClassificationErrors(const FastVector<S>& alignment);

    // return the value of the cross entropy objective function; each column of *this is interpreted as a probability distribution
    template<typename S>
    T crossEntropyObjectiveFunction(const FastVector<S>& alignment);

    // return the value of the weighted cross entropy objective function; each column of *this is interpreted as a probability distribution
    template<typename S>
    T weightedCrossEntropyObjectiveFunction(const FastVector<S>& alignment, const FastVector<T>& weights);

    // return the value of the squared error objective function
    template<typename S>
    T squaredErrorObjectiveFunction(const FastVector<S>& alignment);

    // return the value of the weighted squared error objective function
    template<typename S>
    T weightedSquaredErrorObjectiveFunction(const FastVector<S>& alignment, const FastVector<T>& weights);

    // return the value of the binary divergence objective function
    template<typename S>
    T binaryDivergenceObjectiveFunction(const FastVector<S>& alignment);

    // return the value of the weighted binary divergence objective function
    template<typename S>
    T weightedBinaryDivergenceObjectiveFunction(const FastVector<S>& alignment, const FastVector<T>& weights);

    template<typename S>
    void binaryDivergenceSoftmaxGradient(const FastMatrix<T> &Y, const FastVector<S> &A);

    // dot product
    T dotWithColumn(const FastMatrix<T> &X, u32 thisColumnIndex) const;

    // expand by second order polynomial features (column-wise
    void setToSecondOrderFeatures(const FastMatrix<T> &X);

    // expand by second and third order polynomial features (column-wise
    void setToThirdOrderFeatures(const FastMatrix<T> &X);

    // apply dropout to matrix, each element is dropped with probability dropoutProbability
    void dropout(const T dropoutProbability);

    // add gaussian noise with given standard deviation to matrix
    void addGaussianNoise(const T standardDeviation);

    // l1 regularization with clipping (see tsuruoka et. al.,
    // stochastic gradient descent training for L1-regularized log-linear models with cumulative penalty)
    void l1clipping(const T value);

    /*
     * more math operations
     */

    // apply tanh to each element of matrix
    void tanh();

    // apply exp to each element of matrix
    void exp();

    // apply log to each element of matrix
    void log();

    // return index of maximum absolute value in column
    u32 argAbsMax(u32 column) const;

    // return index of maximum absolute value in column
    u32 argMax(u32 column) const;

    // this = this .* X
    void elementwiseMultiplication(const FastMatrix<T> &X);

    // this = this ./ X
    void elementwiseDivision(const FastMatrix<T> &X);

    // add constant value c to each element
    void addConstantElementwise(T c);

    // add vector (scaled by alpha) to each column of the matrix
    void addToAllColumns(const FastVector<T> &v, T alpha = 1.0);

    // add vector (scaled by alpha) to each column of the matrix
    void addToAllRows(const FastVector<T> &v, T alpha = 1.0);

    // for each i: multiply column i by scalars[i]
    void multiplyColumnsByScalars(const FastVector<T> &scalars);

    // for each i: divide column i by scalars[i]
    void divideColumnsByScalars(const FastVector<T> &scalars);

    // for each i: multiply row i by scalars[i]
    void multiplyRowsByScalars(const FastVector<T> &scalars);

    // for each i: multiply row i by scalars[i]
    void divideRowsByScalars(const FastVector<T> &scalars);

private:
    // C = (scaleA * matrixA) * matrixB + scaleC * (*this)
    template<typename S>
    static void _gemm(bool transposedA, bool transposedB, int M, int N, int K,
	    S scaleA, const S* matrixA, int lda, const S* matrixB, int ldb,
	    T scaleC, T* matrixC, int ldc);

    void appendSecondOrderFeatures(const FastMatrix<T> &X, u32 offset);

    void appendThirdOrderFeatures(const FastMatrix<T> &X, u32 offset);
};

template<typename T>
bool FastMatrix<T>::initialized = false;

template<typename T>
s32 FastMatrix<T>::maxThreads = 1;

template<typename T>
s32 FastMatrix<T>::initialize(){
    if (!initialized){
	initialized = true;

	int value;

	char* svalue;
	svalue = std::getenv("OMP_NUM_THREADS");
	if (svalue != NULL){
	    std::string tmp = svalue;
	    std::istringstream ss(tmp);
	    ss >> value;
	    if (ss.fail())
		value = 1;
	}
	else{
	    value = 1;
	}

	maxThreads = value;
	Core::omp::set_num_threads(value);
	if (Core::Application::us())
	    Core::Application::us()->log("Maximum number of threads for CPU matrix operations: ") << maxThreads;
	else
	    std::cerr << "Maximum number of threads for CPU matrix operations: " << maxThreads << std::endl;

    }
    return maxThreads;
}

/**	Allocate the memory for the matrix.
 *
 * 	Allocate the memory for the matrix. If the size is 0 the pointer
 * 	is ZERO.
 */
template<typename T>
bool FastMatrix<T>::allocate(bool minimalSize){
    if ((capacity_ < nRows_ * nColumns_) || (minimalSize && (capacity_ > nRows_ * nColumns_))){
	if (elem_)
	    delete [] elem_;
	try {
	    elem_ = nRows_ * nColumns_ > 0 ? new T[nRows_*nColumns_] : 0;
	    capacity_ = nRows_ * nColumns_;
	    return true;
	}
	catch (std::bad_alloc& ba) {
	    return false;
	}
    }
    else
	return true;
}

// constructor with allocation
template<typename T>
FastMatrix<T>::FastMatrix(u32 nRows, u32 nColumns) :
nRows_(nRows),
nColumns_(nColumns),
elem_(0),
capacity_(0),
nThreads_(1)
{
    initialized ? maxThreads : initialize();
    if (nRows_* nColumns_ < 250000)
	nThreads_ = 1;
    if (!allocate())
	Core::Application::us()->criticalError("failed to allocate memory for matrix of size ") << nRows_ << " x " << nColumns_;
}

// copy constructor
template<typename T>
FastMatrix<T>::FastMatrix(const FastMatrix<T> &X) :
nRows_(X.nRows_),
nColumns_(X.nColumns_),
elem_(0),
capacity_(0),
nThreads_(1)
{
    initialized ? maxThreads : initialize();
    if (nRows_* nColumns_ < 250000)
	nThreads_ = 1;
    if (!allocate())
	Core::Application::us()->criticalError("failed to allocate memory for matrix of size ") << nRows_ << " x " << nColumns_;
    copy(X);
}

// copy constructor for sub-matrices
template<typename T>
FastMatrix<T>::FastMatrix(const FastMatrix<T> &X, u32 rowIndexX, u32 colIndexX,
    u32 thisRowIndex, u32 thisColIndex, u32 nRows, u32 nCols) :
nRows_(nRows),
nColumns_(nCols),
capacity_(0),
elem_(0),
nThreads_(initialized ? maxThreads : initialize())
{
    if (nRows_* nColumns_ < 250000)
	nThreads_ = 1;
    if (!allocate())
	Core::Application::us()->criticalError("failed to allocate memory for matrix of size ") << nRows_ << " x " << nColumns_;
    copyBlockFromMatrix(X, rowIndexX, colIndexX, thisRowIndex, thisColIndex, nRows, nCols);
}

template<typename T>
FastMatrix<T>::~FastMatrix(){
    if (elem_)
	delete [] elem_;
    elem_ = 0;
}

template<typename T>
void FastMatrix<T>::clear(){
    if (elem_)
	delete [] elem_;
    elem_ = 0;
}


template<typename T>
void FastMatrix<T>::swap(FastMatrix<T> &X){
    std::swap(nRows_, X.nRows_);
    std::swap(nColumns_, X.nColumns_);
    std::swap(elem_, X.elem_);
}

template<typename T>
FastMatrix<T> &FastMatrix<T>::operator=(FastMatrix<T> rhs) {
    swap(rhs);
    return *this;
}

template<typename T>
void FastMatrix<T>::resize(u32 nRows, u32 nColumns, bool minimalSize){
    bool reallocate = nRows * nColumns != nRows_ * nColumns_;
    nRows_ = nRows;
    nColumns_ = nColumns;
    if (reallocate && !allocate()){
	Core::Application::us()->criticalError("failed to allocate memory for matrix of size ") << nRows_ << " x " << nColumns_;
    }
}

template<typename T> template <typename S>
void FastMatrix<T>::copyStructure(const FastMatrix<S> &X){
    bool reallocate = X.nRows_ * X.nColumns_ != nRows_ * nColumns_;
    nRows_ = X.nRows();
    nColumns_ = X.nColumns();
    if (reallocate && !allocate()){
	Core::Application::us()->criticalError("failed to allocate memory for matrix of size ") << nRows_ << " x " << nColumns_;
    }
}

template<typename T>
bool FastMatrix<T>::isFinite() const {
    for (u32 row = 0; row < nRows_; row++){
	for (u32 column = 0; column < nColumns_; column++){
	    T val = at(row, column);
	    if (Math::isnan(val) || val > Core::Type<T>::max || val < Core::Type<T>::min)
		return false;
	}
    }
    return true;
}

template<typename T>
void FastMatrix<T>::ensureMinimalValue(const T threshold) {
    for (u32 row = 0; row < nRows_; row++) {
	for (u32 column = 0; column < nColumns_; column++) {
	    if (at(row, column) < threshold)
		at(row, column) = threshold;
	}
    }
}

template<typename T>
void FastMatrix<T>::show() const{
    for (u32 i = 0; i < nRows_; i++){
	for (u32 j = 0; j < nColumns_; j++){
	    std::cout << at(i,j) << " ";
	}
	std::cout << std::endl;
    }
}

template<typename T>
void FastMatrix<T>::tanh(){
# pragma omp parallel for
    for (u32 i = 0; i < nRows_ * nColumns_; i++)
	elem_[i] = std::tanh(elem_[i]);
}

template<typename T>
void FastMatrix<T>::exp(){
    mt_vr_exp(nRows_ * nColumns_, elem_ , elem_, nThreads_);
}

template<typename T>
void FastMatrix<T>::log(){
    vr_log(nRows_ * nColumns_, elem_ , elem_);
}

template<typename T>
void FastMatrix<T>::sigmoid(T gamma){
    scale(-gamma);
    exp();
#pragma omp parallel for
    for (u32 i = 0; i < nRows_ * nColumns_; i++)
	elem_[i] = 1.0 / (1.0 + elem_[i]);
}

template<typename T>
void FastMatrix<T>::softmax(){
    // softmax: t(i) = exp(s(i)               ) / sum_j { exp(s(j)                ) } (column-wise)
    // softmax: t(i) = exp(s(i) - MAX_j{s(j)} ) / sum_k { exp(s(k) - max_j {s(j)} ) }, more robust computation (avoids overflow)

    // get max of all columns and remove it from all rows
    FastVector<T> tmp(nColumns_);
    tmp.getMaxOfColumns(*this);
    addToAllRows(tmp, (T) -1.0);
    // exponentiate
    exp();
    // accumulate entries of each column
    tmp.setToZero();
    tmp.addSummedRows(*this);

    // compute actual softmax output for each column
    divideColumnsByScalars(tmp);
}

template<typename T>
u32 FastMatrix<T>::argAbsMax(u32 column) const {
    require_lt(column, nColumns_);
    return Math::iamax(nRows_, elem_ + column * nRows_, 1);
}

template<typename T>
u32 FastMatrix<T>::argMax(u32 column) const {
    require_lt(column, nColumns_);
    T maxVal = Core::Type<T>::min;
    u32 result = 0;
    for (u32 i = 0; i < nRows_; i++){
	T val = elem_[column*nRows_ + i];
	if (val > maxVal){
	    maxVal = val;
	    result = i;
	}
    }
    return result;
}


template<typename T>
template<typename S>
void FastMatrix<T>::add(const FastMatrix<S> &X, S alpha){
    require_eq(X.nRows(), nRows_);
    require_eq(X.nColumns(), nColumns_);
    Math::axpy<S,T>(nRows_ * nColumns_, alpha, X.elem_, 1, elem_, 1);
}

template<typename T>
T FastMatrix<T>::dot(const FastMatrix<T> &X) const {
    require_eq(X.nRows(), nRows_);
    require_eq(X.nColumns(), nColumns_);
    return Math::mt_dot(nRows_ * nColumns_, elem_, X.elem_, nThreads_);
}

template<typename T>
void FastMatrix<T>::scale(T alpha){
    Math::mt_scal(nRows_ * nColumns_, alpha, elem_, nThreads_);
}

template<typename T>
template<typename S>
void FastMatrix<T>::copy(const FastMatrix<S> &X){
    require_eq(X.nRows(), nRows_);
    require_eq(X.nColumns(), nColumns_);
    Math::copy<S,T>(nRows_ * nColumns_, X.elem_, 1, elem_, 1);
}

template<typename T>
template<typename S>
void FastMatrix<T>::copy(const S *X, u32 rowOffset, u32 colOffset){
    require_lt(rowOffset, nRows_);
    require_lt(colOffset, nColumns_);
    return Math::copy<S,T>(nRows_ * nColumns_ - colOffset * nRows_ - rowOffset, X, 1, elem_ + colOffset * nRows_ + rowOffset, 1);
}

template<typename T>
template<typename S>
void FastMatrix<T>::copy(const std::vector<S> &X, u32 rowOffset, u32 colOffset){
    require_lt(rowOffset, nRows_);
    require_lt(colOffset, nColumns_);
    return Math::copy<S,T>(X.size(), &X.at(0), 1, elem_ + colOffset * nRows_ + rowOffset, 1);
}

template<typename T>
template<typename S>
void FastMatrix<T>::copy(const Matrix<S> &matrix){
    resize(matrix.nRows(), matrix.nColumns());
    for (u32 i = 0; i < nRows_; i++){
	for (u32 j = 0; j < nColumns_; j++){
	    at(i,j) = matrix[i][j];
	}
    }
}

template<typename T>
template<typename S>
void FastMatrix<T>::convert(Matrix<S> &matrix) const {
    matrix.resize(nRows_, nColumns_);
    for (u32 i = 0; i < nRows_; i++){
	for (u32 j = 0; j < nColumns_; j++){
	    matrix[i][j] = at(i,j);
	}
    }
}


template<typename T>
void FastMatrix<T>::multiply(const FastVector<T> &x, FastVector<T> &y, bool transposed, T alpha, T beta, u32 lda) const {
    require_le(lda,nRows_);
    if (lda == 0)
	lda = nRows_;
    if (!transposed && lda == nRows_){
	require_eq(x.nRows(), nColumns_);
	require_eq(y.nRows(), nRows_);
    }
    else if (transposed && lda == nRows_){
	require_eq(x.nRows(), nRows_);
	require_eq(y.nRows(), nColumns_);
    }
    CBLAS_TRANSPOSE tr = transposed ? CblasTrans : CblasNoTrans;
    // assume col major order
    Math::gemv<T>(CblasColMajor, tr, nRows_, nColumns_, alpha, elem_, lda, x.begin(), 1, beta, y.begin(), 1);
}

template<typename T>
void FastMatrix<T>::elementwiseMultiplication(const FastMatrix<T> &X){
    require_eq(X.nRows(), nRows_);
    require_eq(X.nColumns(), nColumns_);
    std::transform(elem_, elem_ + nRows_ * nColumns_, X.elem_, elem_, std::multiplies<T>());
}

template<typename T>
void FastMatrix<T>::elementwiseMultiplicationWithSigmoidDerivative(const FastMatrix<T> &X) {
    require_eq(X.nRows(), nRows_);
    require_eq(X.nColumns(), nColumns_);
#pragma omp parallel for
    for (u32 i = 0; i < nRows_ * nColumns_; i++)
	elem_[i] *= X.elem_[i] * (1.0 - X.elem_[i]);
}

template<typename T>
void FastMatrix<T>::elementwiseMultiplicationWithTanhDerivative(const FastMatrix<T> &X) {
    require_eq(X.nRows(), nRows_);
    require_eq(X.nColumns(), nColumns_);
# pragma omp parallel for
    for (u32 i = 0; i < nRows_ * nColumns_; i++)
	elem_[i] *= 1.0 - X.elem_[i] * X.elem_[i];
}

template<typename T>
void FastMatrix<T>::multiplicationWithSoftmaxDerivative(const Math::FastMatrix<T>& softmax) {
    require_eq(softmax.nRows(), nRows_);
    require_eq(softmax.nColumns(), nColumns_);
    FastVector<T> v(nColumns_);
    v.columnwiseInnerProduct(softmax, *this);
    for (u32 column = 0; column < nColumns_; column++) {
	for (u32 row = 0; row < nRows_; row++) {
	    at(row,column) = softmax.at(row,column) * (at(row,column) -  v.at(column));
	}
    }
}

template<typename T>
void FastMatrix<T>::elementwiseMultiplicationWithRectifiedDerivative(const Math::FastMatrix<T>& X) {
    require_eq(X.nRows(), nRows_);
    require_eq(X.nColumns(), nColumns_);
# pragma omp parallel for
    for (u32 i = 0; i < nRows_ * nColumns_; i++)
	if (X.elem_[i] <= 0) elem_[i] = 0;
}

template<typename T>
template<typename S>
inline void FastMatrix<T>::addKroneckerDelta(const FastVector<S>& alignment, const T scale) {
    require_eq(nColumns_, alignment.nRows());
    if (typeid(S) != typeid(u32)) {
	Core::Application::us()->warning("FastMatrix<T>::kroneckerDelta expects alignment vector of type u32");
    }
    for (u32 column = 0; column < nColumns_; column++) {
	require_lt((u32)alignment[column], nRows_);
	at(alignment[column], column) += scale;
    }
}

template<typename T>
template<typename S>
u32 FastMatrix<T>::nClassificationErrors(const FastVector<S>& alignment) {
    require_eq(nColumns_, alignment.nRows());
    if (typeid(S) != typeid(u32)) {
	Core::Application::us()->warning("FastMatrix<T>::nClassificationErrors expects alignment vector of type u32");
    }
    u32 classificationErrors = 0;
#pragma omp parallel for reduction(+:classificationErrors)
    for (u32 column = 0; column < nColumns_; column++) {
	if (argMax(column) != (u32)alignment[column])
	    classificationErrors++;
    }
    return classificationErrors;
}


template<typename T>
template<typename S>
T FastMatrix<T>::crossEntropyObjectiveFunction(const FastVector<S>& alignment) {
    require_eq(nColumns_, alignment.nRows());
    if (typeid(S) != typeid(u32)) {
	Core::Application::us()->warning("FastMatrix<T>::crossEntropyObjectiveFunction expects alignment vector of type u32");
    }
    T objFctn = 0;
    for (u32 column = 0; column < nColumns_; column++) {
	objFctn -= std::log(at(alignment[column], column));
    }
    return objFctn;
}

template<typename T>
template<typename S>
T FastMatrix<T>::weightedCrossEntropyObjectiveFunction(const FastVector<S>& alignment, const FastVector<T>& weights) {
    require_eq(nColumns_, alignment.nRows());
    require_eq(nColumns_, weights.nRows());
    if (typeid(S) != typeid(u32)) {
	Core::Application::us()->warning("FastMatrix<T>::weightedCrossEntropyObjectiveFunction expects alignment vector of type u32");
    }
    T objFctn = 0;
    for (u32 column = 0; column < nColumns_; column++) {
	objFctn -= std::log(at(alignment[column], column)) * weights[column];
    }
    return objFctn;
}

template<typename T>
template<typename S>
T FastMatrix<T>::squaredErrorObjectiveFunction(const FastVector<S>& alignment) {
    require_eq(nColumns_, alignment.nRows());
    if (typeid(S) != typeid(u32)) {
	Core::Application::us()->warning("FastMatrix<T>::squaredErrorObjectiveFunction expects alignment vector of type u32");
    }
    T objFctn = 0;
    for (u32 row = 0; row < nRows_; row++) {
	for (u32 column = 0; column < nColumns_; column++) {
	    T kroneckerDelta = (alignment[column] == row) ? 1.0 : 0.0;
	    objFctn += (at(row, column) - kroneckerDelta) * (at(row, column) - kroneckerDelta);
	}
    }
    return objFctn;
}

template<typename T>
template<typename S>
T FastMatrix<T>::weightedSquaredErrorObjectiveFunction(const FastVector<S>& alignment, const FastVector<T>& weights) {
    require_eq(nColumns_, alignment.nRows());
    require_eq(nColumns_, weights.nRows());
    if (typeid(S) != typeid(u32)) {
	Core::Application::us()->warning("FastMatrix<T>::weightedSquaredErrorObjectiveFunction expects alignment vector of type u32");
    }
    T objFctn = 0;
    for (u32 row = 0; row < nRows_; row++) {
	for (u32 column = 0; column < nColumns_; column++) {
	    T kroneckerDelta = (alignment[column] == row) ? 1.0 : 0.0;
	    objFctn += (at(row, column) - kroneckerDelta) * (at(row, column) - kroneckerDelta) * weights[column];
	}
    }
    return objFctn;
}

template<typename T>
template<typename S>
T FastMatrix<T>::binaryDivergenceObjectiveFunction(const FastVector<S>& alignment) {
    require_eq(nColumns_, alignment.nRows());
    if (typeid(S) != typeid(u32)) {
	Core::Application::us()->warning("FastMatrix<T>::binaryDivergenceObjectiveFunction expects alignment vector of type u32");
    }
    T objFctn = 0;

#pragma omp parallel for
    for (u32 row = 0; row < nRows_; row++) {
	for (u32 column = 0; column < nColumns_; column++) {
	    const T y = at(row, column);
	    if (alignment[column] == row)
		objFctn -= std::log(y);
	    else
		objFctn -= std::log(1.0 - y);
	}
    }
    return objFctn;
}

template<typename T>
template<typename S>
T FastMatrix<T>::weightedBinaryDivergenceObjectiveFunction(const FastVector<S>& alignment, const FastVector<T>& weights) {
    require_eq(nColumns_, alignment.nRows());
    require_eq(nColumns_, weights.nRows());
    if (typeid(S) != typeid(u32)) {
	Core::Application::us()->warning("FastMatrix<T>::weightedBinaryDivergenceObjectiveFunction expects alignment vector of type u32");
    }
    T objFctn = 0;

#pragma omp parallel for
    for (u32 row = 0; row < nRows_; row++) {
	for (u32 column = 0; column < nColumns_; column++) {
	    const T y = at(row, column);
	    if (alignment[column] == row && y > 0.0)
		objFctn -= std::log(y) * weights[column];
	    else if (alignment[column] != row && y < 1.0)
		objFctn -= std::log(1.0 - y) * weights[column];
	}
    }
    return objFctn;
}

template<typename T>
template<typename S>
void FastMatrix<T>::binaryDivergenceSoftmaxGradient(const FastMatrix<T> &Y, const FastVector<S> &A){
#pragma omp parallel for
    for (u32 column = 0; column < nColumns_; column++){
	T constsum = 0.0;
	for (u32 i = 0; i < nRows_; ++ i) {
	    const T y = Y.at(i, column);
	    if (A[column] == i)
		constsum -= 1.0;
	    else
		if (y<1.0) constsum += y / (1.0-y);
	}

	for (u32 i = 0; i < nRows_; ++ i) {
	    const T y = Y.at(i, column);
	    if (A[column] == i)
		at(i, column) = -1.0 - y*constsum;
	    else {
		if (y<1.0)
		    at(i, column) = y * (1.0 / (1.0 - y) - constsum);
		else
		    at(i, column) = 0.0;
	    }
	}
    }
}

template<typename T>
T FastMatrix<T>::dotWithColumn(const FastMatrix<T> &X, u32 thisColumnIndex) const {
    require_eq(X.nRows(), nRows_);
    require_lt(thisColumnIndex, nColumns_);
    return Math::dot(nRows_, &(at(0, thisColumnIndex)), 1, X.elem_, 1);
}

template<typename T>
void FastMatrix<T>::setToSecondOrderFeatures(const FastMatrix<T> &X){
    require_eq(nColumns_, X.nColumns_);
    require_eq(nRows_, X.nRows_ + (X.nRows_ * (X.nRows_ + 1)) / 2);
    // copy first order features
    copyBlockFromMatrix(X, 0, 0, 0, 0, X.nRows_, X.nColumns_);
    // append second order features
    appendSecondOrderFeatures(X, X.nRows_);
}

template<typename T>
void FastMatrix<T>::setToThirdOrderFeatures(const FastMatrix<T> &X){
    require_eq(nColumns_, X.nColumns_);
    require_eq(nRows_, X.nRows_ + (X.nRows_ * (X.nRows_ + 1)) / 2 + (X.nRows_ * (X.nRows_ + 1) * (X.nRows_ + 2)) / 6);
    // copy first order features
    copyBlockFromMatrix(X, 0, 0, 0, 0, X.nRows_, X.nColumns_);
    // append second order features
    appendSecondOrderFeatures(X, X.nRows_);
    // append second order features
    appendThirdOrderFeatures(X, X.nRows_ + (X.nRows_ * (X.nRows_ + 1)) / 2);
}

template<typename T>
void FastMatrix<T>::dropout(const T dropoutProbability) {
    Math::Nr::Ran2 randomNumberGenerator(rand());
    for (u32 row = 0; row < nRows_; row++) {
	for (u32 column = 0; column < nColumns_; column++) {
	    if (randomNumberGenerator.work() <= dropoutProbability)
		at(row, column) = 0.0;
	}
    }
}

template<typename T>
void FastMatrix<T>::addGaussianNoise(const T standardDeviation) {
    Math::Nr::Gasdev<Math::Nr::Ran3> normalDistribution(rand());
    for (u32 row = 0; row < nRows_; row++) {
	for (u32 column = 0; column < nColumns_; column++) {
	    at(row, column) += (T) normalDistribution.work() * standardDeviation;
	}
    }
}

template<typename T>
void FastMatrix<T>::l1clipping(const T value) {
    for (u32 row = 0; row < nRows_; row++) {
	for (u32 column = 0; column < nColumns_; column++) {
	    if (at(row, column) > 0) {
		at(row, column) = std::max((T)0.0, at(row, column) - value);
	    }
	    else if (at(row, column) < 0) {
		at(row, column) = std::min((T)0.0, at(row, column) + value);
	    }
	}
    }
}

template<typename T>
void FastMatrix<T>::appendSecondOrderFeatures(const FastMatrix<T> &X, u32 offset){
#pragma omp parallel for
    for (u32 column = 0; column < nColumns_; column++){
	u32 pos = offset;
	for (u32 i = 0; i < X.nRows_; ++ i) {
	    for (u32 j = i; j < X.nRows_; ++ j) {
		at(pos, column) = X.at(i, column) * X.at(j, column);
		pos++;
	    }
	}
    }
}

template<typename T>
void FastMatrix<T>::appendThirdOrderFeatures(const FastMatrix<T> &X, u32 offset){
#pragma omp parallel for
    for (u32 column = 0; column < nColumns_; column++){
	u32 pos = offset;
	for (u32 i = 0; i < X.nRows_; ++ i) {
	    for (u32 j = i; j < X.nRows_; ++ j) {
		for (u32 k = j; k < X.nRows_; ++ k) {
		    at(pos, column) = X.at(i, column) * X.at(j, column) * X.at(k, column);
		    pos++;
		}
	    }
	}
    }
}

template<typename T>
void FastMatrix<T>::elementwiseDivision(const FastMatrix<T> &X){
    require_eq(X.nRows(), nRows_);
    require_eq(X.nColumns(), nColumns_);
    std::transform(elem_, elem_ + nRows_ * nColumns_, X.elem_, elem_, std::divides<T>());
}

template<typename T>
void FastMatrix<T>::addConstantElementwise(T c) {
    std::transform(elem_, elem_ + nRows_ * nColumns_, elem_, std::bind2nd(std::plus<T>(), c));
}

template<typename T>
void FastMatrix<T>::addToAllColumns(const FastVector<T> &v, T alpha){
    require_eq(v.nRows(), nRows_);
# pragma omp parallel for
    for (u32 i = 0; i < nColumns_; i++)
	Math::mt_axpy(nRows_, alpha, v.begin(), elem_ + i*nRows_, nThreads_);
}

template<typename T>
void FastMatrix<T>::addToAllRows(const FastVector<T> &v, T alpha){
    // TODO: parallelize
    require_eq(v.nRows(), nColumns_);
#pragma omp parallel for
    for (u32 j = 0; j < nColumns_; j++){
	T value = alpha * v.at(j);
	std::transform(elem_ + j*nRows_, elem_ + (j+1)*nRows_, elem_ + j*nRows_, std::bind2nd(std::plus<T>(), value));
    }
}

template<typename T>
void FastMatrix<T>::multiplyColumnsByScalars(const FastVector<T> &scalars){
    require_eq(nColumns_, scalars.size());
#pragma omp parallel for
    for (u32 i = 0; i < nColumns_; i++)
	Math::scal(nRows_, scalars[i], &at(0, i), 1);
}

template<typename T>
void FastMatrix<T>::divideColumnsByScalars(const FastVector<T> &scalars){
    require_eq(nColumns_, scalars.size());
#pragma omp parallel for
    for (u32 i = 0; i < nColumns_; i++)
	Math::scal(nRows_, (T) 1.0 / scalars[i], &at(0, i), 1);
}

template<typename T>
void FastMatrix<T>::multiplyRowsByScalars(const FastVector<T> &scalars){
    require_eq(nRows_, scalars.size());
#pragma omp parallel for
    for (u32 i = 0; i < nRows_; i++)
	Math::scal(nColumns_, scalars[i], &at(i, 0), nRows_);
}

template<typename T>
void FastMatrix<T>::divideRowsByScalars(const FastVector<T> &scalars){
    require_eq(nRows_, scalars.size());
#pragma omp parallel for
    for (u32 i = 0; i < nRows_; i++)
	Math::scal(nColumns_, (T) 1.0 / scalars[i], &at(i, 0), nRows_);
}

template<typename T>
void FastMatrix<T>::getRow(u32 rowIndex, Math::FastVector<T> &row) const {
    require_lt(rowIndex, nRows_);
    row.resize(nColumns_);
    Math::copy(nColumns_, elem_ + rowIndex, nRows_, row.begin(), 1);
}

template<typename T>
void FastMatrix<T>::getColumn(u32 columnIndex, Math::FastVector<T> &column) const {
    require_lt(columnIndex, nColumns_);
    column.resize(nRows_);
    Math::copy(nRows_, elem_ + columnIndex * nRows_, 1, column.begin(), 1);
}

template<typename T>
void FastMatrix<T>::copyBlockFromMatrix(const Math::FastMatrix<T> &X, u32 rowIndexX, u32 colIndexX, u32 thisRowIndex, u32 thisColIndex, u32 nRows, u32 nCols) {
    require_le(thisColIndex + nCols, nColumns_);
    require_le(thisRowIndex + nRows, nRows_);
    require_le(colIndexX + nCols, X.nColumns_);
    require_le(rowIndexX + nRows, X.nRows_);
    for (u32 column = 0; column < nCols; column++){
	const T *posX =  &X.at(rowIndexX, colIndexX  + column);
	T * posThis = &at(thisRowIndex, thisColIndex + column);
	Math::copy(nRows, posX, 1, posThis, 1);
    }
}

template<typename T>
void FastMatrix<T>::addOuterProduct(const Math::FastVector<T> &x, const Math::FastVector<T> &y, T alpha, u32 lda){
    require_eq(x.size(), nRows_);
    require_eq(y.size(), nColumns_);
    require_le(lda, nRows_);
    if (lda == 0)
	lda = nRows_;
    Math::ger<T>(CblasColMajor, nRows_, nColumns_, alpha, x.begin(), 1, y.begin(), 1, elem_, lda);
}

// (*this) = (scaleA * matrixA) * matrixB + scaleC * (*this)
template<typename T>
template<typename S>
void FastMatrix<T>::addMatrixProduct(const FastMatrix<S> &matrixA, const FastMatrix<S> &matrixB,
	T scaleC, S scaleA, bool transposedA, bool transposedB) {
    u32 m = transposedA ? matrixA.nColumns_ : matrixA.nRows_;
    u32 n = transposedB ? matrixB.nRows_ : matrixB.nColumns_;
    u32 k = transposedA ? matrixA.nRows_ : matrixA.nColumns_;
    require_eq(m, nRows_);
    require_eq(n, nColumns_);
    require_eq(k, (transposedB ? matrixB.nColumns_ : matrixB.nRows_));

    _gemm(transposedA, transposedB,
	    m, n, k,
	    scaleA, matrixA.begin(), matrixA.nRows(),
	    matrixB.begin(), matrixB.nRows(),
	    scaleC, begin(), nRows());
}



// C = (scaleA * matrixA) * matrixB + scaleC * (*this)
template<typename T>
template<typename S>
void FastMatrix<T>::_gemm(bool transposedA, bool transposedB, int M, int N, int K,
	S scaleA, const S* matrixA, int lda,
	const S* matrixB, int ldb,
	T scaleC, T* matrixC, int ldc) {
    Math::gemm<T>(CblasColMajor,
	    transposedA ? CblasTrans : CblasNoTrans,
	    transposedB ? CblasTrans : CblasNoTrans,
	    M, N, K,
	    scaleA,
	    matrixA, lda,
	    matrixB, ldb,
	    scaleC, matrixC, ldc);
}

// own implementation of mixed precision matrix multiplication
// loop order guarantees cache efficiency

template<>
template<>
inline void FastMatrix<double>::_gemm(bool transposedA, bool transposedB, int M, int N, int K,
	float scaleA, const float* matrixA, int lda,
	const float* matrixB, int ldb,
	double scaleC, double* matrixC, int ldc) {
    verify(!transposedA); // simplified implementation
    Math::scal<double>(M*N, scaleC, matrixC, 1);

    if (!transposedB){
#pragma omp parallel for
	for (int i = 0; i < N; i++) {
	    double *ci = &matrixC[i*ldc];
	    for (int j = 0; j < K; j++) {
		float bij = matrixB[i*ldb + j];
		const float *aj = &matrixA[j*lda];
		for (int l = 0; l < M; l++){
		    ci[l] += scaleA * (bij * aj[l]);
		}
	    }
	}
    }
    else {
#pragma omp parallel for
	for (int i = 0; i < N; i++) {
	    double *ci = &matrixC[i*ldc];
	    for (int j = 0; j < K; j++) {
		float bij = matrixB[i + j*ldb];
		const float *aj = &matrixA[j*lda];
		for (int l = 0; l < M; l++)
		    ci[l] += scaleA * (bij * aj[l]);
	    }
	}
    }
}


} // namespace (Math)


#endif /* FASTMATRIX_HH_ */
