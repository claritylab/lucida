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
#ifndef _MATH_LAPACK_SVD_HH_
#define _MATH_LAPACK_SVD_HH_

#include "Lapack.hh"
#include <Math/Matrix.hh>

namespace Math { namespace Lapack {

    typedef Math::Matrix<double> DoubleMatrix;
    typedef Math::Vector<double> DoubleVector;

    const double svdThreshold=1.0e-12;
    const int workArrayFactor=5;

    s32 pseudoInvert(DoubleMatrix& result,const DoubleMatrix& A);
    s32 solveLinearLeastSquares(DoubleVector& result,const DoubleMatrix& A, const DoubleVector& b);
    s32 solveLinearLeastSquares(DoubleMatrix& result,const DoubleMatrix& A, const DoubleMatrix& B);
    s32 svd(DoubleMatrix &U, DoubleVector &W, DoubleMatrix &V,const DoubleMatrix& A); //returns V, not(!) VT

} }

#endif // _MATH_LAPACK_SVD_HH_
