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
#ifndef _MATH_MATH_LAPACK_MATRIX_HH
#define _MATH_MATH_LAPACK_MATRIX_HH

#include <Core/Types.hh>

namespace Math { namespace Lapack {

    /** Base class for matrix storage schemes in 1-D buffer */

    class MatrixStorageScheme {
    private:
	size_t nRows_;
	size_t nColumns_;
    public:
	MatrixStorageScheme(size_t nRows = 0, size_t nColumns = 0) : nRows_(nRows), nColumns_(nColumns) {}

	size_t nRows() const { return nRows_; }
	size_t nColumns() const { return nColumns_; }
    };


    /** Calculates the index of the matrix element [row][column] in a 1-D buffer
     *    in which elements are stored column after column
     */

    struct ColumnByColumnStorage : public MatrixStorageScheme {
	ColumnByColumnStorage(size_t nRows = 0, size_t nColumns = 0) :
	    MatrixStorageScheme(nRows, nColumns) {}

	size_t operator()(size_t row, size_t column) const { return column * nRows() + row; }
	size_t storageSize() const { return nRows() * nColumns(); }
    };


    /** Calculates the index of the matrix element [row][column] in a 1-D buffer
     *    in which elements are stored column after column
     */

    struct RowByRowStorage : public MatrixStorageScheme {
	RowByRowStorage(size_t nRows = 0, size_t nColumns = 0) : MatrixStorageScheme(nRows, nColumns) {}

	size_t operator()(size_t row, size_t column) const { return row * nColumns() + column; }
	size_t storageSize() const { return nRows() * nColumns(); }
    };


    /** MatrixIterator provides matrix-like access to a 1-D buffer */

    template<class T, class S = ColumnByColumnStorage>
    class Matrix {
    public:
	typedef T Type;
	typedef S StorageScheme;
    private:
	Type* buffer_;
	StorageScheme storageScheme_;
    private:
	void deleteBuffer() {
	    delete[] buffer_;
	    buffer_ = 0;
	    storageScheme_ = StorageScheme(0, 0);
	}
    public:
	Matrix() : buffer_(0) {}
	Matrix(size_t n, size_t m) : buffer_(0) { resize(n, m); }
	Matrix(const Matrix &matrix) : buffer_(0) { operator=(matrix); }
	~Matrix() { deleteBuffer(); };

	Matrix& operator=(const Matrix &matrix) {
	    resize(matrix.nRows(), matrix.nColumns());
	    std::copy(matrix.buffer_, matrix.buffer_ + storageScheme_.storageSize(), buffer_);
	    return *this;
	}

	/**
	 *  @param n is number of rows
	 *  @param m is number of columns
	 */
	void resize(size_t n, size_t m) {
	    StorageScheme newStorageScheme(n, m);
	    if (storageScheme_.storageSize() == newStorageScheme.storageSize())
		return;
	    deleteBuffer();
	    if (newStorageScheme.storageSize() > 0)
		buffer_ = new Type[(storageScheme_ = newStorageScheme).storageSize()];
	}
	size_t nRows() const { return storageScheme_.nRows(); };
	size_t nColumns() const { return storageScheme_.nColumns(); };

	Type& operator()(size_t row, size_t column) {
	    return buffer_[storageScheme_(row, column)];
	}

	const Type& operator()(size_t row, size_t column) const {
	    return buffer_[storageScheme_(row, column)];
	}

	Type* buffer() { return buffer_; };
    };

    template<class T, class Source>
    void copy(const Source &source, Matrix<T> &target) {
	target.resize(source.nRows(), source.nColumns());
	for(size_t row = 0; row < source.nRows(); ++ row) {
	    for(size_t column = 0; column < source.nColumns(); ++ column) {
		target(row, column) = source[row][column];
	    }
	}
    }

    template<class T, class Source>
    void copy(const Source &source, Matrix<T> &target, size_t nColumns) {
	nColumns = std::min(nColumns, source.nColumns());
	target.resize(source.nRows(), nColumns);
	for(size_t row = 0; row < source.nRows(); ++ row) {
	    for(size_t column = 0; column < nColumns; ++ column) {
		target(row, column) = source[row][column];
	    }
	}
    }

    template<class T, class Target>
    void copy(const Matrix<T> &source, Target &target) {
	target.resize(source.nRows(), source.nColumns());
	for(size_t row = 0; row < source.nRows(); ++ row) {
	    for(size_t column = 0; column < source.nColumns(); ++ column) {
		target[row][column] = source(row, column);
	    }
	}
    }

    template<class T, class Target>
    void copy(const Matrix<T> &source, Target &target, size_t nColumns) {
	nColumns = std::min(nColumns, source.nColumns());
	target.resize(source.nRows(), nColumns);
	for(size_t row = 0; row < source.nRows(); ++ row) {
	    for(size_t column = 0; column < nColumns; ++ column) {
		target[row][column] = source(row, column);
	    }
	}
    }

} } //namespace Math::Lapack

#endif //_MATH_LAPACK_MATRIX_HH
