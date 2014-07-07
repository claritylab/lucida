#include "stdio.h"
#include "CudaMatrixKernels.hh"
#include <math_constants.h>
#include <cuda_runtime.h>

#ifdef __CDT_PARSER__
#define __global__
#define __device__
#define __host__
#define __shared__
#endif

#define THREADS_PER_BLOCK 1024


/*
 *
 *  mixed precision axpy
 *
 */


__global__ void __cuda_axpy(int nElements, float alpha, const float *x, double *y){
    unsigned  int index = threadIdx.x + blockIdx.x * blockDim.x;
    if (index < nElements)
	y[index] += alpha * x[index];
}

void _cuda_axpy(int nElements, float alpha, const float *x, double *y)
{

    int gridSize = (int)ceil( (float) nElements/THREADS_PER_BLOCK);
    __cuda_axpy <<< gridSize , THREADS_PER_BLOCK >>> (nElements, alpha, x, y);
}

__global__ void __cuda_axpy(int nElements, double alpha, const double *x, float *y){
    unsigned  int index = threadIdx.x + blockIdx.x * blockDim.x;
    if (index < nElements)
	y[index] += alpha * x[index];
}

void _cuda_axpy(int nElements, double alpha, const double *x, float *y)
{

    int gridSize = (int)ceil( (float) nElements/THREADS_PER_BLOCK);
    __cuda_axpy <<< gridSize , THREADS_PER_BLOCK >>> (nElements, alpha, x, y);
}



__global__ void __cuda_cast(int nElements, const float *x, double *y){
    unsigned  int index = threadIdx.x + blockIdx.x * blockDim.x;
    if (index < nElements)
	y[index] = x[index];
}

void _cuda_cast(int nElements, const float *x, double *y)
{

    int gridSize = (int)ceil( (float) nElements/THREADS_PER_BLOCK);
    __cuda_cast <<< gridSize , THREADS_PER_BLOCK >>> (nElements, x, y);
}

/*
 *
 *  exp
 *
 */

// ###########################################################################
// elementwiseMultiplicationWithRectifiedDerivative
template<typename T>
__global__ void __cuda_exp(T *data, unsigned int nElements){
    unsigned  int index = threadIdx.x + blockIdx.x * blockDim.x;
    if (index < nElements)
	data[index] = exp(data[index]);
}

template<typename T>
void _cuda_exp(T *data, unsigned int nRows, unsigned int nColumns)
{

    unsigned int nElements = nRows * nColumns;
    int gridSize = (int)ceil( (float) nElements/THREADS_PER_BLOCK);

    __cuda_exp <<< gridSize , THREADS_PER_BLOCK >>> (data, nElements);
}

template __global__ void __cuda_exp<float>(float *, unsigned int);
template __global__ void __cuda_exp<double>(double *, unsigned int);
template void _cuda_exp<float>(float *, unsigned int, unsigned int);
template void _cuda_exp<double>(double *, unsigned int, unsigned int);

/*
 *
 *  log
 *
 */

template<typename T>
__global__ void __cuda_log(T *data, unsigned int nElements){
    unsigned  int index = threadIdx.x + blockIdx.x * blockDim.x;
    if (index < nElements)
	data[index] = log(data[index]);
}

template<typename T>
void _cuda_log(T *data, unsigned int nRows, unsigned int nColumns)
{

    unsigned int nElements = nRows * nColumns;
    int gridSize = (int)ceil( (float) nElements/THREADS_PER_BLOCK);

    __cuda_log <<< gridSize , THREADS_PER_BLOCK >>> (data, nElements);
}

template __global__ void __cuda_log<float>(float *, unsigned int);
template __global__ void __cuda_log<double>(double *, unsigned int);
template void _cuda_log<float>(float *, unsigned int, unsigned int);
template void _cuda_log<double>(double *, unsigned int, unsigned int);

/*
 *
 * tanh
 *
 *
 */

template<typename T>
__global__ void __cuda_tanh(T *data, unsigned int nElements){
    unsigned  int index = threadIdx.x + blockIdx.x * blockDim.x;
    if (index < nElements)
	data[index] = tanh(data[index]);
}

template<typename T>
void _cuda_tanh(T *data, unsigned int nRows, unsigned int nColumns)
{

    unsigned int nElements = nRows * nColumns;
    int gridSize = (int)ceil( (float) nElements/THREADS_PER_BLOCK);

    __cuda_tanh <<< gridSize , THREADS_PER_BLOCK >>> (data, nElements);
}

template __global__ void __cuda_tanh<float>(float *, unsigned int);
template __global__ void __cuda_tanh<double>(double *, unsigned int);
template void _cuda_tanh<float>(float *, unsigned int, unsigned int);
template void _cuda_tanh<double>(double *, unsigned int, unsigned int);

/*
 *
 * sigmoid
 *
 */

template<typename T>
__global__ void __cuda_sigmoid1(T *data, unsigned int nElements){
    unsigned  int index = threadIdx.x + blockIdx.x * blockDim.x;
    if (index < nElements)
	data[index] = 1.0 / (1.0 + exp(-data[index]));
}

template<typename T>
__global__ void __cuda_sigmoid(T gamma, T *data, unsigned int nElements){
    unsigned  int index = threadIdx.x + blockIdx.x * blockDim.x;
    if (index < nElements)
	data[index] = 1.0 / (1.0 + exp(-gamma * data[index]));
}

template<typename T>
void _cuda_sigmoid(T gamma, T *data, unsigned int nRows, unsigned int nColumns)
{

    unsigned int nElements = nRows * nColumns;
    int gridSize = (int)ceil( (float) nElements/THREADS_PER_BLOCK);
    if (gamma == 1.0)
	__cuda_sigmoid1 <<< gridSize , THREADS_PER_BLOCK >>> (data, nElements);
    else
	__cuda_sigmoid <<< gridSize , THREADS_PER_BLOCK >>> (gamma, data, nElements);
}

template void _cuda_sigmoid<double>(double gamma, double *data, unsigned int nRows, unsigned int nColumns);
template __global__ void __cuda_sigmoid<double>(double gamma, double *data, unsigned int nElements);
template __global__ void __cuda_sigmoid1<double>(double *data, unsigned int nElements);
template void _cuda_sigmoid<float>(float gamma, float *data, unsigned int nRows, unsigned int nColumns);
template __global__ void __cuda_sigmoid<float>(float gamma, float *data, unsigned int nElements);
template __global__ void __cuda_sigmoid1<float>(float *data, unsigned int nElements);




/*
 *
 * addSummedRows
 *
 */
template<typename T>
__global__ void __cuda_addSummedRows(T *vectorDevPtr, const T *matrixDevPtr, unsigned int nRows, unsigned int nColumns, const T scale){
    unsigned  int columnIndex = threadIdx.x + blockIdx.x * blockDim.x;
    if (columnIndex < nColumns){
	float result = 0.0;
	for (unsigned int i = 0; i < nRows; i++){
	    // result += matrix(i,columnIndex)
	    result += matrixDevPtr[columnIndex * nRows + i];
	}
	vectorDevPtr[columnIndex] += scale * result;
    }
}

template<typename T>
void _cuda_addSummedRows(T *vectorDevPtr, const T *matrixDevPtr, unsigned int nRows, unsigned int nColumns, const T scale){

    // parallelize over columns
    int gridSize = (int)ceil( (float) nColumns/THREADS_PER_BLOCK);

    __cuda_addSummedRows <<< gridSize , THREADS_PER_BLOCK >>> (vectorDevPtr, matrixDevPtr, nRows, nColumns, scale);
}

template __global__ void __cuda_addSummedRows(double *vectorDevPtr, const double *matrixDevPtr, unsigned int nRows, unsigned int nColumns, const double scale);
template void _cuda_addSummedRows(double *vectorDevPtr, const double *matrixDevPtr, unsigned int nRows, unsigned int nColumns, const double scale);
template __global__ void __cuda_addSummedRows(float *vectorDevPtr, const float *matrixDevPtr, unsigned int nRows, unsigned int nColumns, const float scale);
template void _cuda_addSummedRows(float *vectorDevPtr, const float *matrixDevPtr, unsigned int nRows, unsigned int nColumns, const float scale);

/*
 * slightly faster version using tmp array
 *
 */
template<typename T>
__global__ void __cuda_summedRowsTmp(const T *matrixDevPtr, unsigned int nRows, unsigned int nColumns,
	T *tmpDevPtr, unsigned int tmpRows){
    unsigned int columnIndex = threadIdx.x + blockIdx.x * blockDim.x;
    unsigned int columnPart = blockIdx.y;
    if (columnIndex < nColumns){
	unsigned int nRowsDiv = nRows / tmpRows;
	unsigned int startRow =  columnPart * nRowsDiv;
	if (startRow < nRows){
	    unsigned int endRow = columnPart == tmpRows - 1 ? nRows : (columnPart + 1) * nRowsDiv;
	    T result = 0.0;
	    for (unsigned int i = startRow; i < endRow; i++){
		// result += matrix(i, columnIndex)
		result += matrixDevPtr[columnIndex * nRows + i];
	    }
	    tmpDevPtr[columnIndex*tmpRows + columnPart] = result;
	}
    }
}

template<typename T>
void _cuda_addSummedRows(T *vectorDevPtr, const T *matrixDevPtr, unsigned int nRows, unsigned int nColumns,
	T *tmpDevPtr, unsigned int tmpRows, const T scale){
    int gridDimx = (int)ceil( (float) nColumns / THREADS_PER_BLOCK);
    int gridDimy = tmpRows;
    dim3 gridSize(gridDimx,gridDimy);
    __cuda_summedRowsTmp <<< gridSize , THREADS_PER_BLOCK >>> (matrixDevPtr, nRows, nColumns, tmpDevPtr, tmpRows);

    _cuda_addSummedRows<T>(vectorDevPtr, tmpDevPtr, tmpRows, nColumns, scale);
}

template __global__ void __cuda_summedRowsTmp<double>(const double *matrixDevPtr, unsigned int nRows, unsigned int nColumns,
	double *tmpDevPtr, unsigned int tmpRows);
template void _cuda_addSummedRows<double>(double *vectorDevPtr, const double *matrixDevPtr, unsigned int nRows, unsigned int nColumns,
	double *tmpDevPtr, unsigned int tmpRows, const double scale);
template __global__ void __cuda_summedRowsTmp<float>(const float *matrixDevPtr, unsigned int nRows, unsigned int nColumns,
	float *tmpDevPtr, unsigned int tmpRows);
template void _cuda_addSummedRows<float>(float *vectorDevPtr, const float *matrixDevPtr, unsigned int nRows, unsigned int nColumns,
	float *tmpDevPtr, unsigned int tmpRows, const float scale);
/*
 *
 * addSummedColumns
 *
 */

template<typename T, typename S>
__global__ void __cuda_addSummedColumns(T *vectorDevPtr, const S *matrixDevPtr, unsigned int nRows, unsigned int nColumns, const S scale){
    unsigned  int rowIndex = threadIdx.x + blockIdx.x * blockDim.x;
    if (rowIndex < nRows){
	T result = 0.0;
	for (unsigned int i = 0; i < nColumns; i++){
	    // result += matrix(rowIndex,i)
	    result += matrixDevPtr[i * nRows + rowIndex];
	}
	vectorDevPtr[rowIndex] += scale * result;
    }
}

template<typename T, typename S>
void _cuda_addSummedColumns(T *vectorDevPtr, const S *matrixDevPtr, unsigned int nRows, unsigned int nColumns, const S scale){
    // parallelize over rows
    int gridSize = (int)ceil( (float) nRows/THREADS_PER_BLOCK);

    __cuda_addSummedColumns <<< gridSize , THREADS_PER_BLOCK >>> (vectorDevPtr, matrixDevPtr, nRows, nColumns, scale);
}

template __global__ void __cuda_addSummedColumns<double, double>(double *vectorDevPtr, const double *matrixDevPtr, unsigned int nRows, unsigned int nColumns, const double scale);
template     void _cuda_addSummedColumns<double, double>(double *vectorDevPtr, const double *matrixDevPtr, unsigned int nRows, unsigned int nColumns, const double scale);
template __global__ void __cuda_addSummedColumns<float, float>(float *vectorDevPtr, const float *matrixDevPtr, unsigned int nRows, unsigned int nColumns, const float scale);
template     void _cuda_addSummedColumns<float, float>(float *vectorDevPtr, const float *matrixDevPtr, unsigned int nRows, unsigned int nColumns, const float scale);
template __global__ void __cuda_addSummedColumns<double, float>(double *vectorDevPtr, const float *matrixDevPtr, unsigned int nRows, unsigned int nColumns, const float scale);
template     void _cuda_addSummedColumns<double, float>(double *vectorDevPtr, const float *matrixDevPtr, unsigned int nRows, unsigned int nColumns, const float scale);



/*
 *
 * addSquaredSummedColumns
 *
 */

template<typename T>
__global__ void __cuda_addSquaredSummedColumns(T *vectorDevPtr, const T *matrixDevPtr, unsigned int nRows, unsigned int nColumns, const T scale){
    unsigned  int rowIndex = threadIdx.x + blockIdx.x * blockDim.x;
    if (rowIndex < nRows){
	T result = 0.0;
	for (unsigned int i = 0; i < nColumns; i++){
	    result += matrixDevPtr[i * nRows + rowIndex] * matrixDevPtr[i * nRows + rowIndex];
	}
	vectorDevPtr[rowIndex] += scale * result;
    }
}

template<typename T>
void _cuda_addSquaredSummedColumns(T *vectorDevPtr, const T *matrixDevPtr, unsigned int nRows, unsigned int nColumns, const T scale){

    // parallelize over rows
    int gridSize = (int)ceil( (float) nRows/THREADS_PER_BLOCK);

    __cuda_addSquaredSummedColumns <<< gridSize , THREADS_PER_BLOCK >>> (vectorDevPtr, matrixDevPtr, nRows, nColumns, scale);
}

template __global__ void __cuda_addSquaredSummedColumns(double *vectorDevPtr, const double *matrixDevPtr, unsigned int nRows, unsigned int nColumns, const double scale);
template void _cuda_addSquaredSummedColumns(double *vectorDevPtr, const double *matrixDevPtr, unsigned int nRows, unsigned int nColumns, const double scale);
template __global__ void __cuda_addSquaredSummedColumns(float *vectorDevPtr, const float *matrixDevPtr, unsigned int nRows, unsigned int nColumns, const float scale);
template void _cuda_addSquaredSummedColumns(float *vectorDevPtr, const float *matrixDevPtr, unsigned int nRows, unsigned int nColumns, const float scale);



/*
 *
 * elementwise multiplication
 *
 */

template<typename T>
__global__ void __cuda_elementwiseMultiplication(T *data, T *datab, unsigned int nElements){
    unsigned  int index = threadIdx.x + blockIdx.x * blockDim.x;
    if (index < nElements)
	data[index] = data[index] * datab[index];
}

template<typename T>
void _cuda_elementwiseMultiplication(T *data, T *datab, unsigned int nRows, unsigned int nColumns)
{

    unsigned int nElements = nRows * nColumns;
    int gridSize = (int)ceil( (float) nElements/THREADS_PER_BLOCK);

    __cuda_elementwiseMultiplication <<< gridSize , THREADS_PER_BLOCK >>> (data, datab, nElements);
}

template __global__ void __cuda_elementwiseMultiplication<double>(double *data, double *datab, unsigned int nElements);
template __global__ void __cuda_elementwiseMultiplication<float>(float *data, float *datab, unsigned int nElements);
template void _cuda_elementwiseMultiplication<double>(double *data, double *datab, unsigned int nRows, unsigned int nColumns);
template void _cuda_elementwiseMultiplication<float>(float *data, float *datab, unsigned int nRows, unsigned int nColumns);

/*
 *
 * elementwise division
 *
 */

template<typename T>
__global__ void __cuda_elementwiseDivision(T *data, T *datab, unsigned int nElements){
    unsigned  int index = threadIdx.x + blockIdx.x * blockDim.x;
    if (index < nElements)
	data[index] = data[index] / datab[index];
}

template<typename T>
void _cuda_elementwiseDivision(T *data, T *datab, unsigned int nRows, unsigned int nColumns)
{

    unsigned int nElements = nRows * nColumns;
    int gridSize = (int)ceil( (float) nElements/THREADS_PER_BLOCK);

    __cuda_elementwiseDivision <<< gridSize , THREADS_PER_BLOCK >>> (data, datab, nElements);
}

template __global__ void __cuda_elementwiseDivision<double>(double *data, double *datab, unsigned int nElements);
template __global__ void __cuda_elementwiseDivision<float>(float *data, float *datab, unsigned int nElements);
template void _cuda_elementwiseDivision<double>(double *data, double *datab, unsigned int nRows, unsigned int nColumns);
template void _cuda_elementwiseDivision<float>(float *data, float *datab, unsigned int nRows, unsigned int nColumns);

/*
 *
 * add constant elementwise
 *
 */
template<typename T>
__global__ void __cuda_addConstantElementwise(T constant, T *data, unsigned int nElements){
    unsigned  int index = threadIdx.x + blockIdx.x * blockDim.x;
    if (index < nElements)
	data[index] = data[index] + constant;
}

template<typename T>
void _cuda_addConstantElementwise(T constant, T *data, unsigned int nRows, unsigned int nColumns)
{

    unsigned int nElements = nRows * nColumns;
    int gridSize = (int)ceil( (T) nElements/THREADS_PER_BLOCK);

    __cuda_addConstantElementwise <<< gridSize , THREADS_PER_BLOCK >>> (constant, data, nElements);
}

template __global__ void __cuda_addConstantElementwise<double>(double constant, double *data, unsigned int nElements);
template void _cuda_addConstantElementwise<double>(double constant, double *data, unsigned int nRows, unsigned int nColumns);
template __global__ void __cuda_addConstantElementwise<float>(float constant, float *data, unsigned int nElements);
template void _cuda_addConstantElementwise<float>(float constant, float *data, unsigned int nRows, unsigned int nColumns);


/*
 *
 * getMaxOfColumns
 *
 */
template<typename T>
__global__ void __cuda_getMaxOfColumns(T *vectorDevPtr, const T *matrixDevPtr, unsigned int nRows, unsigned int nColumns){
    unsigned  int columnIndex = threadIdx.x + blockIdx.x * blockDim.x;
    if (columnIndex < nColumns){
	T result = 0.0;
	for (unsigned int i = 0; i < nRows; i++){
	    // result += matrix(i, columnIndex)
	    T val = matrixDevPtr[columnIndex * nRows + i];
	    result = fmax(result, val);
	}
	vectorDevPtr[columnIndex] = result;
    }
}

template<typename T>
void _cuda_getMaxOfColumns(T *vectorDevPtr, const T *matrixDevPtr, unsigned int nRows, unsigned int nColumns){
    // parallelize over columns
    int gridSize = (int)ceil( (float) nColumns/THREADS_PER_BLOCK);

    __cuda_getMaxOfColumns <<< gridSize , THREADS_PER_BLOCK >>> (vectorDevPtr, matrixDevPtr, nRows, nColumns);
}

template __global__ void __cuda_getMaxOfColumns(double *vectorDevPtr, const double *matrixDevPtr, unsigned int nRows, unsigned int nColumns);
template void _cuda_getMaxOfColumns(double *vectorDevPtr, const double *matrixDevPtr, unsigned int nRows, unsigned int nColumns);
template __global__ void __cuda_getMaxOfColumns(float *vectorDevPtr, const float *matrixDevPtr, unsigned int nRows, unsigned int nColumns);
template void _cuda_getMaxOfColumns(float *vectorDevPtr, const float *matrixDevPtr, unsigned int nRows, unsigned int nColumns);

/*
 * slightly faster version using tmp array
 */

template<typename T>
__global__ void __cuda_getMaxOfColumnsTmp(const T *matrixDevPtr, unsigned int nRows, unsigned int nColumns,
	T *tmpDevPtr, unsigned int tmpRows){
    unsigned int columnIndex = threadIdx.x + blockIdx.x * blockDim.x;
    unsigned int columnPart = blockIdx.y;
    if (columnIndex < nColumns){
	unsigned int nRowsDiv = nRows / tmpRows;
	unsigned int startRow =  columnPart * nRowsDiv;
	if (startRow < nRows){
	    unsigned int endRow = columnPart == tmpRows - 1 ? nRows : (columnPart + 1) * nRowsDiv;
	    T result = 0.0;
	    for (unsigned int i = startRow; i < endRow; i++){
		// result += matrix(i, columnIndex)
		T val = matrixDevPtr[columnIndex * nRows + i];
		result = fmax(result, val);
	    }
	    tmpDevPtr[columnIndex*tmpRows + columnPart] = result;
	}
    }
}

template<typename T>
void _cuda_getMaxOfColumns(T *vectorDevPtr, const T *matrixDevPtr, unsigned int nRows, unsigned int nColumns,
	T *tmpDevPtr, unsigned int tmpRows){
    int gridDimx = (int)ceil( (float) nColumns / THREADS_PER_BLOCK);
    int gridDimy = tmpRows;
    dim3 gridSize(gridDimx,gridDimy);

    __cuda_getMaxOfColumnsTmp <<< gridSize , THREADS_PER_BLOCK >>> (matrixDevPtr, nRows, nColumns, tmpDevPtr, tmpRows);

    _cuda_getMaxOfColumns<T>(vectorDevPtr, tmpDevPtr, tmpRows, nColumns);
}

template __global__ void __cuda_getMaxOfColumnsTmp(const double *matrixDevPtr, unsigned int nRows, unsigned int nColumns,
	double *tmpDevPtr, unsigned int tmpRows);
template void _cuda_getMaxOfColumns(double *vectorDevPtr, const double *matrixDevPtr, unsigned int nRows, unsigned int nColumns,
	double *tmpDevPtr, unsigned int tmpRows);
template __global__ void __cuda_getMaxOfColumnsTmp(const float *matrixDevPtr, unsigned int nRows, unsigned int nColumns,
	float *tmpDevPtr, unsigned int tmpRows);
template void _cuda_getMaxOfColumns(float *vectorDevPtr, const float *matrixDevPtr, unsigned int nRows, unsigned int nColumns,
	float *tmpDevPtr, unsigned int tmpRows);
/*
 *
 * elementwiseMultiplicationWithSigmoidDerivative
 *
 */

template<typename T>
__global__ void __cuda_elementwiseMultiplicationWithSigmoidDerivative(T *data, T *datab, unsigned int nElements){
    unsigned  int index = threadIdx.x + blockIdx.x * blockDim.x;
    if (index < nElements)
	data[index] = data[index] * (datab[index] * (1 - datab[index]));
}

template<typename T>
void _cuda_elementwiseMultiplicationWithSigmoidDerivative(T *data, T *datab, unsigned int nRows, unsigned int nColumns)
{

    unsigned int nElements = nRows * nColumns;
    int gridSize = (int)ceil( (float) nElements/THREADS_PER_BLOCK);

    __cuda_elementwiseMultiplicationWithSigmoidDerivative <<< gridSize , THREADS_PER_BLOCK >>> (data, datab, nElements);
}

template __global__ void __cuda_elementwiseMultiplicationWithSigmoidDerivative(double *data, double *datab, unsigned int nElements);
template void _cuda_elementwiseMultiplicationWithSigmoidDerivative(double *data, double *datab, unsigned int nRows, unsigned int nColumns);
template __global__ void __cuda_elementwiseMultiplicationWithSigmoidDerivative(float *data, float *datab, unsigned int nElements);
template void _cuda_elementwiseMultiplicationWithSigmoidDerivative(float *data, float *datab, unsigned int nRows, unsigned int nColumns);

/*
 *
 * elementwiseMultiplicationWithTanhDerivative
 *
 */

template<typename T>
__global__ void __cuda_elementwiseMultiplicationWithTanhDerivative(T *data, T *datab, unsigned int nElements){
    unsigned  int index = threadIdx.x + blockIdx.x * blockDim.x;
    if (index < nElements)
	data[index] = data[index] * (1 - pow(datab[index],2));
}

template<typename T>
void _cuda_elementwiseMultiplicationWithTanhDerivative(T *data, T *datab, unsigned int nRows, unsigned int nColumns)
{

    unsigned int nElements = nRows * nColumns;
    int gridSize = (int)ceil( (float) nElements/THREADS_PER_BLOCK);

    __cuda_elementwiseMultiplicationWithTanhDerivative <<< gridSize , THREADS_PER_BLOCK >>> (data, datab, nElements);
}

template __global__ void __cuda_elementwiseMultiplicationWithTanhDerivative(double *data, double *datab, unsigned int nElements);
template void _cuda_elementwiseMultiplicationWithTanhDerivative(double *data, double *datab, unsigned int nRows, unsigned int nColumns);
template __global__ void __cuda_elementwiseMultiplicationWithTanhDerivative(float *data, float *datab, unsigned int nElements);
template void _cuda_elementwiseMultiplicationWithTanhDerivative(float *data, float *datab, unsigned int nRows, unsigned int nColumns);

/*
 *
 * multiplicationWithSoftmaxDerivative
 *
 */

template<typename T>
__global__ void __cuda_multiplicationWithSoftmaxDerivative(T *data, T *datab, T *datac, unsigned int nElements, unsigned int nRows){
    unsigned  int index = threadIdx.x + blockIdx.x * blockDim.x;
    if (index < nElements)
	data[index] = datab[index] * (data[index] - datac[index/nRows]);
}

template<typename T>
void _cuda_multiplicationWithSoftmaxDerivative(T *data, T *datab, T *datac, unsigned int nRows, unsigned int nColumns)
{

    unsigned int nElements = nRows * nColumns;
    int gridSize = (int)ceil( (float) nElements/THREADS_PER_BLOCK);

    __cuda_multiplicationWithSoftmaxDerivative <<< gridSize , THREADS_PER_BLOCK >>> (data, datab, datac, nElements, nRows);
}

template __global__ void __cuda_multiplicationWithSoftmaxDerivative(double *data, double *datab, double *datac, unsigned int nElements, unsigned int nRows);
template void _cuda_multiplicationWithSoftmaxDerivative(double *data, double *datab, double *datac, unsigned int nRows, unsigned int nColumns);
template __global__ void __cuda_multiplicationWithSoftmaxDerivative(float *data, float *datab, float *datac, unsigned int nElements, unsigned int nRows);
template void _cuda_multiplicationWithSoftmaxDerivative(float *data, float *datab, float *datac, unsigned int nRows, unsigned int nColumns);


/*
 * elementwiseMultiplicationWithRectifiedDerivative
 *
 */

template <typename T>
__global__ void __cuda_elementwiseMultiplicationWithRectifiedDerivative(T *errOut, T *activations, unsigned int nElements){
    unsigned int index = threadIdx.x + blockIdx.x * blockDim.x;
    if (index < nElements)
	if (activations[index] <= 0) errOut[index] = 0;
}
template <typename T>
void _cuda_elementwiseMultiplicationWithRectifiedDerivative(T *data, T *datab, unsigned int nRows, unsigned int nColumns) {
    unsigned int nElements = nRows * nColumns;
    int gridSize = (int)ceil( (float) nElements/THREADS_PER_BLOCK);
    __cuda_elementwiseMultiplicationWithRectifiedDerivative<T> <<<gridSize, THREADS_PER_BLOCK>>> (data, datab, nElements);
}
template __global__ void __cuda_elementwiseMultiplicationWithRectifiedDerivative<float>(float*, float*, unsigned int);
template __global__ void __cuda_elementwiseMultiplicationWithRectifiedDerivative<double>(double*, double*, unsigned int);
template void _cuda_elementwiseMultiplicationWithRectifiedDerivative<float>(float*, float*, unsigned int, unsigned int);
template void _cuda_elementwiseMultiplicationWithRectifiedDerivative<double>(double*, double*, unsigned int, unsigned int);



/*
 *
 * addToAllColumns
 *
 */


template<typename T>
__global__ void __cuda_addToAllColumns(T *data, T *datab, unsigned int nElements, unsigned int nRows, T alpha){
    unsigned  int index = threadIdx.x + blockIdx.x * blockDim.x;
    if (index < nElements)
	data[index] += alpha * datab[index%nRows];
}

template<typename T>
void _cuda_addToAllColumns(T *data, T *datab, unsigned int nRows, unsigned int nColumns, T alpha)
{
    // TODO implement kernel without % operator (slow on GPU)
    unsigned int nElements = nRows * nColumns;
    int gridSize = (int)ceil( (float) nElements/THREADS_PER_BLOCK);

    __cuda_addToAllColumns <<< gridSize , THREADS_PER_BLOCK >>> (data, datab, nElements, nRows, alpha);
}

template __global__ void __cuda_addToAllColumns<double>(double *data, double *datab, unsigned int nElements, unsigned int nRows, double alpha);
template void _cuda_addToAllColumns<double>(double *data, double *datab, unsigned int nRows, unsigned int nColumns, double alpha);
template __global__ void __cuda_addToAllColumns<float>(float *data, float *datab, unsigned int nElements, unsigned int nRows, float alpha);
template void _cuda_addToAllColumns<float>(float *data, float *datab, unsigned int nRows, unsigned int nColumns, float alpha);

/*
 *
 * addToAllRows
 *
 */
template<typename T>
__global__ void __cuda_addToAllRows(T *data, T *datab, unsigned int nElements, unsigned int nRows, T alpha){
    unsigned  int index = threadIdx.x + blockIdx.x * blockDim.x;
    if (index < nElements)
	data[index] += alpha * datab[index/nRows];
}
template<typename T>
void _cuda_addToAllRows(T *data, T *datab, unsigned int nRows, unsigned int nColumns, T alpha)
{

    unsigned int nElements = nRows * nColumns;
    int gridSize = (int)ceil( (float) nElements/THREADS_PER_BLOCK);

    __cuda_addToAllRows <<< gridSize , THREADS_PER_BLOCK >>> (data, datab, nElements, nRows, alpha);
}

template __global__ void __cuda_addToAllRows<double>(double *data, double *datab, unsigned int nElements, unsigned int nRows, double alpha);
template void _cuda_addToAllRows<double>(double *data, double *datab, unsigned int nRows, unsigned int nColumns, double alpha);
template __global__ void __cuda_addToAllRows<float>(float *data, float *datab, unsigned int nElements, unsigned int nRows, float alpha);
template void _cuda_addToAllRows<float>(float *data, float *datab, unsigned int nRows, unsigned int nColumns, float alpha);

/*
 *
 * multiplyColumnsByScalars
 *
 */
template<typename T>
__global__ void __cuda_multiplyColumnsByScalars(const T *vectorDevPtr, T *matrixDevPtr, unsigned int nRows, unsigned int nElements){
    unsigned int index = threadIdx.x + blockIdx.x * blockDim.x;
    unsigned int colIndex = index / nRows;
    if (index < nElements)
	matrixDevPtr[index] = matrixDevPtr[index] * vectorDevPtr[colIndex];
}
template<typename T>
void _cuda_multiplyColumnsByScalars(const T *vectorDevPtr, T *matrixDevPtr, unsigned int nRows, unsigned int nColumns){
    // TODO parallelization without mod operator (mod is slow on GPU)
    unsigned int nElements = nRows * nColumns;
    int gridSize = (int)ceil( (float) nElements/THREADS_PER_BLOCK);

    __cuda_multiplyColumnsByScalars <<< gridSize , THREADS_PER_BLOCK >>> (vectorDevPtr, matrixDevPtr, nRows, nElements);
}

template __global__ void __cuda_multiplyColumnsByScalars<double>(const double *vectorDevPtr, double *matrixDevPtr, unsigned int nRows, unsigned int nElements);
template void _cuda_multiplyColumnsByScalars<double>(const double *vectorDevPtr, double *matrixDevPtr, unsigned int nRows, unsigned int nColumns);
template __global__ void __cuda_multiplyColumnsByScalars<float>(const float *vectorDevPtr, float *matrixDevPtr, unsigned int nRows, unsigned int nElements);
template void _cuda_multiplyColumnsByScalars<float>(const float *vectorDevPtr, float *matrixDevPtr, unsigned int nRows, unsigned int nColumns);

/*
 *
 * divideColumnsByScalars
 *
 */
template<typename T>
__global__ void __cuda_divideColumnsByScalars(const T *vectorDevPtr, T *matrixDevPtr, unsigned int nRows, unsigned int nElements){
    unsigned int index = threadIdx.x + blockIdx.x * blockDim.x;
    unsigned int colIndex = index / nRows;
    if (index < nElements)
	matrixDevPtr[index] = matrixDevPtr[index] / vectorDevPtr[colIndex];
}
template<typename T>
void _cuda_divideColumnsByScalars(const T *vectorDevPtr, T *matrixDevPtr, unsigned int nRows, unsigned int nColumns){
    // TODO parallelization without mod operator (mod is slow on GPU)
    unsigned int nElements = nRows * nColumns;
    int gridSize = (int)ceil( (float) nElements/THREADS_PER_BLOCK);

    __cuda_divideColumnsByScalars <<< gridSize , THREADS_PER_BLOCK >>> (vectorDevPtr, matrixDevPtr, nRows, nElements);
}

template __global__ void __cuda_divideColumnsByScalars<double>(const double *vectorDevPtr, double *matrixDevPtr, unsigned int nRows, unsigned int nElements);
template void _cuda_divideColumnsByScalars<double>(const double *vectorDevPtr, double *matrixDevPtr, unsigned int nRows, unsigned int nColumns);
template __global__ void __cuda_divideColumnsByScalars<float>(const float *vectorDevPtr, float *matrixDevPtr, unsigned int nRows, unsigned int nElements);
template void _cuda_divideColumnsByScalars<float>(const float *vectorDevPtr, float *matrixDevPtr, unsigned int nRows, unsigned int nColumns);

/*
 *
 * multiplyRowsByScalars
 *
 */
template<typename T>
__global__ void __cuda_multiplyRowsByScalars(const T *vectorDevPtr, T *matrixDevPtr, unsigned int nRows, unsigned int nElements){
    unsigned int index = threadIdx.x + blockIdx.x * blockDim.x;
    unsigned int rowIndex = index % nRows;
    if (index < nElements)
	matrixDevPtr[index] = matrixDevPtr[index] * vectorDevPtr[rowIndex];
}
template<typename T>
void _cuda_multiplyRowsByScalars(const T *vectorDevPtr, T *matrixDevPtr, unsigned int nRows, unsigned int nColumns){
    // TODO parallelization without mod operator (mod is slow on GPU)
    unsigned int nElements = nRows * nColumns;
    int gridSize = (int)ceil( (float) nElements/THREADS_PER_BLOCK);

    __cuda_multiplyRowsByScalars <<< gridSize , THREADS_PER_BLOCK >>> (vectorDevPtr, matrixDevPtr, nRows, nElements);
}


template __global__ void __cuda_multiplyRowsByScalars<double>(const double *vectorDevPtr, double *matrixDevPtr, unsigned int nRows,unsigned int nElements);
template void _cuda_multiplyRowsByScalars<double>(const double *vectorDevPtr, double *matrixDevPtr, unsigned int nRows, unsigned int nColumns);
template __global__ void __cuda_multiplyRowsByScalars<float>(const float *vectorDevPtr, float *matrixDevPtr, unsigned int nRows, unsigned int nElements);
template void _cuda_multiplyRowsByScalars<float>(const float *vectorDevPtr, float *matrixDevPtr, unsigned int nRows, unsigned int nColumns);

/*
 *
 * divideRowsByScalars
 *
 */
template<typename T>
__global__ void __cuda_divideRowsByScalars(const T *vectorDevPtr, T *matrixDevPtr, unsigned int nRows, unsigned int nElements){
    unsigned int index = threadIdx.x + blockIdx.x * blockDim.x;
    unsigned int rowIndex = index % nRows;
    if (index < nElements)
	matrixDevPtr[index] = matrixDevPtr[index] / vectorDevPtr[rowIndex];
}
template<typename T>
void _cuda_divideRowsByScalars(const T *vectorDevPtr, T *matrixDevPtr, unsigned int nRows, unsigned int nColumns){
    // TODO parallelization without mod operator (mod is slow on GPU)
    unsigned int nElements = nRows * nColumns;
    int gridSize = (int)ceil( (float) nElements/THREADS_PER_BLOCK);

    __cuda_divideRowsByScalars <<< gridSize , THREADS_PER_BLOCK >>> (vectorDevPtr, matrixDevPtr, nRows, nElements);
}


template __global__ void __cuda_divideRowsByScalars<double>(const double *vectorDevPtr, double *matrixDevPtr, unsigned int nRows,unsigned int nElements);
template void _cuda_divideRowsByScalars<double>(const double *vectorDevPtr, double *matrixDevPtr, unsigned int nRows, unsigned int nColumns);
template __global__ void __cuda_divideRowsByScalars<float>(const float *vectorDevPtr, float *matrixDevPtr, unsigned int nRows,unsigned int nElements);
template void _cuda_divideRowsByScalars<float>(const float *vectorDevPtr, float *matrixDevPtr, unsigned int nRows, unsigned int nColumns);

/*
 *
 *  fill
 *
 */
template<typename T>
__global__ void __cuda_fill(T *data, T value, unsigned int nElements){
    unsigned  int index = threadIdx.x + blockIdx.x * blockDim.x;
    if (index < nElements)
	data[index] = value;
}
template<typename T>
void _cuda_fill(T *data, T value, unsigned int nRows, unsigned int nColumns)
{

    unsigned int nElements = nRows * nColumns;
    int gridSize = (int)ceil( (float) nElements/THREADS_PER_BLOCK);

    __cuda_fill <<< gridSize , THREADS_PER_BLOCK >>> (data, value, nElements);
}

template __global__ void __cuda_fill<double>(double *data, double value, unsigned int nElements);
template void _cuda_fill<double>(double *data, double value, unsigned int nRows, unsigned int nColumns);
template __global__ void __cuda_fill<float>(float *data, float value, unsigned int nElements);
template void _cuda_fill<float>(float *data, float value, unsigned int nRows, unsigned int nColumns);
template __global__ void __cuda_fill<uint>(uint *data, uint value, unsigned int nElements);
template void _cuda_fill<uint>(uint *data, uint value, unsigned int nRows, unsigned int nColumns);

/*
 *
 *  ensure minimal value
 *
 */
template<typename T>
__global__ void __cuda_ensureMinimalValue(T *data, T value, unsigned int nElements){
    unsigned  int index = threadIdx.x + blockIdx.x * blockDim.x;
    if ((index < nElements) && (data[index] < value))
	data[index] = value;
}

template<typename T>
void _cuda_ensureMinimalValue(T *data, T value, unsigned int nRows, unsigned int nColumns)
{

    unsigned int nElements = nRows * nColumns;
    int gridSize = (int)ceil( (float) nElements/THREADS_PER_BLOCK);

    __cuda_ensureMinimalValue <<< gridSize , THREADS_PER_BLOCK >>> (data, value, nElements);
}

template __global__ void __cuda_ensureMinimalValue(double *data, double value, unsigned int nElements);
template void _cuda_ensureMinimalValue(double *data, double value, unsigned int nRows, unsigned int nColumns);
template __global__ void __cuda_ensureMinimalValue(float *data, float value, unsigned int nElements);
template void _cuda_ensureMinimalValue(float *data, float value, unsigned int nRows, unsigned int nColumns);


/*
 *
 * nClassificationErrors
 *
 *
 */
template<typename T>
__global__ void __cuda_nClassificationErrors(T *matrixPtr, unsigned int nRows, unsigned int nColumns, unsigned int *alignmentDevPtr, unsigned int *resultDevPtr){
    unsigned  int column= threadIdx.x + blockIdx.x * blockDim.x;
    if (column < nColumns){
	int beginCol = column * nRows;
	T maxVal = matrixPtr[beginCol];
	uint argmax = 0;
	for (int i = 1; i < nRows; i++){
	    T val = matrixPtr[beginCol + i];
	    if (val > maxVal){
		maxVal =  val;
		argmax = i;
	    }
	}
	if (argmax != alignmentDevPtr[column]){
	    atomicAdd(resultDevPtr, 1);
	}
    }
}
template<typename T>
void _cuda_nClassificationErrors(T *matrixPtr, unsigned int nRows, unsigned int nColumns, unsigned int *alignmentDevPtr, unsigned int *resultDevPtr)
{
    // parallelization over columns only
    int gridSize = (int)ceil( (float) nColumns/THREADS_PER_BLOCK);
    unsigned int result = 0;
    cudaMemcpy(resultDevPtr, &result, sizeof(unsigned int), cudaMemcpyHostToDevice);
    __cuda_nClassificationErrors <<< gridSize, THREADS_PER_BLOCK>>> (matrixPtr, nRows, nColumns, alignmentDevPtr, resultDevPtr);
}


template __global__ void __cuda_nClassificationErrors<double>(double *matrixPtr, unsigned int nRows, unsigned int nColumns, unsigned int *alignmentDevPtr, unsigned int *resultDevPtr);
template void _cuda_nClassificationErrors<double>(double *matrixPtr, unsigned int nRows, unsigned int nColumns, unsigned int *alignmentDevPtr, unsigned int *resultDevPtr);
template __global__ void __cuda_nClassificationErrors<float>(float *matrixPtr, unsigned int nRows, unsigned int nColumns, unsigned int *alignmentDevPtr, unsigned int *resultDevPtr);
template void _cuda_nClassificationErrors<float>(float *matrixPtr, unsigned int nRows, unsigned int nColumns, unsigned int *alignmentDevPtr, unsigned int *resultDevPtr);

// crossEntropyObjectiveFunction
template<typename T>
__global__ void __cuda_crossEntropyObjectiveFunction(T *matrixPtr, unsigned int nRows, unsigned int nColumns, unsigned int *alignmentDevPtr, T *objFctn){
    *objFctn = 0.0f;
    for (int column = 0; column < nColumns; column++){
	unsigned int position = column * nRows + alignmentDevPtr[column];
	*objFctn -= log(matrixPtr[position]);
    }
}

template<typename T>
void _cuda_crossEntropyObjectiveFunction(T *matrixPtr, unsigned int nRows, unsigned int nColumns, unsigned int *alignmentDevPtr, T *objFctn)
{
    // no parallelization, but probably not relevant
    __cuda_crossEntropyObjectiveFunction <<< 1,1>>> (matrixPtr, nRows, nColumns, alignmentDevPtr, objFctn);
}

template __global__ void __cuda_crossEntropyObjectiveFunction<double>(double *matrixPtr, unsigned int nRows, unsigned int nColumns, unsigned int *alignmentDevPtr, double *objFctn);
template void _cuda_crossEntropyObjectiveFunction<double>(double *matrixPtr, unsigned int nRows, unsigned int nColumns, unsigned int *alignmentDevPtr, double *objFctn);
template __global__ void __cuda_crossEntropyObjectiveFunction<float>(float *matrixPtr, unsigned int nRows, unsigned int nColumns, unsigned int *alignmentDevPtr, float *objFctn);
template void _cuda_crossEntropyObjectiveFunction<float>(float *matrixPtr, unsigned int nRows, unsigned int nColumns, unsigned int *alignmentDevPtr, float *objFctn);

// weightedCrossEntropyObjectiveFunction
template<typename T>
__global__ void __cuda_weightedCrossEntropyObjectiveFunction(T *matrixPtr, unsigned int nRows, unsigned int nColumns, unsigned int *alignmentDevPtr, T *objFctn, T *weights){
    *objFctn = 0.0f;
    for (int column = 0; column < nColumns; column++){
	unsigned int position = column * nRows + alignmentDevPtr[column];
	*objFctn -= log(matrixPtr[position]) * weights[column];
    }
}

template<typename T>
void _cuda_weightedCrossEntropyObjectiveFunction(T *matrixPtr, unsigned int nRows, unsigned int nColumns, unsigned int *alignmentDevPtr, T *objFctn, T *weights)
{
    // no parallelization, but probably not relevant
    __cuda_weightedCrossEntropyObjectiveFunction <<< 1,1>>> (matrixPtr, nRows, nColumns, alignmentDevPtr, objFctn, weights);
}

template __global__ void __cuda_weightedCrossEntropyObjectiveFunction<double>(double *matrixPtr, unsigned int nRows, unsigned int nColumns, unsigned int *alignmentDevPtr, double *objFctn, double *weights);
template void _cuda_weightedCrossEntropyObjectiveFunction<double>(double *matrixPtr, unsigned int nRows, unsigned int nColumns, unsigned int *alignmentDevPtr, double *objFctn, double *weights);
template __global__ void __cuda_weightedCrossEntropyObjectiveFunction<float>(float *matrixPtr, unsigned int nRows, unsigned int nColumns, unsigned int *alignmentDevPtr, float *objFctn, float *weights);
template void _cuda_weightedCrossEntropyObjectiveFunction<float>(float *matrixPtr, unsigned int nRows, unsigned int nColumns, unsigned int *alignmentDevPtr, float *objFctn, float *weights);



// squaredErrorObjectiveFunction

template<typename T>
__global__ void __cuda_squaredErrorObjectiveFunction(T *matrixPtr, unsigned int nRows, unsigned int nColumns, unsigned int *alignmentDevPtr, T *objFctn){
    unsigned int row = threadIdx.x + blockIdx.x * blockDim.x;
    if (row < nRows){
	objFctn[row] = 0.0f;
	for (int column = 0; column < nColumns; column++){
	    T kroneckerDelta = alignmentDevPtr[column] == row ? 1.0 : 0.0;
	    unsigned int position = column * nRows + row;
	    objFctn[row] += (matrixPtr[position] - kroneckerDelta) * (matrixPtr[position] - kroneckerDelta);
	}
    }
}

template<typename T>
void _cuda_squaredErrorObjectiveFunction(T *matrixPtr, unsigned int nRows, unsigned int nColumns, unsigned int *alignmentDevPtr, T *objFctn)
{
    unsigned int nElements = nRows;
    int gridSize = (int)ceil( (float) nElements/THREADS_PER_BLOCK);
    
    // no parallelization, but probably not relevant
    __cuda_squaredErrorObjectiveFunction <<< gridSize , THREADS_PER_BLOCK >>> (matrixPtr, nRows, nColumns, alignmentDevPtr, objFctn);
}




template __global__ void __cuda_squaredErrorObjectiveFunction(double *matrixPtr, unsigned int nRows, unsigned int nColumns, unsigned int *alignmentDevPtr, double *objFctn);
template void _cuda_squaredErrorObjectiveFunction(double *matrixPtr, unsigned int nRows, unsigned int nColumns, unsigned int *alignmentDevPtr, double *objFctn);
template __global__ void __cuda_squaredErrorObjectiveFunction(float *matrixPtr, unsigned int nRows, unsigned int nColumns, unsigned int *alignmentDevPtr, float *objFctn);
template void _cuda_squaredErrorObjectiveFunction(float *matrixPtr, unsigned int nRows, unsigned int nColumns, unsigned int *alignmentDevPtr, float *objFctn);

// weightedSquaredErrorObjectiveFunction

template<typename T>
__global__ void __cuda_weightedSquaredErrorObjectiveFunction(T *matrixPtr, unsigned int nRows, unsigned int nColumns, unsigned int *alignmentDevPtr, T *objFctn, T *weights){
    unsigned int row = threadIdx.x + blockIdx.x * blockDim.x;
    if (row < nRows){
	objFctn[row] = 0.0f;
	for (int column = 0; column < nColumns; column++){
	    T kroneckerDelta = alignmentDevPtr[column] == row ? 1.0 : 0.0;
	    unsigned int position = column * nRows + row;
	    objFctn[row] += (matrixPtr[position] - kroneckerDelta) * (matrixPtr[position] - kroneckerDelta) * weights[column];
	}
    }
}

template<typename T>
void _cuda_weightedSquaredErrorObjectiveFunction(T *matrixPtr, unsigned int nRows, unsigned int nColumns, unsigned int *alignmentDevPtr, T *objFctn, T *weights)
{
    unsigned int nElements = nRows;
    int gridSize = (int)ceil( (float) nElements/THREADS_PER_BLOCK);
    
    __cuda_weightedSquaredErrorObjectiveFunction <<< gridSize , THREADS_PER_BLOCK >>> (matrixPtr, nRows, nColumns, alignmentDevPtr, objFctn, weights);
}

template __global__ void __cuda_weightedSquaredErrorObjectiveFunction(double *matrixPtr, unsigned int nRows, unsigned int nColumns, unsigned int *alignmentDevPtr, double *objFctn, double *weights);
template void _cuda_weightedSquaredErrorObjectiveFunction(double *matrixPtr, unsigned int nRows, unsigned int nColumns, unsigned int *alignmentDevPtr, double *objFctn, double *weights);
template __global__ void __cuda_weightedSquaredErrorObjectiveFunction(float *matrixPtr, unsigned int nRows, unsigned int nColumns, unsigned int *alignmentDevPtr, float *objFctn, float *weights);
template void _cuda_weightedSquaredErrorObjectiveFunction(float *matrixPtr, unsigned int nRows, unsigned int nColumns, unsigned int *alignmentDevPtr, float *objFctn, float *weights);

// ###########################################################################
// binaryDivergenceObjectiveFunction
template <typename T>
__global__ void __cuda_binaryDivergenceObjectiveFunction(T *matrixPtr, unsigned int nRows, unsigned int nColumns, unsigned int *alignmentDevPtr, T *objFctn){
    unsigned  int column = threadIdx.x + blockIdx.x * blockDim.x;
    if (column < nColumns) {
	objFctn[column] = 0.0;
	for (int row = 0; row < nRows; row++){
	    unsigned int position = column * nRows + row;
	    if (alignmentDevPtr[column] == row)
		objFctn[column] -= log(matrixPtr[position]);
	    else
		objFctn[column] -= log(1.0 - matrixPtr[position]);
	}
    }
}
template <typename T>
void _cuda_binaryDivergenceObjectiveFunction(T *matrixPtr, unsigned int nRows, unsigned int nColumns, unsigned int *alignmentDevPtr, T *objFctn) {
    int gridSize = (int)ceil( (float) nColumns/THREADS_PER_BLOCK);
    __cuda_binaryDivergenceObjectiveFunction<T> <<<gridSize, THREADS_PER_BLOCK>>> (matrixPtr, nRows, nColumns, alignmentDevPtr, objFctn);
}
template __global__ void __cuda_binaryDivergenceObjectiveFunction<float>(float*, unsigned int, unsigned int, unsigned int*, float*);
template __global__ void __cuda_binaryDivergenceObjectiveFunction<double>(double*, unsigned int, unsigned int, unsigned int*, double*);
template void _cuda_binaryDivergenceObjectiveFunction<float>(float*, unsigned int, unsigned int, unsigned int*, float*);
template void _cuda_binaryDivergenceObjectiveFunction<double>(double*, unsigned int, unsigned int, unsigned int*, double*);

// ###########################################################################
// weightedBinaryDivergenceObjectiveFunction
template <typename T>
__global__ void __cuda_weightedBinaryDivergenceObjectiveFunction(T *matrixPtr, unsigned int nRows, unsigned int nColumns, unsigned int *alignmentDevPtr, T *objFctn, T *weights){
    unsigned  int column = threadIdx.x + blockIdx.x * blockDim.x;
    if (column < nColumns){
	objFctn[column] = 0.0;
	for (int row = 0; row < nRows; row++){
	    unsigned int position = column * nRows + row;
	    if (alignmentDevPtr[column] == row)
		objFctn[column] -= log(matrixPtr[position]) * weights[column];
	    else
		objFctn[column] -= log(1.0 - matrixPtr[position]) * weights[column];
	}
    }
}
template <typename T>
void _cuda_weightedBinaryDivergenceObjectiveFunction(T *matrixPtr, unsigned int nRows, unsigned int nColumns, unsigned int *alignmentDevPtr, T *objFctn, T *weights) {
    int gridSize = (int)ceil( (float) nColumns/THREADS_PER_BLOCK);
    __cuda_weightedBinaryDivergenceObjectiveFunction<T> <<<gridSize, THREADS_PER_BLOCK>>> (matrixPtr, nRows, nColumns, alignmentDevPtr, objFctn, weights);
}
template __global__ void __cuda_weightedBinaryDivergenceObjectiveFunction<float>(float*, unsigned int, unsigned int, unsigned int*, float*, float*);
template __global__ void __cuda_weightedBinaryDivergenceObjectiveFunction<double>(double*, unsigned int, unsigned int, unsigned int*, double*, double*);
template void _cuda_weightedBinaryDivergenceObjectiveFunction<float>(float*, unsigned int, unsigned int, unsigned int*, float*, float*);
template void _cuda_weightedBinaryDivergenceObjectiveFunction<double>(double*, unsigned int, unsigned int, unsigned int*, double*, double*);


// ###########################################################################
// binary divergence softmax gradient computation

template <typename T>
__global__ void __cuda_binaryDivergenceSoftmaxGradient(T *gradient, unsigned int nRows, unsigned int nColumns, const T *output, const unsigned int *alignment) {
    unsigned  int column = threadIdx.x + blockIdx.x * blockDim.x;
    if (column < nColumns){
	T constsum = 0.0;
	for (int i = 0; i < nRows; ++ i) {
	    unsigned int position = column * nRows + i;
	    const T y = output[position];
	    if (alignment[column] == i)
		constsum -= 1.0;
	    else
		if (y<1.0) constsum += y / (1.0-y);
	}

	for (int i = 0; i < nRows; ++ i) {
	    unsigned int position = column * nRows + i;
	    const T y = output[position];
	    if (alignment[column] == i)
		gradient[position] = -1.0 - y*constsum;
	    else {
		if (y<1.0)
		    gradient[position] = y * (1.0 / (1.0 - y) - constsum);
		else
		    gradient[position] = 0.0;
	    }
	}
    }
}
template <typename T>
void _cuda_binaryDivergenceSoftmaxGradient(T *matrixPtr, unsigned int nRows, unsigned int nColumns, const T *outputDevPtr, const unsigned int *alignmentDevPtr) {
    int gridSize = (int)ceil((float) nColumns/THREADS_PER_BLOCK);
    __cuda_binaryDivergenceSoftmaxGradient<T> <<<gridSize, THREADS_PER_BLOCK>>> (matrixPtr, nRows, nColumns, outputDevPtr, alignmentDevPtr);
}
template __global__ void __cuda_binaryDivergenceSoftmaxGradient<float>(float*, unsigned int, unsigned int, const float*, const unsigned int*);
template __global__ void __cuda_binaryDivergenceSoftmaxGradient<double>(double*, unsigned int, unsigned int, const double*, const unsigned int*);
template void _cuda_binaryDivergenceSoftmaxGradient<float>(float *, unsigned int, unsigned int, const float *, const unsigned int *);
template void _cuda_binaryDivergenceSoftmaxGradient<double>(double *, unsigned int, unsigned int, const double *, const unsigned int *);


template<typename T>
__global__ void __cuda_addKroneckerDelta(T *matrixPtr, unsigned int nRows, unsigned int nColumns, const unsigned int *alignmentDevPtr, const T scale){
    unsigned  int index = threadIdx.x + blockIdx.x * blockDim.x;
    unsigned int nElements = nRows * nColumns;
    if (index < nElements){
	unsigned int colIndex = index / nRows;
	unsigned int rowIndex = index % nRows;
    	matrixPtr[index] += rowIndex == alignmentDevPtr[colIndex] ? scale : 0.0;
    }
}

template<typename T>
void _cuda_addKroneckerDelta(T *matrixPtr, unsigned int nRows, unsigned int nColumns, const unsigned int *alignmentDevPtr, const T scale)
{
    unsigned int nElements = nRows * nColumns;
    int gridSize = (int)ceil( (float) nElements/THREADS_PER_BLOCK);

    __cuda_addKroneckerDelta <<< gridSize , THREADS_PER_BLOCK >>> (matrixPtr, nRows, nColumns, alignmentDevPtr, scale);

}

template __global__ void __cuda_addKroneckerDelta<double>(double *matrixPtr, unsigned int nRows, unsigned int nColumns, const unsigned int *alignmentDevPtr, const double scale);
template void _cuda_addKroneckerDelta<double>(double *matrixPtr, unsigned int nRows, unsigned int nColumns, const unsigned int *alignmentDevPtr, const double scale);
template __global__ void __cuda_addKroneckerDelta<float>(float *matrixPtr, unsigned int nRows, unsigned int nColumns, const unsigned int *alignmentDevPtr, const float scale);
template void _cuda_addKroneckerDelta<float>(float *matrixPtr, unsigned int nRows, unsigned int nColumns, const unsigned int *alignmentDevPtr, const float scale);

/*
 *  appendSecondOrderFeatures
 */

template<typename T>
__global__ void __cuda_appendSecondOrderFeatures(const T *X, unsigned int nRowsX, unsigned int nColumnsX, T *Y, unsigned int nRowsY, unsigned int offset){
    unsigned  int column = threadIdx.x + blockIdx.x * blockDim.x;
    if (column < nColumnsX){
	unsigned int pos = offset;
	for (unsigned int i = 0; i < nRowsX; ++ i) {
	    for (unsigned int j = i; j < nRowsX; ++ j) {
		Y[column * nRowsY + pos] = X[column * nRowsX + i] * X[column * nRowsX + j];
		pos++;
	    }
	}
    }
}

template<typename T>
void _cuda_appendSecondOrderFeatures(const T *X, unsigned int nRowsX, unsigned int nColumnsX, T *Y, unsigned int nRowsY, unsigned int offset){
    int gridSize = (int)ceil( (float) nColumnsX/THREADS_PER_BLOCK);
    __cuda_appendSecondOrderFeatures <<< gridSize , THREADS_PER_BLOCK >>> (X, nRowsX, nColumnsX, Y, nRowsY, offset);
}

template __global__ void __cuda_appendSecondOrderFeatures(const double *X, unsigned int nRowsX, unsigned int nColumnsX, double *Y, unsigned int nRowsY, unsigned int offset);
template void _cuda_appendSecondOrderFeatures(const double *X, unsigned int nRowsX, unsigned int nColumnsX, double *Y, unsigned int nRowsY, unsigned int offset);
template __global__ void __cuda_appendSecondOrderFeatures(const float *X, unsigned int nRowsX, unsigned int nColumnsX, float *Y, unsigned int nRowsY, unsigned int offset);
template void _cuda_appendSecondOrderFeatures(const float *X, unsigned int nRowsX, unsigned int nColumnsX, float *Y, unsigned int nRowsY, unsigned int offset);


// appendThirdOrderFeatures

template<typename T>
__global__ void __cuda_appendThirdOrderFeatures(const T *X, unsigned int nRowsX, unsigned int nColumnsX, T *Y, unsigned int nRowsY, unsigned int offset){
    unsigned  int column = threadIdx.x + blockIdx.x * blockDim.x;
    if (column < nColumnsX){
	unsigned int pos = offset;
	for (unsigned int i = 0; i < nRowsX; ++ i) {
	    for (unsigned int j = i; j < nRowsX; ++ j) {
		for (unsigned int k = j; k < nRowsX; ++ k) {
		    Y[column * nRowsY + pos]  = X[column * nRowsX + i] * X[column * nRowsX + j] * X[column * nRowsX + k];
		    pos++;
		}
	    }
	}
    }
}

template<typename T>
void _cuda_appendThirdOrderFeatures(const T *X, unsigned int nRowsX, unsigned int nColumnsX, T *Y, unsigned int nRowsY, unsigned int offset){
    int gridSize = (int)ceil( (float) nColumnsX/THREADS_PER_BLOCK);
    __cuda_appendThirdOrderFeatures <<< gridSize , THREADS_PER_BLOCK >>> (X, nRowsX, nColumnsX, Y, nRowsY, offset);
}

template __global__ void __cuda_appendThirdOrderFeatures(const double *X, unsigned int nRowsX, unsigned int nColumnsX, double *Y, unsigned int nRowsY, unsigned int offset);
template void _cuda_appendThirdOrderFeatures(const double *X, unsigned int nRowsX, unsigned int nColumnsX, double *Y, unsigned int nRowsY, unsigned int offset);
template __global__ void __cuda_appendThirdOrderFeatures(const float *X, unsigned int nRowsX, unsigned int nColumnsX, float *Y, unsigned int nRowsY, unsigned int offset);
template void _cuda_appendThirdOrderFeatures(const float *X, unsigned int nRowsX, unsigned int nColumnsX, float *Y, unsigned int nRowsY, unsigned int offset);
/*
 *
 * dropout
 *
 */
template<typename T>
__global__ void __cuda_dropout(T *data, const T *mask, unsigned int nElements, T dropoutProbability){
    unsigned  int index = threadIdx.x + blockIdx.x * blockDim.x;
    if ((index < nElements) && (mask[index] <= dropoutProbability))
	    data[index] = 0.0;
}

template<typename T>
void _cuda_dropout(T *data, const T *mask, unsigned int nRows, unsigned int nColumns, T dropoutProbability)
{

    unsigned int nElements = nRows * nColumns;
    int gridSize = (int)ceil( (float) nElements/THREADS_PER_BLOCK);

    __cuda_dropout <<< gridSize , THREADS_PER_BLOCK >>> (data, mask, nElements, dropoutProbability);
}

template __global__ void __cuda_dropout(double *data, const double *mask, unsigned int nElements, double dropoutProbability);
template void _cuda_dropout(double *data, const double *mask, unsigned int nRows, unsigned int nColumns, double dropoutProbability);
template __global__ void __cuda_dropout(float *data, const float *mask, unsigned int nElements, float dropoutProbability);
template void _cuda_dropout(float *data, const float *mask, unsigned int nRows, unsigned int nColumns, float dropoutProbability);

/*
 *
 * l1clipping
 *
 */
template<typename T>
__global__ void __cuda_l1clipping(T *data, unsigned int nElements, T value){

    unsigned  int index = threadIdx.x + blockIdx.x * blockDim.x;
    if (index < nElements) {
	    if (data[index] > 0) {
	        if (data[index] - value > 0)
	            data[index] = data[index] - value;
	        else
	            data[index] = 0;
	    }
	    else if (data[index] < 0) {
	        if (data[index] + value < 0)
	            data[index] = data[index] + value;
	        else
	           data[index] = 0;
	    }
    }
}

template<typename T>
void _cuda_l1clipping(T *data, unsigned int nRows, unsigned int nColumns, T value)
{

    unsigned int nElements = nRows * nColumns;
    int gridSize = (int)ceil( (float) nElements/THREADS_PER_BLOCK);

    __cuda_l1clipping <<< gridSize , THREADS_PER_BLOCK >>> (data, nElements, value);
}

template __global__ void __cuda_l1clipping(double *data, unsigned int nElements, double value);
template void _cuda_l1clipping(double *data, unsigned int nRows, unsigned int nColumns, double value);
template __global__ void __cuda_l1clipping(float *data, unsigned int nElements, float value);
template void _cuda_l1clipping(float *data, unsigned int nRows, unsigned int nColumns, float value);
