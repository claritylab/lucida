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
#ifndef CUDAKERNELS_HH_
#define CUDAKERNELS_HH_

// mixed precision methods
void _cuda_axpy(int n, float alpha, const float *x, double *y);
void _cuda_axpy(int n, double alpha, const double *x, float *y);
void _cuda_cast(int n, const float *x, double *y);

// own kernels
template<typename T>
void _cuda_exp(T *data, unsigned int nRows, unsigned int nColumns);

// log
template<typename T>
void _cuda_log(T *data, unsigned int nRows, unsigned int nColumns);

// tanh
template<typename T>
void _cuda_tanh(T *data, unsigned int nRows, unsigned int nColumns);

template<typename T>
void _cuda_sigmoid(T gamma, T *data, unsigned int nRows, unsigned int nColumns);

template<typename T>
void _cuda_addSummedRows(T *vectorDevPtr, const T *matrixDevPtr, unsigned int nRows, unsigned int nColumns, const T scale);

template<typename T>
void _cuda_addSummedRows(T *vectorDevPtr, const T *matrixDevPtr, unsigned int nRows, unsigned int nColumns, T *tmpDevPtr, unsigned int tmpRows, const T scale);

template<typename T, typename S>
void _cuda_addSummedColumns(T *vectorDevPtr, const S *matrixDevPtr, unsigned int nRows, unsigned int nColumns, const S scale);

// square matrix elementwise and add sum of columns to vector
template<typename T>
void _cuda_addSquaredSummedColumns(T *vectorDevPtr, const T *matrixDevPtr, unsigned int nRows, unsigned int nColumns, const T scale);

template<typename T>
void _cuda_elementwiseMultiplication(T *data, T *datab, unsigned int nRows, unsigned int nColumns);

template<typename T>
void _cuda_elementwiseDivision(T *data, T *datab, unsigned int nRows, unsigned int nColumns);

template<typename T>
void _cuda_addConstantElementwise(T constant, T *data, unsigned int nRows, unsigned int nColumns);

template<typename T>
void _cuda_getMaxOfColumns(T *vectorDevPtr, const T *matrixDevPtr, unsigned int nRows, unsigned int nColumns);

template<typename T>
void _cuda_getMaxOfColumns(T *vectorDevPtr, const T *matrixDevPtr, unsigned int nRows, unsigned int nColumns, T *tmpDevPtr, unsigned int tmpRows);

template<typename T>
void _cuda_elementwiseMultiplicationWithSigmoidDerivative(T *data, T *datab, unsigned int nRows, unsigned int nColumns);

template<typename T>
void _cuda_elementwiseMultiplicationWithTanhDerivative(T *data, T *datab, unsigned int nRows, unsigned int nColumns);

template<typename T>
void _cuda_multiplicationWithSoftmaxDerivative(T *data, T *datab, T *datac, unsigned int nRows, unsigned int nColumns);

template <typename T>
void _cuda_elementwiseMultiplicationWithRectifiedDerivative(T *data, T *datab, unsigned int nRows, unsigned int nColumns);

template<typename T>
void _cuda_addToAllColumns(T *data, T *datab, unsigned int nRows, unsigned int nColumns, T alpha);

template<typename T>
void _cuda_addToAllRows(T *data, T *datab, unsigned int nRows, unsigned int nColumns, T alpha);

template<typename T>
void _cuda_multiplyColumnsByScalars(const T *vectorDevPtr, T *matrixDevPtr, unsigned int nRows, unsigned int nColumns);

template<typename T>
void _cuda_divideColumnsByScalars(const T *vectorDevPtr, T *matrixDevPtr, unsigned int nRows, unsigned int nColumns);

template<typename T>
void _cuda_multiplyRowsByScalars(const T *vectorDevPtr, T *matrixDevPtr, unsigned int nRows, unsigned int nColumns);

template<typename T>
void _cuda_divideRowsByScalars(const T *vectorDevPtr, T *matrixDevPtr, unsigned int nRows, unsigned int nColumns);

template<typename T>
void _cuda_fill(T *devPtr, T value, unsigned int nRows, unsigned int nColumns);

template<typename T>
void _cuda_ensureMinimalValue(T *devPtr, T value, unsigned int nRows, unsigned int nColumns);

template<typename T>
void _cuda_nClassificationErrors(T *devPtr, unsigned int nRows, unsigned int nColumns, unsigned int *alignmentDevPtr, unsigned int *resultDev);

template<typename T>
void _cuda_crossEntropyObjectiveFunction(T *matrixPtr, unsigned int nRows, unsigned int nColumns, unsigned int *alignmentDevPtr, T *resultDev);

template<typename T>
void _cuda_weightedCrossEntropyObjectiveFunction(T *matrixPtr, unsigned int nRows, unsigned int nColumns, unsigned int *alignmentDevPtr, T *resultDev, T *weights);

template<typename T>
void _cuda_squaredErrorObjectiveFunction(T *matrixPtr, unsigned int nRows, unsigned int nColumns, unsigned int *alignmentDevPtr, T *resultDev);

template<typename T>
void _cuda_weightedSquaredErrorObjectiveFunction(T *matrixPtr, unsigned int nRows, unsigned int nColumns, unsigned int *alignmentDevPtr, T *resultDev, T *weights);

template<typename T>
void _cuda_binaryDivergenceObjectiveFunction(T *matrixPtr, unsigned int nRows, unsigned int nColumns, unsigned int *alignmentDevPtr, T *resultDev);

template<typename T>
void _cuda_weightedBinaryDivergenceObjectiveFunction(T *matrixPtr, unsigned int nRows, unsigned int nColumns, unsigned int *alignmentDevPtr, T *resultDev, T *weights);

template<typename T>
void _cuda_binaryDivergenceSoftmaxGradient(T *matrixPtr, unsigned int nRows, unsigned int nColumns, const T *outputDevPtr, const unsigned int *alignmentDevPtr);

template<typename T>
void _cuda_addKroneckerDelta(T *matrixPtr, unsigned int nRows, unsigned int nColumns, const unsigned int *alignmentDevPtr,  T scale);

template<typename T>
void _cuda_appendSecondOrderFeatures(const T *X, unsigned int nRowsX, unsigned int nColumnsX, T *Y, unsigned int nRowsY, unsigned int offset);

template<typename T>
void _cuda_appendThirdOrderFeatures(const T *X, unsigned int nRowsX, unsigned int nColumnsX, T *Y, unsigned int nRowsY, unsigned int offset);

template<typename T>
void _cuda_dropout(T *X, const T *mask, unsigned int nRows, unsigned int nColumns, T dropoutProbability);

template<typename T>
void _cuda_l1clipping(T *X, unsigned int nRows, unsigned int nColumns, T value);

#endif /* CUDAKERNELS_HH_ */
