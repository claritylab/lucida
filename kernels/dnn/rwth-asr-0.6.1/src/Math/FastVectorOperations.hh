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
#ifndef FAST_VECTOR_OPERATIONS_HH_
#define FAST_VECTOR_OPERATIONS_HH_

#include <Modules.hh>

#include <Core/OpenMPWrapper.hh>
#include <Math/MultithreadingHelper.hh>
#include <functional>
#include <cmath>


namespace Math {


// TODO add multithreading for all methods

/*
 *  y = exp(x) (componentwise)
 *
 */

template <typename T>
inline void vr_exp(int n, T *x, T *y){
    for (int i = 0; i < n; i++){
	y[i] = exp(x[i]);
    }
}


template <typename T>
inline void mt_vr_exp(int n, T *x, T *y, int nThreads){
#pragma omp parallel for
    for (int i = 0; i < n; i++){
	y[i] = exp(x[i]);
    }
}


// TODO add Intel MKL


/*
 *  y = log(x) (componentwise)
 */

template <typename T>
inline void vr_log(int n, T *x, T *y){
    for (int i = 0; i < n; i++){
	y[i] = log(x[i]);
    }
}


/*
 *  z = x**y (componentwise)
 */

template <typename T>
inline void vr_powx(int n, T *x, T y, T *z){
    for (int i = 0; i < n; i++){
	z[i] = pow(x[i], y);
    }
}


} // namespace math

#endif
