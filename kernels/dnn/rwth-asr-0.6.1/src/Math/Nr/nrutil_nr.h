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
#ifndef _MATH_NR_UTIL_H_
#define _MATH_NR_UTIL_H_

#include <string>
#include <cmath>
#include <complex>
#include <iostream>

namespace Math { namespace Nr {

using namespace std;

typedef double DP;

template<class T>
inline const T SQR(const T a) {return a*a;}

#ifdef OBSOLETE
template<class T>
inline const T MAX(const T &a, const T &b)
        {return b > a ? (b) : (a);}

inline float MAX(const double &a, const float &b)
        {return b > a ? (b) : float(a);}

inline float MAX(const float &a, const double &b)
        {return b > a ? float(b) : (a);}

template<class T>
inline const T MIN(const T &a, const T &b)
        {return b < a ? (b) : (a);}

inline float MIN(const double &a, const float &b)
        {return b < a ? (b) : float(a);}

inline float MIN(const float &a, const double &b)
        {return b < a ? float(b) : (a);}
#endif

template<class T>
inline const T SIGN(const T &a, const T &b)
	{return b >= 0 ? (a >= 0 ? a : -a) : (a >= 0 ? -a : a);}

inline float SIGN(const float &a, const double &b)
	{return b >= 0 ? (a >= 0 ? a : -a) : (a >= 0 ? -a : a);}

inline float SIGN(const double &a, const float &b)
	{return b >= 0 ? (a >= 0 ? a : -a) : (a >= 0 ? -a : a);}

template<class T>
inline void SWAP(T &a, T &b)
	{T dum=a; a=b; b=dum;}


	inline void nrerror(const string error_text)
	// Numerical Recipes standard error handler
	{
		cerr << "Numerical Recipes run-time error..." << endl;
		cerr << error_text << endl;
		cerr << "...now exiting to system..." << endl;
		exit(1);
	}

template <class T>
class NRVec {
private:
	int nn;	// size of array. upper index is nn-1
	T *v;
public:
	NRVec();
	explicit NRVec(int n);		// Zero-based array
	NRVec(const T &a, int n);	//initialize to constant value
	NRVec(const T *a, int n);	// Initialize to array
	NRVec(const NRVec &rhs);	// Copy constructor
	NRVec & operator=(const NRVec &rhs);	//assignment
	NRVec & operator=(const T &a);	//assign a to every element
	inline T & operator[](const int i);	//i'th element
	inline const T & operator[](const int i) const;
	inline int size() const;
	~NRVec();
};

template <class T>
NRVec<T>::NRVec() : nn(0), v(0) {}

template <class T>
NRVec<T>::NRVec(int n) : nn(n), v(new T[n]) {}

template <class T>
NRVec<T>::NRVec(const T& a, int n) : nn(n), v(new T[n])
{
	for(int i=0; i<n; i++)
		v[i] = a;
}

template <class T>
NRVec<T>::NRVec(const T *a, int n) : nn(n), v(new T[n])
{
	for(int i=0; i<n; i++)
		v[i] = *a++;
}

template <class T>
NRVec<T>::NRVec(const NRVec<T> &rhs) : nn(rhs.nn), v(new T[nn])
{
	for(int i=0; i<nn; i++)
		v[i] = rhs[i];
}

template <class T>
NRVec<T> & NRVec<T>::operator=(const NRVec<T> &rhs)
// postcondition: normal assignment via copying has been performed;
//		if vector and rhs were different sizes, vector
//		has been resized to match the size of rhs
{
	if (this != &rhs)
	{
		if (nn != rhs.nn) {
			if (v != 0) delete [] (v);
			nn=rhs.nn;
			v= new T[nn];
		}
		for (int i=0; i<nn; i++)
			v[i]=rhs[i];
	}
	return *this;
}

template <class T>
NRVec<T> & NRVec<T>::operator=(const T &a)	//assign a to every element
{
	for (int i=0; i<nn; i++)
		v[i]=a;
	return *this;
}

template <class T>
inline T & NRVec<T>::operator[](const int i)	//subscripting
{
	return v[i];
}

template <class T>
inline const T & NRVec<T>::operator[](const int i) const	//subscripting
{
	return v[i];
}

template <class T>
inline int NRVec<T>::size() const
{
	return nn;
}

template <class T>
NRVec<T>::~NRVec()
{
	if (v != 0)
		delete[] (v);
}

template <class T>
class NRMat {
private:
	int nn;
	int mm;
	T **v;
public:
	NRMat();
	NRMat(int n, int m);			// Zero-based array
	NRMat(const T &a, int n, int m);	//Initialize to constant
	NRMat(const T *a, int n, int m);	// Initialize to array
	NRMat(const NRMat &rhs);		// Copy constructor
	NRMat & operator=(const NRMat &rhs);	//assignment
	NRMat & operator=(const T &a);		//assign a to every element
	inline T* operator[](const int i);	//subscripting: pointer to row i
	inline const T* operator[](const int i) const;
	inline int nrows() const;
	inline int ncols() const;
	~NRMat();
};

template <class T>
NRMat<T>::NRMat() : nn(0), mm(0), v(0) {}

template <class T>
NRMat<T>::NRMat(int n, int m) : nn(n), mm(m), v(new T*[n])
{
	v[0] = new T[m*n];
	for (int i=1; i< n; i++)
		v[i] = v[i-1] + m;
}

template <class T>
NRMat<T>::NRMat(const T &a, int n, int m) : nn(n), mm(m), v(new T*[n])
{
	int i,j;
	v[0] = new T[m*n];
	for (i=1; i< n; i++)
		v[i] = v[i-1] + m;
	for (i=0; i< n; i++)
		for (j=0; j<m; j++)
			v[i][j] = a;
}

template <class T>
NRMat<T>::NRMat(const T *a, int n, int m) : nn(n), mm(m), v(new T*[n])
{
	int i,j;
	v[0] = new T[m*n];
	for (i=1; i< n; i++)
		v[i] = v[i-1] + m;
	for (i=0; i< n; i++)
		for (j=0; j<m; j++)
			v[i][j] = *a++;
}

template <class T>
NRMat<T>::NRMat(const NRMat &rhs) : nn(rhs.nn), mm(rhs.mm), v(new T*[nn])
{
	int i,j;
	v[0] = new T[mm*nn];
	for (i=1; i< nn; i++)
		v[i] = v[i-1] + mm;
	for (i=0; i< nn; i++)
		for (j=0; j<mm; j++)
			v[i][j] = rhs[i][j];
}

template <class T>
NRMat<T> & NRMat<T>::operator=(const NRMat<T> &rhs)
// postcondition: normal assignment via copying has been performed;
//		if matrix and rhs were different sizes, matrix
//		has been resized to match the size of rhs
{
	if (this != &rhs) {
		int i,j;
		if (nn != rhs.nn || mm != rhs.mm) {
			if (v != 0) {
				delete[] (v[0]);
				delete[] (v);
			}
			nn=rhs.nn;
			mm=rhs.mm;
			v = new T*[nn];
			v[0] = new T[mm*nn];
		}
		for (i=1; i< nn; i++)
			v[i] = v[i-1] + mm;
		for (i=0; i< nn; i++)
			for (j=0; j<mm; j++)
				v[i][j] = rhs[i][j];
	}
	return *this;
}

template <class T>
NRMat<T> & NRMat<T>::operator=(const T &a)	//assign a to every element
{
	for (int i=0; i< nn; i++)
		for (int j=0; j<mm; j++)
			v[i][j] = a;
	return *this;
}

template <class T>
inline T* NRMat<T>::operator[](const int i)	//subscripting: pointer to row i
{
	return v[i];
}

template <class T>
inline const T* NRMat<T>::operator[](const int i) const
{
	return v[i];
}

template <class T>
inline int NRMat<T>::nrows() const
{
	return nn;
}

template <class T>
inline int NRMat<T>::ncols() const
{
	return mm;
}

template <class T>
NRMat<T>::~NRMat()
{
	if (v != 0) {
		delete[] (v[0]);
		delete[] (v);
	}
}

template <class T>
class NRMat3d {
private:
	int nn;
	int mm;
	int kk;
	T ***v;
public:
	NRMat3d();
	NRMat3d(int n, int m, int k);
	inline T** operator[](const int i);	//subscripting: pointer to row i
	inline const T* const * operator[](const int i) const;
	inline int dim1() const;
	inline int dim2() const;
	inline int dim3() const;
	~NRMat3d();
};

template <class T>
NRMat3d<T>::NRMat3d(): nn(0), mm(0), kk(0), v(0) {}

template <class T>
NRMat3d<T>::NRMat3d(int n, int m, int k) : nn(n), mm(m), kk(k), v(new T**[n])
{
	int i,j;
	v[0] = new T*[n*m];
	v[0][0] = new T[n*m*k];
	for(j=1; j<m; j++)
		v[0][j] = v[0][j-1] + k;
	for(i=1; i<n; i++) {
		v[i] = v[i-1] + m;
		v[i][0] = v[i-1][0] + m*k;
		for(j=1; j<m; j++)
			v[i][j] = v[i][j-1] + k;
	}
}

template <class T>
inline T** NRMat3d<T>::operator[](const int i) //subscripting: pointer to row i
{
	return v[i];
}

template <class T>
inline const T* const * NRMat3d<T>::operator[](const int i) const
{
	return v[i];
}

template <class T>
inline int NRMat3d<T>::dim1() const
{
	return nn;
}

template <class T>
inline int NRMat3d<T>::dim2() const
{
	return mm;
}

template <class T>
inline int NRMat3d<T>::dim3() const
{
	return kk;
}

template <class T>
NRMat3d<T>::~NRMat3d()
{
	if (v != 0) {
		delete[] (v[0][0]);
		delete[] (v[0]);
		delete[] (v);
	}
}

//The next 3 classes are used in artihmetic coding, Huffman coding, and
//wavelet transforms respectively. This is as good a place as any to put them!

class arithcode {
private:
	NRVec<unsigned long> *ilob_p,*iupb_p,*ncumfq_p;
public:
	NRVec<unsigned long> &ilob,&iupb,&ncumfq;
	unsigned long jdif,nc,minint,nch,ncum,nrad;
	arithcode(unsigned long n1, unsigned long n2, unsigned long n3)
		: ilob_p(new NRVec<unsigned long>(n1)),
		iupb_p(new NRVec<unsigned long>(n2)),
		ncumfq_p(new NRVec<unsigned long>(n3)),
		ilob(*ilob_p),iupb(*iupb_p),ncumfq(*ncumfq_p) {}
	~arithcode() {
		if (ilob_p != 0) delete ilob_p;
		if (iupb_p != 0) delete iupb_p;
		if (ncumfq_p != 0) delete ncumfq_p;
	}
};

class huffcode {
private:
	NRVec<unsigned long> *icod_p,*ncod_p,*left_p,*right_p;
public:
	NRVec<unsigned long> &icod,&ncod,&left,&right;
	int nch,nodemax;
	huffcode(unsigned long n1, unsigned long n2, unsigned long n3,
		unsigned long n4) :
		icod_p(new NRVec<unsigned long>(n1)),
		ncod_p(new NRVec<unsigned long>(n2)),
		left_p(new NRVec<unsigned long>(n3)),
		right_p(new NRVec<unsigned long>(n4)),
		icod(*icod_p),ncod(*ncod_p),left(*left_p),right(*right_p) {}
	~huffcode() {
		if (icod_p != 0) delete icod_p;
		if (ncod_p != 0) delete ncod_p;
		if (left_p != 0) delete left_p;
		if (right_p != 0) delete right_p;
	}
};

class wavefilt {
private:
	NRVec<DP> *cc_p,*cr_p;
public:
	int ncof,ioff,joff;
	NRVec<DP> &cc,&cr;
	wavefilt() : cc(*cc_p),cr(*cr_p) {}
	wavefilt(const DP *a, const int n) :  //initialize to array
		cc_p(new NRVec<DP>(n)),cr_p(new NRVec<DP>(n)),
		ncof(n),ioff(-(n >> 1)),joff(-(n >> 1)),cc(*cc_p),cr(*cr_p) {
			int i;
			for (i=0; i<n; i++)
				cc[i] = *a++;
			DP sig = -1.0;
			for (i=0; i<n; i++) {
				cr[n-1-i]=sig*cc[i];
				sig = -sig;
			}
	}
	~wavefilt() {
		if (cc_p != 0) delete cc_p;
		if (cr_p != 0) delete cr_p;
	}
};

//Overloaded complex operations to handle mixed float and double
//This takes care of e.g. 1.0/z, z complex<float>

inline const complex<float> operator+(const double &a,
	const complex<float> &b) { return float(a)+b; }

inline const complex<float> operator+(const complex<float> &a,
	const double &b) { return a+float(b); }

inline const complex<float> operator-(const double &a,
	const complex<float> &b) { return float(a)-b; }

inline const complex<float> operator-(const complex<float> &a,
	const double &b) { return a-float(b); }

inline const complex<float> operator*(const double &a,
	const complex<float> &b) { return float(a)*b; }

inline const complex<float> operator*(const complex<float> &a,
	const double &b) { return a*float(b); }

inline const complex<float> operator/(const double &a,
	const complex<float> &b) { return float(a)/b; }

inline const complex<float> operator/(const complex<float> &a,
	const double &b) { return a/float(b); }

//some compilers choke on pow(float,double) in single precision. also atan2

// inline float pow (float x, double y) {return std::pow(double(x),y);}
// inline float pow (double x, float y) {return std::pow(x,double(y));}
// inline float atan2 (float x, double y) {return std::atan2(double(x),y);}
// inline float atan2 (double x, float y) {return std::atan2(x,double(y));}

} } // namespace Math::Nr

#endif /*_MATH_NR_UTIL_H_ */
