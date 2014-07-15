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
#ifndef _CORE_MATRIX_HH
#define _CORE_MATRIX_HH

#include "Vector.hh"
#include <Core/Assertions.hh>
#include <Core/XmlStream.hh>
#include <iostream>
#include <sstream>
#include <complex>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <cmath>

namespace Math {

     /**
     * Class for two-dimensional arrays
     */

    template <class T,class P = VectorRangeCheckOffPolicy>
    class Matrix {
    public:
	typedef T Type;
    private:
	size_t nRows_;
	size_t nColumns_;
	Vector<Vector<T, P>,P > elem_;
    public:
	Matrix() { nRows_ = nColumns_ = 0; }
	Matrix(int n, int m) { resize(n,m); }
	explicit Matrix(int n){ resize(n); }
	template<class S> Matrix(const Matrix<S> &x) { operator=(x); }
	Matrix(const Vector<T, P> &a, const Vector<T, P> &b);
	Matrix(const Vector<T, P> &a, size_t n, size_t m);

	template<class S>
	Matrix<T> &operator=(const Matrix<S> &x) {
	    resize(x.nRows(), x.nColumns());
	    for (size_t i = 0; i < nRows_; ++i)
		for (size_t j = 0; j < nColumns_; ++j)
		  elem_[i][j] = T(x[i][j]);
	    return *this;
	}

	size_t nRows() const { return nRows_; }
	Vector<T, P> row(size_t row) const {return elem_[row];}
	size_t nColumns() const { return nColumns_; }
	Vector<T, P> column(size_t col)const;
	bool empty() const { return nRows() == 0 || nColumns() == 0; }
	size_t size() const { return nRows_*nColumns_; }

	/** @return is (row, column) index of the first not-equal elements.
	 *  If none, @return is (nRows(), nColumns()).
	 */
	std::pair<size_t, size_t> isAlmostEqual(const Matrix<T, P> &m, T tolerance) const;

	void resize(size_t nRows, size_t nColumns, T init = T(0));
	/** Set matrix dimension to i x i. */
	void resize(int i) { resize(i, i); }
	void clear() { resize(0); }
	void fill(T x);

	void addRow(const Vector<T, P>& v);
	void insertRow(size_t position, const Vector<T, P>& v);
	void setRow(size_t position, const Vector<T, P>& v);
	void removeRow(size_t position);

	void addColumn(const Vector<T, P>& v);
	void insertColumn(size_t position, const Vector<T, P>& v);
	void setColumn(size_t position, const Vector<T, P>& v);
	void removeColumn(size_t position);

	void setDiagonal(const Vector<T, P> &diagonal);
	const Vector<T, P> diagonal() const;

	// T determinant() const ;
	// T logDeterminant() const ;
      //Matrix<T, P> invert() const ;
	Matrix<T, P> transpose() const;
	T trace() const;
	T productTrace(const Matrix<T, P> &rs) const;
	T transposedProductTrace(const Matrix<T, P> &rs) const;
	T maxElement() const;
	T l2Norm() const;
	void squareVector(const std::vector<T> & vec);
	T vvt(const Vector<T, P>&);
	/**
	 *  Returns @c norm-th norm of column @c column..
	 */
	T columnNorm(size_t column, f64 norm);
	/**
	 *  Ensures that $c norm-th norm of each column is 1.
	 */
	void normalizeColumns(f64 norm);
	/**
	 *  Ensures that first non-zero element of each column vector is positive.
	 */
	void normalizeColumnDirection();
	void permuteColumns(const std::vector<size_t> &permutation);

	Matrix operator*(const T &scalar) const;
	Matrix operator/(const T &scalar) const;
	Vector<T, P> operator*(const Vector<T, P>& v) const;
	Matrix<T, P> operator*(const Matrix<T, P> &rs) const;
	Matrix<T, P> multiplyElementwise(const Matrix<T, P> &m) const;
	Matrix<T, P> operator+(const Matrix<T, P>&) const;
	Matrix<T, P>& operator+=(const Matrix<T, P>&);
	Matrix<T, P> operator-(const Matrix<T, P>&) const;
	Matrix<T, P>& operator-=(const Matrix<T, P>&);
	Matrix<T, P>& operator*=(const T&);
	Matrix<T, P>& operator/=(const T &v) { return operator*=((T)1 / v); }

	Vector<T, P> &operator[](size_t i) { P().checkRange(elem_, i); return elem_[i]; }
	const Vector<T, P> &operator[](size_t i) const { P().checkRange(elem_, i); return elem_[i]; }

	void write(Core::BinaryOutputStream &) const;
	void write(Core::XmlWriter &) const;
	void read(Core::BinaryInputStream &);
	void read(std::istream &);

	void print(std::ostream & = std::cout) const;
	void print_large(std::ostream & = std::cout) const;
	void print_maple(std::ostream &os = std::cout, u16 prec = 3) const;
	void print_raw(std::ostream &os = std::cout) const;
    };

    template<class T, class P>
    Matrix<T, P>::Matrix(const Vector<T, P> &a,const Vector<T, P> &b)
    {
	resize(a.size(),b.size());
	for(size_t i = 0; i < nRows(); ++ i)
	    for(size_t j = 0; j < nColumns(); ++ j)
		elem_[i][j]=a[i]*b[j];
    }

    template<class T, class P>
    Matrix<T, P>::Matrix(const Vector<T, P> &a, size_t n, size_t m)
    {
	require(a.size() == n * m);
	resize(n,m);
	for(size_t i = 0; i < nRows(); ++ i)
	    for(size_t j = 0; j < nColumns(); ++ j)
		elem_[i][j]=a[i*nColumns()+j];
    }

    template<class T, class P>
    Vector<T, P> Matrix<T, P>::column(size_t col) const
    {
	Vector<T, P> result(nRows_);
	for(size_t i = 0; i < nRows(); ++ i) result[i]=elem_[i][col];
	return result;
    }

    template<class T, class P>
    std::pair<size_t, size_t> Matrix<T, P>::isAlmostEqual(const Matrix<T, P> &m, T tolerance) const
    {
	require(nRows() == m.nRows() && nColumns() == m.nColumns());
	for(size_t row = 0; row < nRows(); ++ row) {
	    for(size_t column = 0; column < nColumns(); ++ column) {
		if (!Core::isAlmostEqual(elem_[row][column], m[row][column], tolerance))
		    return std::make_pair(row, column);
	    }
	}
	return std::make_pair(nRows(), nColumns());
    }

    template<class T, class P>
    void Matrix<T, P>::resize(size_t nRows, size_t nColumns, T init) {
	elem_.resize(nRows_ = nRows);
	nColumns_ = nColumns;
	for(size_t row = 0; row < nRows_; ++ row)
	    elem_[row].resize(nColumns_, init);
    }

    template<class T, class P>
    void Matrix<T, P>::addRow(const Vector<T, P> &v)
    {
	require(nRows() == 0 || v.size() == nColumns());
	if (nRows() == 0)
	    nColumns_ = v.size();
	elem_.push_back(v);
	++nRows_;
    }

    template<class T, class P>
    void Matrix<T, P>::insertRow(size_t position, const Vector<T, P> &v)
    {
	require(position <= nRows());
	require(v.size() == nColumns() || nRows() == 0);

	if (nRows() == 0)
	    nColumns_ = v.size();
	elem_.insert(elem_.begin() + position, v);
	++nRows_;
    }

    template<class T, class P>
    void Matrix<T, P>::setRow(size_t position, const Vector<T, P> &v)
    {
	require(position < nRows() && v.size() == nColumns());
	elem_[position] = v;
    }

    template<class T, class P>
    void Matrix<T, P>::removeRow(size_t position)
    {
	require(nRows() != 0);
	elem_.erase(elem_.begin()+position);
	--nRows_;
    }

    template<class T, class P>
    void Matrix<T, P>::addColumn(const Vector<T, P> &v)
    {
	require(nColumns() == 0 || v.size() ==nRows());
	if (nColumns() == 0) {
	    elem_.resize(v.size());
	    nRows_=v.size();
	}
	for (size_t row = 0; row < nRows(); row ++)
	    elem_[row].push_back(v[row]);
	++nColumns_;
    }

    template<class T, class P>
    void Matrix<T, P>::insertColumn(size_t position, const Vector<T, P> &v)
    {
	require(nColumns() == 0 || v.size() == nRows());
	if (nColumns() == 0){
	    elem_.resize(v.size());
	    nRows_ = v.size();
	}
	for (size_t row = 0; row < nRows(); row ++)
	    elem_[row].insert(elem_[row].begin() + position,v[row]);
	++nColumns_;
    }

    template<class T, class P>
    void Matrix<T, P>::setColumn(size_t position, const Vector<T, P> &v)
    {
	require(position < nColumns() && v.size() == nRows());
	for (size_t row = 0; row < nRows(); row ++)
	    elem_[row][position] = v[row];
    }

    template<class T, class P>
    void  Matrix<T, P>::removeColumn(size_t position)
    {
	require(nColumns() != 0);
	for(size_t row = 0; row < nRows(); ++ row)
	    elem_[row].erase(elem_[row].begin() + position);
	--nColumns_;
    }

    template<class T, class P>
    void Matrix<T, P>::setDiagonal(const Vector<T, P> &diagonal)
    {
	require(std::min(nRows(), nColumns()) == diagonal.size());
	for (size_t i = 0; i < diagonal.size(); ++ i)
	    elem_[i][i] = diagonal[i];
    }

    template<class T, class P>
    const Vector<T, P> Matrix<T, P>::diagonal() const
    {
	Vector<T, P> result(std::min(nRows(), nColumns()));
	for (size_t i = 0;  i < result.size(); i++)
	    result[i] = elem_[i][i];
	return result;
    };

    template<class T, class P>
    void Matrix<T, P>::fill(T x)
    {
	typename Vector<Vector<T, P>, P>::iterator i;
	for(i = elem_.begin(); i != elem_.end(); ++i)
	    std::fill(i->begin(), i->end(), x);
    }

    template<class T, class P>
    Matrix<T, P> Matrix<T, P>::multiplyElementwise(const Matrix<T, P> &m) const
    {
	require(nRows() == m.nRows() && nColumns() == m.nColumns());
	Matrix<T> result(nRows(),nColumns());
	for(size_t row = 0; row < nRows(); ++row)
	    for(size_t col = 0; col < nColumns(); ++ col)
		result[row][col] = elem_[row][col] * m[row][col];
	return result;
    };

    template<class T, class P>
    Matrix<T, P> Matrix<T, P>::transpose() const
    {
	Matrix<T, P> mat(nColumns_,nRows_);
	    for(size_t i=0;  i < nColumns(); i++)
		for(size_t j=0; j < nRows(); j++)
		    mat[i][j] = elem_[j][i];
	return mat;
    }

    template<class T, class P>
    T Matrix<T, P>::trace() const
    {
	T trace = (T) 0.0f;
	require(nRows_==nColumns_);
	for (size_t j = 0; j < nRows(); j++) trace += elem_[j][j];
	return(trace);
    }

    template<class T, class P>
    T Matrix<T, P>::productTrace(const Matrix<T, P> &rs) const
    {
	T trace = (T) 0.0f;
	require(nRows_==rs.nColumns_ && nColumns_==rs.nRows_);
	for (u16 i = 0; i < nRows_; i++)
	    for (u16 j =0; j< nColumns_; j++)
		trace += elem_[i][j]*rs[j][i];
	return(trace);
    }

    template<class T, class P>
    T Matrix<T, P>::transposedProductTrace(const Matrix<T, P> &rs) const
    {
	T trace = (T) 0.0f;
	require(nRows_==rs.nColumns_ && nColumns_==rs.nRows_);
	for (u16 i = 0; i < nRows_; i++)
	    for (u16 j =0; j< nColumns_; j++)
		trace += elem_[i][j]*rs[i][j];
	return(trace);
    }

    template<class T, class P>
    T Matrix<T, P>::columnNorm(size_t column, f64 norm)
    {
	T result = 0;
	for(size_t row = 0; row < nRows(); ++ row)
	    result += pow(Core::abs(elem_[row][column]), norm);
	return pow(result, (f64)1 / norm);
    }

    template<class T, class P>
    void Matrix<T, P>::normalizeColumns(f64 norm)
    {
	for(size_t column = 0; column < nColumns(); column ++) {
	    T normFactor = columnNorm(column, norm);
	    if (normFactor != 0) {
		for(size_t row = 0; row < nRows(); ++ row)
		    elem_[row][column] /= normFactor;
	    }
	}
    }

    template<class T, class P>
    void Matrix<T, P>::normalizeColumnDirection()
    {
	for(size_t column = 0; column < nColumns(); column ++) {
	    bool changeSign = false;
	    for(size_t row = 0; row < nRows(); ++ row) {
		if (elem_[row][column] != 0) {
		    changeSign = (elem_[row][column] < 0);
		    break;
		}
	    }
	    if (changeSign) {
		for(size_t row = 0; row < nRows(); ++ row)
		    elem_[row][column] = -elem_[row][column];
	    }
	}
    }

    template<class T, class P>
    void Matrix<T, P>::permuteColumns(const std::vector<size_t> &permutation)
    {
	require(permutation.size() == nColumns());
	Matrix<T, P> tmp = *this;
	for(size_t row = 0; row < nRows(); ++ row) {
	    for(size_t column = 0; column < nColumns(); column ++) {
		require_(permutation[column] < tmp.nColumns());
		elem_[row][column] = tmp[row][permutation[column]];
	    }
	}
    }

    template<class T, class P>
    void Matrix<T, P>::squareVector(const std::vector<T> & vec){
	require(nRows()==nColumns());
	for (u32 col=0; col < elem_.size(); col++){
	    for(u32 row=0; row < col; row++){
		elem_[col][row] = vec[col] * vec[row];
		elem_[row][col] = elem_[col][row];
	    }
	    elem_[col][col] = vec[col]*vec[col];
	}
    }

    template<class T, class P>
    T Matrix<T, P>::vvt(const Vector<T, P> &vec)
    {
	T result = (T)0.0f;
	require(nRows_==nColumns_);
	for (size_t col=0; col < elem_.size(); col++)
	    for (size_t row=0; row < elem_.size(); row++)
		result += vec[row]*vec[col]*elem_[col][row];
	return(result);
    }

    template<class T, class P>
    T Matrix<T, P>::l2Norm() const
    {
	T norm = (T) 0.0f;
	for(size_t row = 0; row < nRows(); ++ row)
	    for(size_t col = 0; col < nColumns(); ++ col)
		norm += elem_[row][col] * elem_[row][col];
	return norm;
    }

    template<class T, class P>
    T Matrix<T, P>::maxElement() const
    {
	T result = Core::Type<T>::min;
	for (size_t row = 0; row < nRows(); ++row) {
	    for(size_t col = 0; col < nColumns(); ++col) {
		if(elem_[row][col] > result)
		    result = elem_[row][col];
	    }
	}
	return result;
    }

    template<class T, class P>
    Matrix<T, P> Matrix<T, P>::operator*(const T &scalar) const
    {
	Matrix<T, P> result(*this);
	for (size_t n = 0; n < nRows(); ++n)
	    for (size_t m = 0; m < nColumns(); ++m) {
		T &e(result[n][m]);
		e = e * scalar;
	    }
	return result;
    }

    template<class T, class P>
    Matrix<T, P> Matrix<T, P>::operator/(const T &scalar) const
    {
	Matrix<T, P> result(*this);
	for (size_t n = 0; n < nRows(); ++n)
	    for (size_t m = 0; m < nColumns(); ++m) {
		T &e(result[n][m]);
		e = e / scalar;
	    }
	return result;
    }

    template<class T, class P>
    Vector<T, P> Matrix<T, P>::operator*(const Vector<T, P>& v) const
    {
	require(v.size() == nColumns());
	Vector<T, P> result(nRows_);
	for(size_t n=0; n<nRows_; n++)
	    result[n] = elem_[n] * v;
	ensure(result.size() == nRows());
	return result;
    }

    template<class T, class P>
    Matrix<T, P> Matrix<T, P>::operator*(const Matrix<T, P> &rs) const
    {
	require(nColumns_==rs.nRows_);
	Matrix result(nRows_,rs.nColumns_);
	for (size_t n = 0; n < nRows(); n++) {
	    for (size_t m = 0; m < rs.nColumns(); m++) {
		result[n][m] = 0;
		for(size_t k = 0; k < nColumns(); k++)
		    result[n][m] += elem_[n][k] * rs[k][m];
	    }
	}
	return result;
    }

    template<class T, class P>
    Matrix<T, P> Matrix<T, P>::operator+(const Matrix<T, P> &rs) const
    {
	require(nRows() == rs.nRows() && nColumns() == rs.nColumns());
	Matrix result(nRows(), nColumns());
	for (size_t n = 0; n < nRows(); n ++)
	    for (size_t m = 0; m < nColumns(); m ++)
		result[n][m] = elem_[n][m] + rs[n][m];
	return result;
    }

    template<class T, class P>
    Matrix<T, P> &Matrix<T, P>::operator+=(const Matrix<T, P> &rs)
    {
	require(nRows() == rs.nRows() && nColumns() == rs.nColumns());
	for (size_t n = 0; n < nRows(); n ++)
	    for (size_t m = 0; m < nColumns(); m ++)
		elem_[n][m] += rs[n][m];
	return *this;
    }

    template<class T, class P>
    Matrix<T, P> Matrix<T, P>::operator-(const Matrix<T, P> & rs) const
    {
	require(nRows() == rs.nRows() && nColumns() == rs.nColumns());
	Matrix result(nRows(), nColumns());
	for (size_t n = 0; n < nRows(); n ++)
	    for (size_t m = 0; m < nColumns(); m ++)
		result[n][m] = elem_[n][m] - rs[n][m];
	return result;
    }

    template<class T, class P>
    Matrix<T, P> &Matrix<T, P>::operator-= (const Matrix<T, P> & rs)
    {
	require(nRows_==rs.nRows_ && nColumns_==rs.nColumns_);
	for (size_t n=0; n < nRows(); n++)
	    for (size_t m=0; m < nColumns(); m++)
		elem_[n][m] -= rs[n][m];
	return (*this);
    }

    template<class T, class P>
    Matrix<T, P>& Matrix<T, P>::operator*=(const T &weight)
    {
	for (size_t n = 0; n < nRows(); n++)
	    for (size_t m = 0; m < nColumns(); m++)
		elem_[n][m] *= weight;
	return (*this);
    }


    template<class T, class P>
    void Matrix<T, P>::write(Core::BinaryOutputStream &o) const
    {
	o << (u32)nRows_ << (u32)nColumns_; o << elem_;
    }

    template<class T, class P>
    void Matrix<T, P>::read(Core::BinaryInputStream &i)
    {
	u32 s;
	i >> s; nRows_ = s;
	i >> s; nColumns_ = s;
	i >> elem_;
    }

    template<class T, class P>
    void Matrix<T, P>::print(std::ostream &os) const
    {
	for(size_t col = 0; col < elem_.size(); col++){
	    for (size_t row = 0; row < elem_[col].size(); row++){
		os << std::setw(5) << std::setprecision(3)<< elem_[col][row] << " ";
	    }
	    os << std::endl;
	}
    }

    template<class T, class P>
    void Matrix<T, P>::print_large(std::ostream &os) const
    {
	for(size_t col = 0; col < elem_.size(); col++){
	    for (size_t row = 0; row < elem_[col].size(); row++){
		if (elem_[col][row] > 1e-4 && col!=row)
		    os << std::setw(8) << std::setprecision(3)<< "(" <<col<<","<<row<<") " << elem_[col][row] << "\n ";
	    }
	}
    }

    template<class T, class P>
    void Matrix<T, P>::print_raw(std::ostream &os) const
    {
	for(size_t row = 0; row < elem_.size(); row++){
	    for (size_t col = 0; col < elem_[row].size(); col++)
		os << elem_[row][col] << " ";
	    os << std::endl;
	}
    }

    template<class T, class P>
    void Matrix<T, P>::print_maple(std::ostream &os, u16 prec) const
    {
	os << "Matrix([" << std::setprecision(prec) ;
	for(size_t col=0; col < elem_.size(); col++){
	    os <<"[";
	    for (size_t row=0; row<elem_[col].size(); row++){
		os <<  elem_[col][row];
		if (row < elem_[col].size()-1) os << ",";
	    }
	    if (col < elem_.size()-1) os << "],";

	}
	os << "]])" << std::endl;
    }

    template<class T, class P>
    void Matrix<T, P>::read(std::istream &is)
    {
	elem_.clear();
	std::string cutOffLine;
	while (is) {
	    cutOffLine = "";
	    getline(is, cutOffLine);

	    if (!is) break;
	    std::istringstream cutOffLineStream(cutOffLine.c_str());
	    Vector<T, P> rowVector;

	    std::copy(std::istream_iterator<T>(cutOffLineStream), std::istream_iterator<T>(),
		      std::back_inserter(rowVector));
	    if (rowVector.size() != 0)
	      addRow(rowVector);
	}
    }

    template<class T, class P>
    void Matrix<T, P>::write(Core::XmlWriter &os) const
    {
	os << Core::XmlOpen(std::string("matrix-") + Core::Type<T>::name)
	    + Core::XmlAttribute("nRows", nRows())
	    + Core::XmlAttribute("nColumns", nColumns());
	os << setiosflags (std::ios::scientific);
	print_raw(os);
	os << Core::XmlClose(std::string("matrix-") + Core::Type<T>::name);
    }


    // Global function for matrices

    template<class T>
    Matrix<T> makeDiagonalMatrix(size_t dimension, T init = (T)1)
    {
	return makeDiagonalMatrix(Vector<T>(dimension, T(init)));
    }

    template<class T, class P>
    Matrix<T, P> makeDiagonalMatrix(const Vector<T, P> &diagonal)
    {
	Matrix<T, P> result(diagonal.size(), diagonal.size());
	result.fill((T)0);
	result.setDiagonal(diagonal);
	return result;
    }

    template<class T, class P>
    Vector<T, P> diagonal(const Matrix<T, P> &m) { return m.diagonal(); }

    template<class T, class P>
    Matrix<T, P> operator*(const T &scalar, const Matrix<T, P> &matrix)
    {
	return matrix * scalar;
    }

    template<class T, class P>
    Matrix<T, P> multiplyElementwise(const Matrix<T, P> &m1, const Matrix<T, P> &m2)
    {
	return m1.multiplyElementwise(m2);
    }

    template<class T, class P>
    Matrix<T, P> vectorInnerProduct(const Vector<T, P> &l, const Vector<T, P> &r)
    {
	Matrix<T, P> result(l.size(), r.size());
	for (size_t row = 0; row < result.nRows(); ++ row)
	    result.setRow(row, l[row] * r);
	return result;
    }

    template<class T, class P>
    Matrix<T, P> real(const Matrix<std::complex<T>, P> &m)
    {
	Matrix<T, P> result(m.nRows(), m.nColumns());
	for (size_t i = 0; i < m.nRows(); ++i)
	    for(size_t j = 0; j < m.nColumns(); ++j)
		result[i][j] = m[i][j].real();
	return result;
    }

    template<class T, class P>
    Matrix<T, P> imag(const Matrix<std::complex<T>, P> &m)
    {
	Matrix<T, P> result(m.nRows(), m.nColumns());
	for (size_t i = 0; i < m.nRows(); ++i)
	    for(size_t j = 0; j < m.nColumns(); ++j)
		result[i] = m[i][j].imag();
	return result;
    }

    template<class T, class P, class TA, class PA>
    void abs(const Matrix<T ,P> &m, Matrix<TA, PA> &a)
    {
	a(m.nRows(), m.nColumns());
	for (size_t i = 0; i < m.nRows(); ++i)
	    for(size_t j = 0; j < m.nColumns(); ++j)
		a[i] = Core::abs(m[i][j]);
    }

    template<class T, class P>
    Matrix<T, P> abs(const Matrix<T, P> &m)
    {
	Matrix<T, P> a; abs(m, a); return a;
    }

    template<class T, class P>
    Matrix<T, P> abs(const Matrix<std::complex<T>, P> &m)
    {
	Matrix<T, P> a; abs(m, a); return a;
    }

    template<class T, class P, class M>
    void maxAbsoluteElement(const Matrix<T ,P> &m, M &r) {
	r = Core::Type<M>::min;
	for (size_t row = 0; row < m.nRows(); ++ row) {
	    for(size_t col = 0; col < m.nColumns(); ++ col) {
		M absValue = (M)Core::abs(m[row][col]);
		if(absValue > r)
		    r = absValue;
	    }
	}
    }

    template<class T, class P>
    T maxAbsoluteElement(const Matrix<T, P> &m)
    {
	T r; maxAbsoluteElement(m, r); return r;
    }

    template<class T, class P>
    T maxAbsoluteElement(const Matrix<std::complex<T>, P> &m)
    {
	T r; maxAbsoluteElement(m, r); return r;
    }

    template<class T, class P>
    std::istream& operator>>(std::istream &is, Matrix<T, P> &m)
    {
	m.read(is); return is;
    }

    template<class T, class P>
    Core::XmlWriter &operator<<(Core::XmlWriter &os, const Matrix<T, P> &m)
    {
	m.write(os); return os;
    }

    template<class T, class P>
    Core::BinaryOutputStream& operator<< (Core::BinaryOutputStream& o, const Matrix<T, P> &m)
    {
	m.write(o); return o;
    }

    template<class T, class P>
    Core::BinaryInputStream& operator>> (Core::BinaryInputStream& i, Matrix<T, P> &m)
    {
	m.read(i); return i;
    }

    /** Refernce Counted Matrix */
    template<class T, class P = VectorRangeCheckOffPolicy>
    class RefernceCountedMatrix:
	    public Core::ReferenceCounted, public Matrix<T, P>
    {
    public:
	RefernceCountedMatrix():Matrix<T, P>() {}
	RefernceCountedMatrix(int n, int m):Matrix<T, P>(n,m) {}
	RefernceCountedMatrix(int n):Matrix<T, P>(n) {}
	template<class S> RefernceCountedMatrix(const Matrix<S>& x) : Matrix<T, P>(x) {}
    };

} // namespace Math

namespace Core {
    template<class T, class P>
    class NameHelper<Math::Matrix<T, P> > : public std::string {
    public:
	NameHelper() : std::string(std::string("matrix-") + NameHelper<T>()) {}
    };

    template<class T, class P>
    class NameHelper<Math::RefernceCountedMatrix<T, P> > : public std::string {
    public:
	NameHelper() : std::string(std::string("ref-counted-matrix-") + NameHelper<T>()) {}
    };
}

#endif // _Math_MATRIX_HH
