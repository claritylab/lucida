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
#ifndef MULTITHREADINGHELPER_HH_
#define MULTITHREADINGHELPER_HH_

namespace Math {

/**
 *
 * generic multithreading for vector operations
 * useful for BLAS 1 operations or similar which are not automatically
 * parallelized
 *
 * (application would be much easier in C++x11 with std::bind)
 *
 */


/*
 *
 * parallelization of function of type vector + scalar to vector, e.g. axpy
 *
 *
 */



template<typename T>
void mt_sv2v(int N, T alpha, const T *X, T *Y, void (*fn)(int, T, const T*, T*), int nThreads){
    int nParts = nThreads < 1 ? 1 : nThreads;
    nParts = nParts > N ? std::max(1,N) : nParts;
    int d = N / nParts;
    int r = N % nParts;
    if (r == 0){
#pragma omp parallel for
	for (int partId = 0; partId < nParts ; partId++){
	    fn(d, alpha, X + partId * d, Y+ partId * d);
	}
    }
    else{
#pragma omp parallel for
	for (int partId = 0; partId < nParts; partId++){
	    if (partId < nParts - 1)
		fn(d, alpha, X + partId * d, Y + partId * d);
	    else
		fn(d + r, alpha, X + (nParts -1)* d, Y + (nParts -1)* d);
	}
    }
}

/*
 *
 * parallelization of function of type vector to vector, e.g. exp
 * (unfortunately, design error in vr_exp, first vector is not const!)
 *
 */



template<typename T>
void mt_v2v(int N, T *X, T *Y, void (*fn)(int, T*, T*), int nThreads){
    int nParts = nThreads < 1 ? 1 : nThreads;
    nParts = nParts > N ? std::max(1,N) : nParts;

    int d = N / nParts;
    int r = N % nParts;
    if (r == 0){
#pragma omp parallel for
	for (int partId = 0; partId < nParts ; partId++){
	    fn(d, X + partId * d, Y+ partId * d);
	}
    }
    else{
#pragma omp parallel for
	for (int partId = 0; partId < nParts; partId++){
	    if (partId < nParts - 1)
		fn(d, X + partId * d, Y + partId * d);
	    else
		fn(d + r, X + (nParts -1)* d, Y + (nParts -1)* d);
	}
    }
}


/*
 *
 * parallelization of function of type scalar, vector, vector-> scalar, e.g. nrm2, dot, asum
 * reduction with operator +
 *
 */

template<typename T>
T mt_svv2s(int N, T alpha, const T *X, const T *Y, T (*fn)(int, T, const T*, const T*), int nThreads){
    int nParts = nThreads < 1 ? 1 : nThreads;
    nParts = nParts > N ? std::max(1,N) : nParts;

    int d = N / nParts;
    int r = N % nParts;
    T result = 0.0;
    if (r == 0){
#pragma omp parallel for reduction(+:result)
	for (int partId = 0; partId < nParts ; partId++){
	    result += fn(d, alpha, X + partId * d, Y+ partId * d);
	}
    }
    else{
#pragma omp parallel for reduction(+:result)
	for (int partId = 0; partId < nParts; partId++){
	    if (partId < nParts - 1)
		result += fn(d, alpha, X + partId * d, Y+ partId * d);
	    else
		result += fn(d + r, alpha, X + (nParts -1)* d, Y + (nParts -1)* d);
	}
    }
    return result;
}


}

#endif /* MULTITHREADINGHELPER_HH_ */
