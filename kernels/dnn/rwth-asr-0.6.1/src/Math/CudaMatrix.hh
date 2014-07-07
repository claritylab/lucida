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
#ifndef CUDAMATRIX_HH_
#define CUDAMATRIX_HH_

#include <cmath>

#include <Math/CudaDataStructure.hh>
#include <Math/CudaMatrixKernelsWrapper.hh>
#include <Math/CudaVector.hh>
#include <Math/FastMatrix.hh>

namespace Math {

/*
 * TODO
 *
 * check maximum matrix dimensions (needs to work with Cuda !!)
 *
 */

/*
 * CudaMatrix
 *
 * Matrix class that makes use of GPU parallelization when compile with MODULE_CUDA and GPU is available.
 * Derives from FastMatrix.
 *
 * Concept:
 *
 * gpuMode_: const flag, true when compiled with MODULE_CUDA and GPU is available
 * if gpuMode_:
 * 	all computations are performed on GPU, data on host and on device may differ
 * if isComputing_
 * 	* the data on the device is the most recent one
 * 	* it is not possible to access matrix elements on the host
 * synchronization and change of the isComputing_ flag is managed by the methods initComputation() and finishComputation()
 * use of protected inheritance in order to avoid use of FastMatrix methods without checking the isComputing_ flag
 *
 */

template<typename T>
class CudaMatrix : protected FastMatrix<T>, public CudaDataStructure {
    friend class CudaVector<T>;
    friend class CudaVector<double>;
    friend class CudaVector<float>;
    friend class CudaMatrix<double>;
    friend class CudaMatrix<float>;
    friend class CudaVector<u32>;
    typedef FastMatrix<T> Precursor;
protected:
    using Precursor::nColumns_;
    using Precursor::nRows_;
    using Precursor::elem_;
    using Precursor::capacity_;
    using CudaDataStructure::cublasHandle;
    using CudaDataStructure::gpuMode_;
    using CudaDataStructure::multiPrecisionBunchSize;
protected:
    mutable bool isComputing_;
    T *d_elem_;
    u32 gpuCapacity_;
public:
    // constructor with memory allocation
    CudaMatrix(u32 nRows = 0, u32 nColumns = 0);

    // copy constructor
    CudaMatrix(const CudaMatrix<T> &X);

    virtual ~CudaMatrix();
private:
    bool allocateGpuMemory(bool minimalSize=false);

public: // GPU handling
    void initComputation(bool sync = true) const;
    void finishComputation(bool sync = true) const ;
    bool isComputing() const { return isComputing_; }
    bool isInGpuMode() const { return gpuMode_; }
public:
    void show();
    void syncAndShow();
    void clear();

    T* d_elem() { return d_elem_; }
public:
    // methods that can only be called when !isComputing
    // and call their predecessor method

    // resize & allocate
    // side effect: after resize content is meaningless
    void resize(u32 nRows, u32 nColumns, bool minimalSize=false);

    // resize to size of X & allocate
    // side effect: after resize content is meaningless
    template <typename S>
    void copyStructure(const CudaMatrix<S> &X);

    u32 nRows() const;
    u32 nColumns() const;

    bool empty() const { return Precursor::empty(); }

    bool isFinite() const;

    u32 size() const;

    void fill(T value);

    // set all values < threshold to threshold
    void ensureMinimalValue(const T threshold);

    T& at(u32 i, u32 j);
    const T& at(u32 i, u32 j) const;

    // get row with index rowIndex
    void getRow(u32 rowIndex, Math::CudaVector<T> &row) const;

    // get column with index columnIndex
    void getColumn(u32 columnIndex, Math::CudaVector<T> &column) const;

    // copy block from FastMatrix to specific position
    void copyBlockFromMatrix(const Math::FastMatrix<T> &X, u32 rowIndexX, u32 colIndexX, u32 thisRowIndex, u32 thisColIndex, u32 nRows, u32 nCols = 1);
    // copy block from CudaMatrix to specific position
    void copyBlockFromMatrix(const Math::CudaMatrix<T> &X, u32 rowIndexX, u32 colIndexX, u32 thisRowIndex, u32 thisColIndex, u32 nRows, u32 nCols = 1);

    // this = 0
    void setToZero();

    // fast matrix access without assertions for host memory (use only if performance is important)
    T& operator() (s32 row, s32 column);
    const T& operator() (s32 row, s32 column) const;

    // need assignment operator, because we have a copy constructor
    // pass by value ! (needed for temporary object creation)
    CudaMatrix<T> & operator=(CudaMatrix<T> X);

    // required for assignment operator
    void swap(CudaMatrix<T> &X);

public:
    // iterators
    typedef T* iterator;
    typedef const T* const_iterator;
    iterator begin();
    const_iterator begin() const;
    iterator end();
    const_iterator end() const;

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
    void add(const CudaMatrix<S> &X, S alpha = 1.0);

    // @return l1-norm of matrix
    T l1norm() const;

    T sumOfSquares() const { return dot(*this); }

    // dot product
    // return this' * X
    // for matrices with multiple columns: interpret matrix as vector
    T dot(const CudaMatrix<T> &X) const;

    // scale elements
    // this *= alpha
    void scale(T alpha);

    // copy method
    // this = X
    // for matrices with same dimensions
    template<typename S>
    void copy(const CudaMatrix<S> &X);

    // copy from array
    void copy(const T *X, u32 rowOffset = 0, u32 colOffset = 0);

    // copy matrix from conventional sprint matrices
    template<typename S>
    void copy(const Matrix<S> &matrix);

    // copy from std::vector
    void copy(const std::vector<T> &X, u32 rowOffset = 0, u32 colOffset = 0);

    // convert to conventional sprint Matrix
    template<typename S>
    void convert(Matrix<S> &matrix) const ;

    /*
     * Blas2-like methods
     */

    // matrix vector product
    // Y := alpha * this * X + beta * Y,   or   Y := alpha * this**T * X + beta * Y
    void multiply(const CudaVector<T> &x, CudaVector<T> &y,
	    bool transposed = false, T alpha = 1.0, T beta = 0.0, u32 lda = 0) const;

    // rank-1 update: this += alpha * x y^T
    void addOuterProduct(const CudaVector<T>& x, const CudaVector<T> &y, T alpha, u32 lda = 0);

    /*
     * Blas3-like methods
     */
    template<typename S>
    void addMatrixProduct(const CudaMatrix<S> &matrixA, const CudaMatrix<S> &matrixB,
	    T scaleC = 0, S scaleA = 1, bool transposedA = false, bool transposedB = false);

    /*
     * special methods required for neural network computations
     */
    // apply sigmoid function to each element of matrix
    void sigmoid(T gamma = 1.0);

    // apply softmax to each column of matrix
    void softmax();

    // apply softmax to each column of matrix, tmp vector and tmp matrix are passed
    void softmax(CudaVector<T> &tmpVector, CudaMatrix<T> &tmpMatrix);

    // this = this .* (X .* (1 - X))
    void elementwiseMultiplicationWithSigmoidDerivative(const CudaMatrix<T> &X);

    // this = this .* (1 - X .^ 2)
    void elementwiseMultiplicationWithTanhDerivative(const CudaMatrix<T> &X);

    // for each column i: this(_,i) = diag(softmax(_,i) - softmax(_,i)*softmax(_,i)^T) * this(_,i)
    void multiplicationWithSoftmaxDerivative(const CudaMatrix<T> &softmax);

    // this = this .* sign(X)
    void elementwiseMultiplicationWithRectifiedDerivative(const CudaMatrix<T> &X);

    // apply kronecker delta (based on alignment) to each column of the matrix
    template<typename S>
    void addKroneckerDelta(const CudaVector<S>& alignment, const T scale = 1.0);

    // return number of classifications errors; each column of *this is interpreted as a probability distribution
    template<typename S>
    u32 nClassificationErrors(const CudaVector<S>& alignment);

    // return the value of the cross entropy objective function; each column of *this is interpreted as a probability distribution
    template<typename S>
    T crossEntropyObjectiveFunction(const CudaVector<S>& alignment);

    // return the value of the weighted cross entropy objective function; each column of *this is interpreted as a probability distribution
    template<typename S>
    T weightedCrossEntropyObjectiveFunction(const CudaVector<S>& alignment, const CudaVector<T>& weights);

    // return the value of the squared error objective function
    template<typename S>
    T squaredErrorObjectiveFunction(const CudaVector<S>& alignment);

    // return the value of the weighted squared error objective function
    template<typename S>
    T weightedSquaredErrorObjectiveFunction(const CudaVector<S>& alignment, const CudaVector<T>& weights);

    // return the value of the binary divergence objective function
    template<typename S>
    T binaryDivergenceObjectiveFunction(const CudaVector<S>& alignment);

    // return the value of the weighted binary divergence objective function
    template<typename S>
    T weightedBinaryDivergenceObjectiveFunction(const CudaVector<S>& alignment, const CudaVector<T>& weights);

    void binaryDivergenceSoftmaxGradient(const CudaMatrix<T> &Y, const CudaVector<u32> &A);

    T dotWithColumn(const CudaMatrix<T> &X, u32 thisColumnIndex) const;

    void setToSecondOrderFeatures(const CudaMatrix<T> &X);

    void setToThirdOrderFeatures(const CudaMatrix<T> &X);

    void dropout(const T dropoutProbability);

    void addGaussianNoise(const T standardDeviation);

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
    u32 argAbsMax(u32 column);

    // this = this .* X
    void elementwiseMultiplication(const CudaMatrix<T> &X);

    // this = this ./ X
    void elementwiseDivision(const CudaMatrix<T> &X);

    void addConstantElementwise(T c);

    // add vector to each column of the matrix
    void addToAllColumns(const CudaVector<T> &v, T alpha = 1.0);

    // add vector to each row of the matrix
    void addToAllRows(const CudaVector<T> &v, T alpha = 1.0);

    // for each i: multiply column i by scalars[i]
    void multiplyColumnsByScalars(const CudaVector<T> &scalars);

    // for each i: divide column i by scalars[i]
    void divideColumnsByScalars(const CudaVector<T> &scalars);

    // for each i: multiply row i by scalars[i]
    void multiplyRowsByScalars(const CudaVector<T> &scalars);

    // for each i: divide row i by scalars[i]
    void divideRowsByScalars(const CudaVector<T> &scalars);

private:
    void appendSecondOrderFeatures(const FastMatrix<T> &X, u32 offset);

    template<typename S>
    static void _gemm(bool transposedA, bool transposedB, int M, int N, int K,
    S scaleA, const S *A, int lda, const S *B, int ldb, T scaleC, T *C, int ldc);

};

// constructors

template<typename T>
CudaMatrix<T>::CudaMatrix(u32 nRows, u32 nCols)
: Precursor(nRows, nCols),
  CudaDataStructure(),
  isComputing_(false),
  d_elem_(0),
  gpuCapacity_(0)
{
    if (!allocateGpuMemory())
	Core::Application::us()->error("failed to allocate GPU memory for matrix of size ") << nRows_ << " x " << nColumns_;
}

template<typename T>
CudaMatrix<T>::CudaMatrix(const CudaMatrix<T> &X)
: Precursor(X),
  CudaDataStructure(X),
  isComputing_(false),
  d_elem_(0),
  gpuCapacity_(0)
{
    require(!X.isComputing_);
    if (!allocateGpuMemory())
	Core::Application::us()->error("failed to allocate GPU memory for matrix of size ") << nRows_ << " x " << nColumns_;
}

template<typename T>
bool CudaMatrix<T>::allocateGpuMemory(bool minimalSize){
    int result = 0;
    if (gpuMode_){
	if ((gpuCapacity_ < nRows_ * nColumns_) || (minimalSize && (gpuCapacity_ > nRows_ * nColumns_))){
	    if (d_elem_)
		Cuda::free(d_elem_);
	    result = Cuda::alloc(d_elem_, nColumns_ * nRows_);
	    if (result == 0)
		gpuCapacity_ = nRows_ * nColumns_;
	    else
		Core::Application::us()->error("failed to allocate GPU memory for matrix of size ") << nRows_ << " x " << nColumns_;
	}
    }
    return result == 0;
}


template<typename T>
CudaMatrix<T>::~CudaMatrix(){
    int result = 0;
    if (gpuMode_){
	result = Cuda::free(d_elem_);
	require_eq(result, 0);
    }
}

template<typename T>
void CudaMatrix<T>::initComputation(bool sync) const {
    int result = 0;
    if (gpuMode_ && !isComputing_){
	if (sync){
	    result = Cuda::copyToGpu(d_elem_, elem_, nColumns_ * nRows_);
	    require_eq(result, 0);
	}
    }
    isComputing_ = true;
}

template<typename T>
void CudaMatrix<T>::finishComputation(bool sync) const {
    int result = 0;
    if (gpuMode_ && isComputing_) {
	if (d_elem_ && sync){
	    result = Cuda::copyFromGpu(elem_, d_elem_, nColumns_ * nRows_);
	    require_eq(result, 0);
	}
    }
    isComputing_ = false;
}

template<typename T>
void CudaMatrix<T>::resize(u32 nRows, u32 nCols, bool minimalSize) {
    bool reallocate = nRows * nCols != nRows_ * nColumns_;
    Precursor::resize(nRows, nCols);
    if (reallocate && !allocateGpuMemory(minimalSize))
	Core::Application::us()->error("failed to allocate GPU memory for matrix of size ") << nRows_ << " x " << nColumns_;
}

template<typename T>
template<typename S>
void CudaMatrix<T>::copyStructure(const CudaMatrix<S> &X) {
    bool reallocate = X.nRows_ * X.nColumns_ != nRows_ * nColumns_;
    Precursor::copyStructure(X);
    if (reallocate && !allocateGpuMemory())
	Core::Application::us()->error("failed to allocate GPU memory for matrix of size ") << nRows_ << " x " << nColumns_;
}

template<typename T>
u32 CudaMatrix<T>::nRows() const{
    return Precursor::nRows();
}

template<typename T>
u32 CudaMatrix<T>::nColumns() const{
    return Precursor::nColumns();
}

template<typename T>
u32 CudaMatrix<T>::size() const{
    return Precursor::size();
}

template<typename T>
bool CudaMatrix<T>::isFinite() const {
    require(!isComputing_);
    return Precursor::isFinite();
}

template<typename T>
void CudaMatrix<T>::fill(T value) {
    require(isComputing_);
    if (gpuMode_) {
	Cuda::fill(d_elem_, value, nRows_, nColumns_);
    } else {
	Precursor::fill(value);
    }
}

template<typename T>
void CudaMatrix<T>::ensureMinimalValue(const T threshold) {
    require(isComputing_);
    if (gpuMode_) {
	Cuda::ensureMinimalValue(d_elem_, threshold, nRows_ * nColumns_, 1);
    } else {
	Precursor::ensureMinimalValue(threshold);
    }
}

template<typename T>
T& CudaMatrix<T>::at(u32 i, u32 j) {
    require(!isComputing_);
    return Precursor::at(i,j);
}

template<typename T>
inline T& CudaMatrix<T>::operator() (s32 row, s32 column) {
    return *(Precursor::elem_ + column * nRows_ + row);
}

template<typename T>
inline const T& CudaMatrix<T>::operator() (s32 row, s32 column) const {
    return *(Precursor::elem_ + column * nRows_ + row);
}

template<typename T>
CudaMatrix<T> &CudaMatrix<T>::operator=(CudaMatrix<T> rhs) {
    swap(rhs);
    return *this;
}

template<typename T>
void CudaMatrix<T>::swap(CudaMatrix<T>& X) {
    require_eq(X.gpuMode_, gpuMode_);
    require_eq(X.isComputing_, isComputing_);
    Precursor::swap(X);
    std::swap(d_elem_, X.d_elem_);
}

template<typename T>
const T& CudaMatrix<T>::at(u32 i, u32 j) const {
    require(!isComputing_);
    return Precursor::at(i,j);
}

template<typename T>
T* CudaMatrix<T>::begin() {
    require(!isComputing_);
    return elem_;
}

template<typename T>
const T* CudaMatrix<T>::begin() const {
    require(!isComputing_);
    return elem_;
}

template<typename T>
T* CudaMatrix<T>::end() {
    require(!isComputing_);
    return &elem_[nRows_ * nColumns_];
}

template<typename T>
const T* CudaMatrix<T>::end() const {
    require(!isComputing_);
    return &elem_[nRows_ * nColumns_];
}

template<typename T>
void CudaMatrix<T>::tanh() {
    require(isComputing_);
    if (gpuMode_) {
	Cuda::tanh(d_elem_, nRows_, nColumns_);
    } else {
	Precursor::tanh();
    }
}

template<typename T>
void CudaMatrix<T>::sigmoid(T gamma) {
    require(isComputing_);
    if (gpuMode_)
	Cuda::sigmoid(gamma, d_elem_, nRows_, nColumns_);
    else
	Precursor::sigmoid(gamma);
}

template<typename T>
void CudaMatrix<T>::softmax() {
    require(isComputing_);
    if (gpuMode_) {
	CudaVector<T> tmpVector(nColumns_);
	CudaMatrix<T> tmpMatrix(32, nColumns_);
	tmpVector.initComputation(false);
	tmpMatrix.initComputation(false);
	tmpVector.getMaxOfColumns(*this, tmpMatrix);
	addToAllRows(tmpVector, (T) -1.0);

	exp();
	// accumulate entries of each column
	tmpVector.setToZero();
	tmpVector.addSummedRows(*this, tmpMatrix);

	// compute actual softmax output for each column
	divideColumnsByScalars(tmpVector);
    } else {
	Precursor::softmax();
    }
}

template<typename T>
void CudaMatrix<T>::softmax(CudaVector<T> &tmpVector, CudaMatrix<T> &tmpMatrix) {
    require(isComputing_);
    if (gpuMode_) {
	tmpVector.resize(nColumns_);
	tmpMatrix.resize(32, nColumns_);
	tmpVector.initComputation(false);
	tmpMatrix.initComputation(false);
	tmpVector.getMaxOfColumns(*this, tmpMatrix);
	addToAllRows(tmpVector, (T) -1.0);

	exp();
	// accumulate entries of each column
	tmpVector.setToZero();
	tmpVector.addSummedRows(*this, tmpMatrix);

	// compute actual softmax output for each column
	divideColumnsByScalars(tmpVector);
    } else {
	Precursor::softmax();
    }
}


template<typename T>
void CudaMatrix<T>::exp() {
    require(isComputing_);
    if (gpuMode_)
	Cuda::exp(d_elem_, nRows_ , nColumns_);
    else
	Precursor::exp();
}

template<typename T>
void CudaMatrix<T>::log() {
    require(isComputing_);
    if (gpuMode_)
	Cuda::log(d_elem_, nRows_ , nColumns_);
    else
	Precursor::log();
}

template<typename T>
u32 CudaMatrix<T>::argAbsMax(u32 column) {
    require(isComputing_);
    if (gpuMode_) {
	require_lt(column, nColumns_);
	int result = 0;
	Cuda::iamax(cublasHandle, nRows_, d_elem_ + column * nRows_, 1, &result);
	return result;
    } else {
	return Precursor::argAbsMax(column);
    }
}

template<typename T>
void CudaMatrix<T>::elementwiseMultiplicationWithSigmoidDerivative(const CudaMatrix<T> &X) {
    require(isComputing_);
    require(X.isComputing_);
    require_eq(X.nRows(), nRows_);
    require_eq(X.nColumns(), nColumns_);
    if (gpuMode_)
	Cuda::elementwiseMultiplicationWithSigmoidDerivative(d_elem_, X.d_elem_, X.nRows_, X.nColumns_);
    else
	Precursor::elementwiseMultiplicationWithSigmoidDerivative(X);
}

template<typename T>
void CudaMatrix<T>::elementwiseMultiplicationWithTanhDerivative(const CudaMatrix<T> &X) {
    require(isComputing_);
    require(X.isComputing_);
    require_eq(X.nRows(), nRows_);
    require_eq(X.nColumns(), nColumns_);
    if (gpuMode_)
	Cuda::elementwiseMultiplicationWithTanhDerivative(d_elem_, X.d_elem_, X.nRows_, X.nColumns_);
    else
	Precursor::elementwiseMultiplicationWithTanhDerivative(X);
}

template<typename T>
void CudaMatrix<T>::multiplicationWithSoftmaxDerivative(const Math::CudaMatrix<T>& softmax) {
    require(isComputing_);
    require(softmax.isComputing());
    if (gpuMode_) {
	require_eq(softmax.nRows(), nRows_);
	require_eq(softmax.nColumns(), nColumns_);
	CudaVector<T> v;
	v.initComputation();
	v.resize(nColumns_);
	v.columnwiseInnerProduct(softmax, *this);
	Cuda::multiplicationWithSoftmaxDerivative(d_elem_, softmax.d_elem_, v.d_elem_, nRows_, nColumns_);
    } else {
	Precursor::multiplicationWithSoftmaxDerivative(softmax);
    }
}

template<typename T>
void CudaMatrix<T>::elementwiseMultiplicationWithRectifiedDerivative(const CudaMatrix<T> &X) {
    require(isComputing_);
    require(X.isComputing_);
    require_eq(X.nRows(), nRows_);
    require_eq(X.nColumns(), nColumns_);
    if (gpuMode_)
	Cuda::elementwiseMultiplicationWithRectifiedDerivative(d_elem_, X.d_elem_, X.nRows_, X.nColumns_);
    else
	Precursor::elementwiseMultiplicationWithRectifiedDerivative(X);
}

template<typename T>
template<typename S>
void CudaMatrix<T>::addKroneckerDelta(const CudaVector<S>& alignment, const T scale) {
    require(isComputing_);
    require(alignment.isComputing());
    if (typeid(S) != typeid(u32)) {
	Core::Application::us()->warning("CudaMatrix<T>::kroneckerDelta expects alignment vector of type u32");
    }
    if (gpuMode_)
	Cuda::addKroneckerDelta(d_elem_, nRows_, nColumns_, alignment.d_elem_, scale);
    else
	Precursor::addKroneckerDelta(alignment, scale);
}

template<typename T>
template<typename S>
u32 CudaMatrix<T>::nClassificationErrors(const CudaVector<S>& alignment) {
    require(isComputing_);
    require(alignment.isComputing());
    if (typeid(S) != typeid(u32)) {
	Core::Application::us()->warning("CudaMatrix<T>::nClassificationErrors expects alignment vector of type u32");
    }
    require_eq(nColumns_, alignment.nRows());
    if (gpuMode_) {
	unsigned int result = 0u;
	unsigned int *resultDev;
	Cuda::alloc(resultDev, 1);
	Cuda::nClassificationErrors(d_elem_, nRows_, nColumns_, alignment.d_elem_, resultDev);
	Cuda::copyFromGpu(&result, resultDev, 1);
	return result;
    } else {
	return Precursor::nClassificationErrors(alignment);
    }
}

template<typename T>
template<typename S>
T CudaMatrix<T>::crossEntropyObjectiveFunction(const CudaVector<S>& alignment) {
    require(isComputing_);
    require(alignment.isComputing());
    if (typeid(S) != typeid(u32)) {
	Core::Application::us()->warning("CudaMatrix<T>::crossEntropyObjectiveFunction expects alignment vector of type u32");
    }
    require_eq(nColumns_, alignment.nRows());
    if (gpuMode_) {
	T result = 0;
	T *resultDev;
	Cuda::alloc(resultDev, 1);
	Cuda::crossEntropyObjectiveFunction(d_elem_, nRows_, nColumns_, alignment.d_elem_, resultDev);
	Cuda::copyFromGpu(&result, resultDev, 1);
	Cuda::free(resultDev);
	return result;
    } else {
	return Precursor::crossEntropyObjectiveFunction(alignment);
    }
}

template<typename T>
template<typename S>
T CudaMatrix<T>::weightedCrossEntropyObjectiveFunction(const CudaVector<S>& alignment, const CudaVector<T>& weights) {
    require(isComputing_);
    require(alignment.isComputing());
    require(weights.isComputing());
    if (typeid(S) != typeid(u32)) {
	Core::Application::us()->warning("CudaMatrix<T>::weightedCrossEntropyObjectiveFunction expects alignment vector of type u32");
    }
    require_eq(nColumns_, alignment.nRows());
    require_eq(nColumns_, weights.nRows());
    if (gpuMode_) {
	T result = 0;
	T *resultDev;
	Cuda::alloc(resultDev, 1);
	Cuda::weightedCrossEntropyObjectiveFunction(d_elem_, nRows_, nColumns_, alignment.d_elem_, resultDev, weights.d_elem_);
	Cuda::copyFromGpu(&result, resultDev, 1);
	Cuda::free(resultDev);
	return result;
    } else {
	return Precursor::weightedCrossEntropyObjectiveFunction(alignment, weights);
    }
}

template<typename T>
template<typename S>
T CudaMatrix<T>::squaredErrorObjectiveFunction(const CudaVector<S>& alignment) {
    require(isComputing_);
    require(alignment.isComputing());
    if (typeid(S) != typeid(u32)) {
	Core::Application::us()->warning("CudaMatrix<T>::squaredErrorObjectiveFunction expects alignment vector of type u32");
    }
    require_eq(nColumns_, alignment.nRows());
    if (gpuMode_) {
	T result = 0;
	T *resultDev;
	Cuda::alloc(resultDev, nRows_);
	Cuda::squaredErrorObjectiveFunction(d_elem_, nRows_, nColumns_, alignment.d_elem_, resultDev);
	Cuda::asum(cublasHandle, nRows_, resultDev, 1, &result);
	Cuda::free(resultDev);
	return result;
    } else {
	return Precursor::squaredErrorObjectiveFunction(alignment);
    }
}

template<typename T>
template<typename S>
T CudaMatrix<T>::weightedSquaredErrorObjectiveFunction(const CudaVector<S>& alignment, const CudaVector<T>& weights) {
    require(isComputing_);
    require(alignment.isComputing());
    require(weights.isComputing());
    if (typeid(S) != typeid(u32)) {
	Core::Application::us()->warning("CudaMatrix<T>::weightedSquaredErrorObjectiveFunction expects alignment vector of type u32");
    }
    require_eq(nColumns_, alignment.nRows());
    require_eq(nColumns_, weights.nRows());
    if (gpuMode_) {
	T result = 0;
	T *resultDev;
	Cuda::alloc(resultDev, nRows_);
	Cuda::weightedSquaredErrorObjectiveFunction(d_elem_, nRows_, nColumns_, alignment.d_elem_, resultDev, weights.d_elem_);
	Cuda::asum(cublasHandle, nRows_, resultDev, 1, &result);
	Cuda::free(resultDev);
	return result;
    } else {
	return Precursor::weightedSquaredErrorObjectiveFunction(alignment, weights);
    }
}

template<typename T>
template<typename S>
T CudaMatrix<T>::binaryDivergenceObjectiveFunction(const CudaVector<S>& alignment) {
    require(isComputing_);
    require(alignment.isComputing());
    if (typeid(S) != typeid(u32)) {
	Core::Application::us()->warning("CudaMatrix<T>::binaryDivergenceObjectiveFunction expects alignment vector of type u32");
    }
    require_eq(nColumns_, alignment.nRows());
    if (gpuMode_) {
	T result = 0;
	T *resultPerFrame;
	Cuda::alloc(resultPerFrame, nColumns_);
	Cuda::binaryDivergenceObjectiveFunction<T>(d_elem_, nRows_, nColumns_, alignment.d_elem_, resultPerFrame);
	Cuda::asum(cublasHandle, nColumns_, resultPerFrame, 1, &result);
	Cuda::free(resultPerFrame);
	return result;
    } else {
	return Precursor::binaryDivergenceObjectiveFunction(alignment);
    }
}

template<typename T>
template<typename S>
T CudaMatrix<T>::weightedBinaryDivergenceObjectiveFunction(const CudaVector<S>& alignment, const CudaVector<T>& weights) {
    require(isComputing_);
    require(alignment.isComputing());
    require(weights.isComputing());
    if (typeid(S) != typeid(u32)) {
	Core::Application::us()->warning("CudaMatrix<T>::weightedBinaryDivergenceObjectiveFunction expects alignment vector of type u32");
    }
    require_eq(nColumns_, alignment.nRows());
    require_eq(nColumns_, weights.nRows());
    if (gpuMode_) {
	T result = 0;
	T *resultPerFrame;
	Cuda::alloc(resultPerFrame, nColumns_);
	Cuda::weightedBinaryDivergenceObjectiveFunction<T>(d_elem_, nRows_, nColumns_, alignment.d_elem_, resultPerFrame, weights.d_elem_);
	Cuda::asum(cublasHandle, nColumns_, resultPerFrame, 1, &result);
	Cuda::free(resultPerFrame);
	return result;
    } else {
	return Precursor::weightedBinaryDivergenceObjectiveFunction(alignment, weights);
    }
}

template<typename T>
T CudaMatrix<T>::dotWithColumn(const CudaMatrix<T> &X, u32 thisColumnIndex) const {
    require_eq(X.nRows(), nRows_);
    require_lt(thisColumnIndex, nColumns_);
    if (gpuMode_){
	T dotProduct;
	int result = Cuda::dot(cublasHandle, nRows_, X.d_elem_, 1, d_elem_ + thisColumnIndex * nRows_, 1, dotProduct);
	require_eq(result, 0);
	return dotProduct;
    }
    else{
	return Precursor::dotWithColumn(X, thisColumnIndex);
    }

}

template<typename T>
void CudaMatrix<T>::setToSecondOrderFeatures(const CudaMatrix<T> &X){
    require(isComputing_);
    require(X.isComputing_);
    require_eq(nColumns_, X.nColumns_);
    require_eq(nRows_, X.nRows_ + (X.nRows_ * (X.nRows_ + 1)) / 2);
    if (gpuMode_){
	copyBlockFromMatrix(X, 0, 0, 0, 0, X.nRows_, X.nColumns_);
	Cuda::appendSecondOrderFeatures(X.d_elem_, X.nRows_, X.nColumns_, d_elem_, nRows_, X.nRows_);
    }
    else {
	Precursor::setToSecondOrderFeatures(X);
    }
}

template<typename T>
void CudaMatrix<T>::setToThirdOrderFeatures(const CudaMatrix<T> &X){
    require(isComputing_);
    require(X.isComputing_);
    require_eq(nColumns_, X.nColumns_);
    require_eq(nRows_, X.nRows_ + (X.nRows_ * (X.nRows_ + 1)) / 2 + (X.nRows_ * (X.nRows_ + 1) * (X.nRows_ + 2)) / 6);
    if (gpuMode_){
	copyBlockFromMatrix(X, 0, 0, 0, 0, X.nRows_, X.nColumns_);
	Cuda::appendSecondOrderFeatures(X.d_elem_, X.nRows_, X.nColumns_, d_elem_, nRows_, X.nRows_);
	Cuda::appendThirdOrderFeatures(X.d_elem_, X.nRows_, X.nColumns_, d_elem_, nRows_, X.nRows_ + (X.nRows_ * (X.nRows_ + 1)) / 2);
    }
    else {
	Precursor::setToThirdOrderFeatures(X);
    }
}

template<typename T>
void CudaMatrix<T>::dropout(const T dropoutProbability) {
    require(isComputing_);
    if (gpuMode_) {
	int result;
	T* mask;
	result = Cuda::alloc(mask, nColumns_ * nRows_);
	require_eq(result, 0);
	result = Cuda::generateUniform(randomNumberGenerator, mask, nColumns_ * nRows_);
	require_eq(result, 0);
	Cuda::dropout(d_elem_, mask, nRows_, nColumns_, dropoutProbability);
	Cuda::free(mask);
    }
    else {
	Precursor::dropout(dropoutProbability);
    }
}

template<typename T>
void CudaMatrix<T>::addGaussianNoise(const T standardDeviation) {
    require(isComputing_);
    if (gpuMode_) {
	int result;
	T* mask;
	result = Cuda::alloc(mask, nColumns_ * nRows_);
	require_eq(result, 0);
	result = Cuda::generateNormal(randomNumberGenerator, mask, nColumns_ * nRows_, (T) 0.0, standardDeviation);
	require_eq(result, 0);
	result = Cuda::axpy(cublasHandle, nColumns_ * nRows_, (T) 1.0, mask, 1, d_elem_, 1);
	require_eq(result, 0);
	Cuda::free(mask);
    }
    else {
	Precursor::addGaussianNoise(standardDeviation);
    }
}

template<typename T>
void CudaMatrix<T>::l1clipping(const T value) {
    require(isComputing_);
    if (gpuMode_) {
	Cuda::l1clipping(d_elem_, nRows_, nColumns_, value);
    }
    else {
	Precursor::l1clipping(value);
    }
}

template<typename T>
template<typename S>
void CudaMatrix<T>::add(const CudaMatrix<S> &X, S alpha) {
    require(isComputing_);
    require(X.isComputing_);
    if (gpuMode_){
	int result = Cuda::axpy(cublasHandle, nColumns_ * nRows_, alpha, X.d_elem_, 1, d_elem_, 1);
	require_eq(result, 0);
    }
    else
	Precursor::add(X, alpha);
}

template<typename T>
T CudaMatrix<T>::l1norm() const {
    require(isComputing_);
    if (gpuMode_) {
	T result;
	Cuda::asum(cublasHandle, nColumns_ * nRows_, d_elem_, 1, &result);
	return result;
    } else {
	return Precursor::l1norm();
    }
}

template<typename T>
T CudaMatrix<T>::dot(const CudaMatrix<T> &X) const{
    require(isComputing_);
    require(X.isComputing_);
    T dotProduct =0;
    if (gpuMode_){
	int result = Cuda::dot(cublasHandle, nColumns_ * nRows_, X.d_elem_, 1, d_elem_, 1, dotProduct);
	require_eq(result, 0);
	return dotProduct;
    }
    else
	return Precursor::dot(X);
}

template<typename T>
void CudaMatrix<T>::scale(T alpha) {
    require(isComputing_);
    if (gpuMode_){
	int result = Cuda::scal(cublasHandle, nColumns_ * nRows_, alpha, d_elem_, 1);
	require_eq(result, 0);
    }
    else
	Precursor::scale(alpha);
}

template<>
template<>
inline void CudaMatrix<double>::copy(const CudaMatrix<double> &X) {
    require(isComputing_);
    require(X.isComputing_);
    if (gpuMode_){
	require_eq(X.nRows(), nRows_);
	require_eq(X.nColumns(), nColumns_);
	int result = Cuda::copy(cublasHandle, nColumns_ * nRows_, X.d_elem_, 1, d_elem_, 1);
	require_eq(result, 0);
    }
    else
	Precursor::copy(X);
}

template<>
template<>
inline void CudaMatrix<float>::copy(const CudaMatrix<float> &X) {
    require(isComputing_);
    require(X.isComputing_);
    if (gpuMode_){
	require_eq(X.nRows(), nRows_);
	require_eq(X.nColumns(), nColumns_);
	int result = Cuda::copy(cublasHandle, nColumns_ * nRows_, X.d_elem_, 1, d_elem_, 1);
	require_eq(result, 0);
    }
    else
	Precursor::copy(X);
}

template<>
template<>
inline void CudaMatrix<double>::copy(const CudaMatrix<float> &X) {
    require(isComputing_);
    require(X.isComputing_);
    if (gpuMode_){
	require_eq(X.nRows(), nRows_);
	require_eq(X.nColumns(), nColumns_);
	Cuda::cast(nColumns_ * nRows_, X.d_elem_, d_elem_);
    }
    else
	Precursor::copy(X);
}


template<typename T>
void CudaMatrix<T>::copy(const T* X, u32 rowOffset, u32 colOffset) {
    require_lt(rowOffset, nRows_);
    require_lt(colOffset, nColumns_);
    if (gpuMode_ && isComputing_){
	int result = Cuda::copyToGpu(d_elem_ + colOffset * nRows_ + rowOffset, X, (size_t) (nColumns_ * nRows_ - colOffset * nRows_ - rowOffset));
	require_eq(result, 0);
    }
    else
	Precursor::copy(X, rowOffset, colOffset);
}

template<typename T>
void CudaMatrix<T>::copy(const std::vector<T> &X, u32 rowOffset, u32 colOffset) {
    require_lt(rowOffset, nRows_);
    require_lt(colOffset, nColumns_);
    if (gpuMode_ && isComputing_){
	int result = Cuda::copyToGpu(d_elem_ + colOffset * nRows_ + rowOffset, &X.at(0), X.size());
	require_eq(result, 0);
    }
    else
	Precursor::copy(X, rowOffset, colOffset);
}

// copy matrix from conventional sprint matrices
template<typename T>
template<typename S>
void CudaMatrix<T>::copy(const Matrix<S> &matrix){
    require(!isComputing_);
    Precursor::copy(matrix);
}


template<typename T>
template<typename S>
void CudaMatrix<T>::convert(Matrix<S> &matrix) const {
    require(!isComputing_);
    Precursor::convert(matrix);
}


template<typename T>
void CudaMatrix<T>::multiply(const CudaVector<T> &x, CudaVector<T> &y, bool transposed, T alpha, T beta, u32 lda) const {
    require(isComputing_);
    if (gpuMode_) {
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
	// TODO checks with non-default lda ?
	int result = Cuda::gemv(cublasHandle, transposed, nRows_, nColumns_, alpha, d_elem_, lda, x.d_elem_, 1, beta, y.d_elem_, 1);
	require_eq(result, 0);
    } else {
	Precursor::multiply(x, y, transposed, alpha, beta, lda);
    }
}

template<typename T>
void CudaMatrix<T>::addOuterProduct(const CudaVector<T>& x, const CudaVector<T> &y, T alpha, u32 lda) {
    require(isComputing_);
    if (gpuMode_) {
	require_eq(x.size(), nRows_);
	require_eq(y.size(), nColumns_);
	require_le(lda, nRows_);
	if (lda == 0)
	    lda = nRows_;
	int result = Cuda::ger(cublasHandle, nRows_, nColumns_, alpha, x.d_elem_, 1, y.d_elem_, 1, d_elem_, lda);
	require_eq(result, 0);
    } else {
	Precursor::addOuterProduct(x, y, alpha, lda);
    }
}

template<typename T>
void CudaMatrix<T>::elementwiseMultiplication(const CudaMatrix<T> &X) {
    require(isComputing_);
    require(X.isComputing_);
    require_eq(X.nRows(), nRows_);
    require_eq(X.nColumns(), nColumns_);
    if (gpuMode_){
	Cuda::elementwiseMultiplication(d_elem_, X.d_elem_, X.nRows_, X.nColumns_);
    }
    else
	Precursor::elementwiseMultiplication(X);
}

template<typename T>
void CudaMatrix<T>::elementwiseDivision(const CudaMatrix<T> &X) {
    require(isComputing_);
    require(X.isComputing_);
    require_eq(X.nRows(), nRows_);
    require_eq(X.nColumns(), nColumns_);
    if (gpuMode_){
	Cuda::elementwiseDivision(d_elem_, X.d_elem_, X.nRows_, X.nColumns_);
    }
    else
	Precursor::elementwiseDivision(X);
}

template<typename T>
void CudaMatrix<T>::addConstantElementwise(T c) {
    require(isComputing_);
    if (gpuMode_)
	Cuda::addConstantElementwise(c, d_elem_, nRows_, nColumns_);
    else
	Precursor::addConstantElementwise(c);
}

template<typename T>
void CudaMatrix<T>::addToAllColumns(const CudaVector<T> &v, T alpha) {
    require(isComputing_);
    require(v.isComputing_);
    require_eq(v.nRows(), nRows_);
    if (gpuMode_) {
	Cuda::addToAllColumns(d_elem_, v.d_elem_, nRows_, nColumns_, alpha);
    } else {
	Precursor::addToAllColumns(v, alpha);
    }
}

template<typename T>
void CudaMatrix<T>::addToAllRows(const CudaVector<T> &v, T alpha) {
    require(isComputing_);
    require(v.isComputing_);
    require_eq(v.nRows(), nColumns_);
    if (gpuMode_) {
	Cuda::addToAllRows(d_elem_, v.d_elem_, nRows_, nColumns_, alpha);
    } else {
	Precursor::addToAllRows(v, alpha);
    }
}

template<typename T>
void CudaMatrix<T>::getRow(u32 rowIndex, Math::CudaVector<T> &row) const {
    require(isComputing_);
    require(row.isComputing_);
    require_lt(rowIndex, nRows_);
    row.resize(nColumns_);

    if (gpuMode_){
	int result = Cuda::copy(cublasHandle, nColumns_, d_elem_ + rowIndex, nRows_, row.d_elem_, 1);
	require_eq(result, 0);
    }
    else
	Precursor::getRow(rowIndex, row);
}

template<typename T>
void CudaMatrix<T>::getColumn(u32 columnIndex, Math::CudaVector<T> &column) const {
    require(isComputing_);
    require(column.isComputing_);
    require_lt(columnIndex, nColumns_);
    column.resize(nRows_);

    if (gpuMode_){
	int result = Cuda::copy(cublasHandle, nRows_, d_elem_ + columnIndex * nRows_, 1, column.d_elem_, 1);
	require_eq(result, 0);
    }
    else
	Precursor::getColumn(columnIndex, column);
}

template<typename T>
void CudaMatrix<T>::setToZero(){
    if (gpuMode_ && isComputing_){
	int result = Cuda::memSet(d_elem_, 0, nRows_ * nColumns_);
	require_eq(result, 0);
    }
    else
	Precursor::setToZero();
}

template<typename T>
void CudaMatrix<T>::multiplyColumnsByScalars(const CudaVector<T> &scalars){
    require_eq(scalars.size(), nColumns_);
    if (gpuMode_){
	Cuda::multiplyColumnsByScalars(scalars.d_elem_, d_elem_, nRows_, nColumns_);
    }
    else{
	Precursor::multiplyColumnsByScalars(scalars);
    }
}

template<typename T>
void CudaMatrix<T>::divideColumnsByScalars(const CudaVector<T> &scalars){
    require_eq(scalars.size(), nColumns_);
    if (gpuMode_){
	Cuda::divideColumnsByScalars(scalars.d_elem_, d_elem_, nRows_, nColumns_);
    }
    else{
	Precursor::divideColumnsByScalars(scalars);
    }
}

template<typename T>
void CudaMatrix<T>::multiplyRowsByScalars(const CudaVector<T> &scalars){
    require_eq(scalars.size(), nRows_);
    if (gpuMode_){
	Cuda::multiplyRowsByScalars(scalars.d_elem_, d_elem_, nRows_, nColumns_);
    }
    else{
	Precursor::multiplyRowsByScalars(scalars);
    }
}

template<typename T>
void CudaMatrix<T>::divideRowsByScalars(const CudaVector<T> &scalars){
    require_eq(scalars.size(), nRows_);
    if (gpuMode_){
	Cuda::divideRowsByScalars(scalars.d_elem_, d_elem_, nRows_, nColumns_);
    }
    else{
	Precursor::divideRowsByScalars(scalars);
    }
}


template<typename T>
void CudaMatrix<T>::show() {
    require(!isComputing_);
    Precursor::show();
}

template<typename T>
void CudaMatrix<T>::syncAndShow() {
    if (isComputing_ && gpuMode_){
	int result = Cuda::copyFromGpu(elem_, d_elem_, nRows_ * nColumns_);
	require_eq(result, 0);
    }
    Precursor::show();
}

template<typename T>
void CudaMatrix<T>::clear() {
    if (gpuMode_ && d_elem_) {
	Cuda::free(d_elem_);
	d_elem_ = 0;
    }
    Precursor::clear();
}

template<typename T>
void CudaMatrix<T>::copyBlockFromMatrix(const Math::FastMatrix<T> &X, u32 rowIndexX, u32 colIndexX, u32 thisRowIndex, u32 thisColIndex, u32 nRows, u32 nCols){
    require(!isComputing_);
    Precursor::copyBlockFromMatrix(X, rowIndexX, colIndexX, thisRowIndex, thisColIndex, nRows, nCols);
}

template<typename T>
void CudaMatrix<T>::copyBlockFromMatrix(const Math::CudaMatrix<T> &X, u32 rowIndexX, u32 colIndexX, u32 thisRowIndex, u32 thisColIndex, u32 nRows, u32 nCols){
    require(isComputing_);
    require(X.isComputing_); // TODO implementation for !isComputing
    require_le(thisColIndex + nCols, nColumns_);
    require_le(thisRowIndex + nRows, nRows_);
    require_le(colIndexX + nCols, X.nColumns_);
    require_le(rowIndexX + nRows, X.nRows_);
    if (gpuMode_){
	for (u32 column = 0; column < nCols; column++){
	    const T *posX = X.d_elem_ + (colIndexX + column)* X.nRows_ + rowIndexX;
	    T * posThis = d_elem_ + (thisColIndex + column)* nRows_ + thisRowIndex;
	    Cuda::copy(cublasHandle, nRows, posX, 1, posThis, 1);
	}
    }
    else{
	for (u32 column = 0; column < nCols; column++){
	    const T *posX =  X.elem_ + rowIndexX +  (colIndexX  + column) * X.nRows_;
	    T * posThis = elem_ + thisRowIndex  + (thisColIndex + column) * nRows_;
	    Math::copy(nRows, posX, 1, posThis, 1);
	}
    }
}

template<typename T>
void CudaMatrix<T>::binaryDivergenceSoftmaxGradient(const CudaMatrix<T> &Y, const CudaVector<u32> &A) {
    require(isComputing_);
    require(Y.isComputing_);
    if (gpuMode_){
	Cuda::binaryDivergenceSoftmaxGradient(d_elem_, nRows_, nColumns_, Y.d_elem_, A.d_elem_);
    }
    else
	Precursor::binaryDivergenceSoftmaxGradient(Y, A);
}


/**
 *
 *  matrix multiplication
 *
 *  interface supports multi-precision
 *  for the case with unique precision: use CUBLAS directly
 *  multi-precision case: work-around: convert single-precision matrices to double and use CUBLAS double precision routine
 *
 */

template<typename T>
template<typename S>
void CudaMatrix<T>::addMatrixProduct(const CudaMatrix<S> &matrixA, const CudaMatrix<S> &matrixB,
	T scaleC, S scaleA, bool transposedA, bool transposedB){
    require(isComputing_);
    require(matrixA.isComputing_);
    require(matrixB.isComputing_);
    if (gpuMode_){
	u32 m = transposedA ? matrixA.nColumns_ : matrixA.nRows_;
	u32 n = transposedB ? matrixB.nRows_ : matrixB.nColumns_;
	u32 k = transposedA ? matrixA.nRows_ : matrixA.nColumns_;
	require_eq(m, nRows_);
	require_eq(n, nColumns_);
	require_eq(k, (transposedB ? matrixB.nColumns_ : matrixB.nRows_));
	int result = Cuda::gemm(cublasHandle, transposedA, transposedB, m, n, k, scaleA, matrixA.d_elem_, matrixA.nRows_,
		matrixB.d_elem_, matrixB.nRows_, scaleC, d_elem_, nRows_);
	require_eq(result, 0);
    }
    else
	Precursor::addMatrixProduct(matrixA, matrixB, scaleC, scaleA, transposedA, transposedB);
}

template<>
template<>
inline void CudaMatrix<double>::addMatrixProduct(const CudaMatrix<float> &matrixA, const CudaMatrix<float> &matrixB,
	double scaleC, float scaleA, bool transposedA, bool transposedB){
    require(isComputing_);
    require(matrixA.isComputing_);
    require(matrixB.isComputing_);
    if (gpuMode_){
	// simplified multi precision implementation
	require(!transposedA);

	u32 m = transposedA ? matrixA.nColumns_ : matrixA.nRows_;
	u32 n = transposedB ? matrixB.nRows_ : matrixB.nColumns_;
	u32 k = transposedA ? matrixA.nRows_ : matrixA.nColumns_;
	require_eq(m, nRows_);
	require_eq(n, nColumns_);
	require_eq(k, (transposedB ? matrixB.nColumns_ : matrixB.nRows_));

	// scale C
	scale(scaleC);

	// divide matrices into blocks
	u32 nBlocks = std::ceil((1.0 * k) / multiPrecisionBunchSize);
	float *tmpC_d = 0;
	int result = Cuda::alloc(tmpC_d, nColumns_ * nRows_);
	require_eq(result, 0);

	if (!transposedB){
	    // allocate tmp memory for B block
	    float *tmpB_d = 0;
	    result = Cuda::alloc(tmpB_d, nColumns_ * multiPrecisionBunchSize);
	    require_eq(result, 0);

	    // iterate over sub-blocks of matrix
	    for (u32 blockIdx = 0; blockIdx < nBlocks; ++blockIdx){
		int blockSize = blockIdx == nBlocks - 1 ? (k - (nBlocks - 1)*multiPrecisionBunchSize) : multiPrecisionBunchSize;
		// block of A
		const float *blockA = matrixA.d_elem_ + blockIdx * matrixA.nRows_ * multiPrecisionBunchSize;

		// copy block of B
		for (int i = 0; i < blockSize; ++i){
		    int rowIndex = blockIdx * multiPrecisionBunchSize + i;
		    Cuda::copy(cublasHandle, nColumns_, matrixB.d_elem_ + rowIndex, matrixB.nRows_, tmpB_d + i, blockSize);
		}

		// multiply blocks
		result = Cuda::gemm(cublasHandle, false, false, m, n, blockSize,
			scaleA, blockA, matrixA.nRows_,
			tmpB_d, blockSize,
			0.0f, tmpC_d, nRows_);
		require_eq(result, 0);

		// add to C
		result = Cuda::axpy(cublasHandle, nColumns_ * nRows_, 1.0f, tmpC_d, 1, d_elem_, 1);
		require_eq(result, 0);
	    }
	    // free tmp memory
	    Cuda::free(tmpB_d);
	}
	else{
	    // iterate over sub-blocks of matrix
	    for (u32 blockIdx = 0; blockIdx < nBlocks; ++blockIdx){
		int blockSize = blockIdx == nBlocks - 1 ? (k - (nBlocks - 1)*multiPrecisionBunchSize) : multiPrecisionBunchSize;
		// blocks of A and B
		const float *blockA = matrixA.d_elem_ + blockIdx * matrixA.nRows_ * multiPrecisionBunchSize;
		const float *blockB = matrixB.d_elem_ + blockIdx * matrixB.nRows_ * multiPrecisionBunchSize;

		// multiply blocks
		result = Cuda::gemm(cublasHandle, false, true, m, n, blockSize,
			scaleA, blockA, matrixA.nRows_,
			blockB, matrixB.nRows_,
			0.0f, tmpC_d, nRows_);
		require_eq(result, 0);

		// add to C
		result = Cuda::axpy(cublasHandle, nColumns_ * nRows_, 1.0f, tmpC_d, 1, d_elem_, 1);
		require_eq(result, 0);
	    }
	}
	// free tmp memory
	Cuda::free(tmpC_d);

    }
    else
	Precursor::addMatrixProduct(matrixA, matrixB, scaleC, scaleA, transposedA, transposedB);
}

} // namespace Math

#endif /* CUDAMATRIX_HH_ */
