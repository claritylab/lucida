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
#ifndef _MATH_MATRIX_TOOLS_HH_
#define _MATH_MATRIX_TOOLS_HH

#include <Math/Matrix.hh>
#include <Math/Vector.hh>
#include "nr.h"


namespace Math { namespace Nr {

    const double TINY=1.0e-20;
    const double MINLOGEPS=11.513f;
    const double svdThreshold=1.0e-12;

    template<typename T, class P>  void invert(Math::Matrix<T,P>& mat){
	require(mat.nRows()==mat.nColumns());

	Math::Matrix<T,P> inv_mat(mat.nRows());
	double d=1.0;
	Math::Vector<int> indx(mat.nRows());
	Math::Vector<double> col (mat.nRows(),0.0);

	Math::Matrix<double> tempMat(mat);

	ludcmp(tempMat,indx, d);
	for (u32 j=0; j<tempMat.nRows(); j++){
	    for(u32 i=0; i<tempMat.nRows(); i++) col[i]=0.0;
	    col[j]=1.0;
	    lubksb(tempMat,indx,col);
	    for(u32 i=0; i<tempMat.nRows(); i++) inv_mat[i][j]=col[i];
	}
	mat=inv_mat;
    }

    template<typename T, class P> void pseudoInvert(Math::Matrix<T,P>& mat){
	require(mat.nRows()==mat.nColumns());

	Math::Matrix<T,P> inv_mat(mat.nRows()),V(mat.nRows()),U(mat);
	Math::Vector<double> w(mat.nRows());
	Math::Vector<double> col (mat.nRows(),0.0);
	Math::Vector<double> x(mat.nRows(),0.0);

	svdcmp(U,w,V);
	double wMax=*std::max_element(w.begin(), w.end());
	double wMin=wMax*svdThreshold;

	for (Math::Vector<double>::iterator p=w.begin();p!=w.end(); ++p)
	    if (*p < wMin) (*p)=0;

	for (u32 j=0; j<mat.nRows(); j++){
	    for(u32 i=0; i<mat.nRows(); i++) col[i]=0.0;
	    col[j]=1.0;
	    svbksb(U,w,V,col,x);
	    for(u32 i=0; i<mat.nRows(); i++) inv_mat[i][j]=x[i];
	}
	mat=inv_mat;
    }




    template<typename T, class P>
    void pseudoInverseProduct(Math::Matrix<T,P> result, const Math::Matrix<T,P>& ls, const Math::Matrix<T,P>& rs) {
	require(ls.nRows()==ls.nColumns());
	require(ls.nColumns()==rs.nRows());
	Math::Matrix<T,P> V(ls.nRows()),U(ls);
	Math::Vector<double> w(ls.nRows());
	Math::Vector<double> x(ls.nRows(),0.0);

	svdcmp(U,w,V);

	double wMax=*std::max_element(w.begin(), w.end());
	double wMin=wMax*svdThreshold;

	for (Math::Vector<double>::iterator p=w.begin();p!=w.end(); ++p)
	    if (*p < wMin) (*p)=0;


	result.resize(ls.nRows(),rs.nColumns());
	for (u32 j=0; j<rs.nColumns(); j++){
	    svbksb(U,w,V,rs.column(j),x);
	    for(u32 i=0; i<ls.nRows(); i++) result[i][j]=x[i];
	}
	return result;
    }



    template<typename T, class P> T logDeterminant(const Math::Matrix<T,P>& mat){
	require(mat.nRows()==mat.nColumns());

	double d=1.0;
	Math::Vector<int> indx(mat.nRows());

	Math::Matrix<double> tmp_mat(mat);
	ludcmp(tmp_mat,indx, d);
	d=0.0;
	for (u32 i=0; i< mat.nRows(); i++) d+=log(fabs(tmp_mat[i][i]));
	return d;
    }

    template<typename T, class P> T determinant(const Math::Matrix<T,P>& mat){
	require(mat.nRows()==mat.nColumns());

	double d=1.0;
	Math::Vector<int> indx(mat.nRows());

	Math::Matrix<T,P> tmp_mat(mat);
	ludcmp(tmp_mat,indx, d);
	for (u32 i=0; i< mat.nRows(); i++) d*=tmp_mat[i][i];
	return d;
    }
} } // namespace Math::Nr

#endif // _MATH_MATRIX_TOOLS_HH_
