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
#ifndef FASTMATRIXMULT_HH_
#define FASTMATRIXMULT_HH_

#include <Math/Blas.hh>

namespace Signal {

/*
 * Matrix multiplication node that makes use of the Blas level 2 routine
 * limitations: matrix update not possible, only single feature stream
 * Plans: use feature buffering in order to make use of the Blas level 3 routine (matrix-matrix multiplication)
 *
 */
template<typename T>
class FastMatrixMultiplicationNode : public Flow::SleeveNode {
    typedef Flow::SleeveNode Precursor;
protected:
    u32 nRows_;  // number of rows of matrix
    u32 nCols_;  // number of columns of matrix
    T *matrix_;  // matrix stored in row major order
    std::string matrixFilename_; // filename of matrix
    bool needInit_; // need to load matrix from file
public:
    static std::string filterName() { return std::string("signal-fast-matrix-multiplication-") + Core::Type<T>::name; };

    FastMatrixMultiplicationNode(const Core::Configuration &c);
    virtual ~FastMatrixMultiplicationNode();
public:
    // load matrix and set dimensions
    void initialize();
    // set matrix filename from flow file
    virtual bool setParameter(const std::string &name, const std::string &value);
    // apply matrix
    virtual bool work(Flow::PortId p);
};


template<typename T>
FastMatrixMultiplicationNode<T>::FastMatrixMultiplicationNode(const Core::Configuration &c) :
Core::Component(c),
Precursor(c),
nRows_(0),
nCols_(0),
matrix_(0),
needInit_(true)
{}

template<typename T>
FastMatrixMultiplicationNode<T>::~FastMatrixMultiplicationNode()
{
    if (matrix_)
	delete matrix_;
}

template<typename T>
bool FastMatrixMultiplicationNode<T>::setParameter(const std::string &name, const std::string &value) {
    if (paramMatrixMultiplicationFileName.match(name))
	matrixFilename_ = paramMatrixMultiplicationFileName(value);
    else
	return false;
    return true;
}

template<typename T>
void FastMatrixMultiplicationNode<T>::initialize() {
    if (needInit_) {
	// use Math::Matrix for IO
	Math::Matrix<T> naiveMatrix;
	if (matrixFilename_.empty()) {
	    this->error() << "Matrix filename is empty.";
	} else {
	    if (!Math::Module::instance().formats().read(
		    matrixFilename_, naiveMatrix)) {
		error("Failed to read matrix from file '%s'.", matrixFilename_.c_str());
		return;
	    }
	}
	nRows_ = naiveMatrix.nRows();
	nCols_ = naiveMatrix.nColumns();
	matrix_ = new T[nRows_ * nCols_];
	// copy matrix into array, use row-major format
	for (u32 i = 0; i < nRows_; i++){
	    for (u32 j = 0; j < nCols_; j++){
		matrix_[i * nCols_ + j] = naiveMatrix[i][j];
	    }
	}
    }
    needInit_ = false;
}

template<typename T>
bool FastMatrixMultiplicationNode<T>::work(Flow::PortId p) {
    Flow::DataPtr<Flow::Vector<T> > in;
    if (! getData(0, in))
	return SleeveNode::putData(0, in.get());

    if (needInit_)
	initialize();

    /* check the size of the two vectors */
    if (in->size() != nCols_)
	error() << "dimension missmatch: dimension of vector is " << in->size()
	<< ", expected " << nCols_;

    // create output object and reserve space
    Flow::DataPtr<Flow::Vector<T> > out(new Flow::Vector<T>(nRows_));
    out->setTimestamp(*in);

    // apply matrix, use Blas matrix-vector multiplication routine
    T *vector = &(in->at(0));
    Math::gemv<T>(CblasRowMajor, CblasNoTrans, nRows_, nCols_, 1.0, matrix_, nCols_, vector, 1, 0.0, &(out->at(0)), 1);

    return putData(0, out.get());
}

} // namespace Signal

#endif
