#include <stdio.h>
#include <cuda_runtime.h>
#include <limits.h>
#include <float.h>
#include <math.h>
#include <sys/time.h>
#include <pthread.h>
#include <string>

#include "../../utils/timer.h"

float feature_vect[] = {2.240018,    2.2570236,    0.11304555,   -0.21307051,
                        0.8988138,   0.039065503,  0.023874786,  0.13153112,
                        0.15324382,  0.16986738,   -0.020297153, -0.26773554,
                        0.40202165,  0.35923952,   0.060746543,  0.35402644,
                        0.086052455, -0.10499257,  0.04395058,   0.026407119,
                        -0.48301497, 0.120889395,  0.67980754,   -0.19875681,
                        -0.5443737,  -0.039534688, 0.20888293,   0.054865785,
                        -0.4846478,  0.1,          0.1,          0.1};

float *means_vect;
float *precs_vect;
float *weight_vect;
float *factor_vect;
float *score_vect;

__device__ __constant__ float logZero = -3.4028235E38;
__device__ __constant__ float maxLogValue = 7097004.5;
__device__ __constant__ float minLogValue = -7443538.0;
__device__ __constant__ float naturalLogBase = (float)1.00011595E-4;
__device__ __constant__ float inverseNaturalLogBase = 9998.841;
// fixed for a given accoustic model
__device__ __constant__ int comp_size = 32;
__device__ __constant__ int feat_size = 29;
__device__ __constant__ int senone_size = 5120;

extern "C"

    __global__ void
    computeScore(const float *feature_vect, float *means_vect,
                 float *precs_vect, float *weight_vect, float *factor_vect,
                 float *score_vect) {
  int i = blockIdx.x * blockDim.x + threadIdx.x;

  if (i < senone_size) {
    float local_score_vect = logZero;

#pragma unroll 32
    for (int j = 0; j < comp_size; j++) {
      // getScore
      float logDval = 0.0f;
#pragma unroll 29
      for (int k = 0; k < feat_size; k++) {
        int idx = i + senone_size * j + k * comp_size * senone_size;
        float logDiff = feature_vect[k] - means_vect[idx];
        logDval += logDiff * logDiff * precs_vect[idx];
      }

      // Convert to the appropriate base.
      if (logDval != logZero) {
        logDval = logDval * inverseNaturalLogBase;
      }

      int idx2 = i + j * senone_size;

      // Add the precomputed factor, with the appropriate sign.
      logDval -= factor_vect[idx2];

      if (logDval < logZero) {
        logDval = logZero;
      }
      // end of getScore

      float logVal2 = logDval + weight_vect[idx2];

      float logHighestValue = local_score_vect;
      float logDifference = local_score_vect - logVal2;

      // difference is always a positive number
      if (logDifference < 0) {
        logHighestValue = logVal2;
        logDifference = -logDifference;
      }

      float logValue = -logDifference;
      float logInnerSummation;
      if (logValue < minLogValue) {
        logInnerSummation = 0.0;
      } else if (logValue > maxLogValue) {
        logInnerSummation = FLT_MAX;
      } else {
        if (logValue == logZero) {
          logValue = logZero;
        } else {
          logValue = logValue * naturalLogBase;
        }
        logInnerSummation = __expf(logValue);
      }

      logInnerSummation += 1.0;

      float returnLogValue;
      if (logInnerSummation <= 0.0) {
        returnLogValue = logZero;
      } else {
        returnLogValue = __logf(logInnerSummation) * inverseNaturalLogBase;
        if (returnLogValue > FLT_MAX) {
          returnLogValue = FLT_MAX;
        } else if (returnLogValue < -FLT_MAX) {
          returnLogValue = -FLT_MAX;
        }
      }
      // sum log
      local_score_vect = logHighestValue + returnLogValue;
    }
    score_vect[i] = local_score_vect;
  }
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "[ERROR] Invalid arguments provided.\n\n");
    fprintf(stderr, "Usage: %s [INPUT FILE]\n\n", argv[0]);
    exit(0);
  }
  STATS_INIT("kernel", "gpu_gaussian_mixture_model");
  PRINT_STAT_STRING("abrv", "gpu_gmm");

  float *dev_feat_vect;

  float cuda_elapsedTime;
  cudaEvent_t eStart, eStop;
  int comp_size = 32;
  int senone_size = 5120;

  int means_array_size = senone_size * comp_size * comp_size;
  int comp_array_size = senone_size * comp_size;

  means_vect = (float *)malloc(means_array_size * sizeof(float));
  precs_vect = (float *)malloc(means_array_size * sizeof(float));
  weight_vect = (float *)malloc(comp_array_size * sizeof(float));
  factor_vect = (float *)malloc(comp_array_size * sizeof(float));

  float *means_vect2 = (float *)malloc(means_array_size * sizeof(float));
  float *precs_vect2 = (float *)malloc(means_array_size * sizeof(float));
  float *weight_vect2 = (float *)malloc(comp_array_size * sizeof(float));
  float *factor_vect2 = (float *)malloc(comp_array_size * sizeof(float));

  float *dev_means_vect;
  float *dev_precs_vect;
  float *dev_weight_vect;
  float *dev_factor_vect;

  score_vect = (float *)malloc(senone_size * sizeof(float));

  float *dev_score_vect;

  int blockSizeX = 256;
  int gridSizeX = (int)ceil(senone_size / blockSizeX);

  int div_grid = ((int)(gridSizeX / 32));
  gridSizeX = (div_grid + 1) * 32;

  // load model from file
  FILE *fp = fopen(argv[1], "r");
  if (fp == NULL) {  // checks for the file
    printf("\n Can’t open file");
    exit(-1);
  }

  int idx = 0;
  for (int i = 0; i < senone_size; i++) {
    for (int j = 0; j < comp_size; j++) {
      for (int k = 0; k < comp_size; k++) {
        float elem;
        fscanf(fp, "%f", &elem);
        means_vect[idx] = elem;
        idx = idx + 1;
      }
    }
  }

  idx = 0;
  for (int i = 0; i < senone_size; i++) {
    for (int j = 0; j < comp_size; j++) {
      for (int k = 0; k < comp_size; k++) {
        float elem;
        fscanf(fp, "%f", &elem);
        precs_vect[idx] = elem;
        idx = idx + 1;
      }
    }
  }

  idx = 0;
  for (int i = 0; i < senone_size; i++) {
    for (int j = 0; j < comp_size; j++) {
      float elem;
      fscanf(fp, "%f", &elem);
      weight_vect[idx] = elem;
      idx = idx + 1;
    }
  }

  idx = 0;
  for (int i = 0; i < senone_size; i++) {
    for (int j = 0; j < comp_size; j++) {
      float elem;
      fscanf(fp, "%f", &elem);
      factor_vect[idx] = elem;
      idx = idx + 1;
    }
  }

  fclose(fp);

  int idx3 = 0;
  for (int j = 0; j < comp_size; j++) {
    for (int i = 0; i < senone_size; i++) {
      int ij = j + i * comp_size;
      weight_vect2[idx3] = weight_vect[ij];
      factor_vect2[idx3] = factor_vect[ij];
      idx3 += 1;
    }
  }

  int idx4 = 0;
  for (int k = 0; k < comp_size; k++) {
    for (int j = 0; j < comp_size; j++) {
      for (int i = 0; i < senone_size; i++) {
        int ijk = k + comp_size * j + i * comp_size * comp_size;
        means_vect2[idx4] = means_vect[ijk];
        precs_vect2[idx4] = precs_vect[ijk];
        idx4 += 1;
      }
    }
  }

  for (int i = 0; i < senone_size; i++) {
    for (int j = 0; j < comp_size; j++) {
      for (int k = 0; k < 29; k++) {
        int ijk = k + comp_size * j + i * comp_size * comp_size;
        int kji = i + senone_size * j + k * comp_size * senone_size;
        if (means_vect2[kji] != means_vect[ijk]) {
          printf("%f != %f\n", means_vect2[kji], means_vect[ijk]);
        }
      }
    }
  }

  cudaEventCreate(&eStart);
  cudaEventCreate(&eStop);

  // just one time to load acoustic model
  cudaMalloc((void **)&dev_means_vect, sizeof(float) * means_array_size);
  cudaMalloc((void **)&dev_precs_vect, sizeof(float) * means_array_size);
  cudaMalloc((void **)&dev_weight_vect, sizeof(float) * comp_array_size);
  cudaMalloc((void **)&dev_factor_vect, sizeof(float) * comp_array_size);

  cudaMemcpy(dev_means_vect, means_vect2, sizeof(float) * means_array_size,
             cudaMemcpyHostToDevice);
  cudaMemcpy(dev_precs_vect, precs_vect2, sizeof(float) * means_array_size,
             cudaMemcpyHostToDevice);
  cudaMemcpy(dev_weight_vect, weight_vect2, sizeof(float) * comp_array_size,
             cudaMemcpyHostToDevice);
  cudaMemcpy(dev_factor_vect, factor_vect2, sizeof(float) * comp_array_size,
             cudaMemcpyHostToDevice);

  cudaMalloc((void **)&dev_feat_vect, sizeof(float) * comp_size);
  cudaMalloc((void **)&dev_score_vect, sizeof(float) * senone_size);

  PRINT_STAT_INT("blockSizeX", blockSizeX);
  PRINT_STAT_INT("gridSizeX", gridSizeX);

  dim3 block(128);
  dim3 grid;
  grid.x = (senone_size + block.x - 1) / block.x;

  if (grid.x < 32) grid.x = 32;

  cudaEventRecord(eStart, 0);

  // each time needed for computing score of a given feature vect
  cudaEventRecord(eStart, 0);
  cudaMemcpy(dev_feat_vect, feature_vect, comp_size * sizeof(float),
             cudaMemcpyHostToDevice);
  cudaEventRecord(eStop, 0);
  cudaEventSynchronize(eStop);
  cudaEventElapsedTime(&cuda_elapsedTime, eStart, eStop);
  PRINT_STAT_DOUBLE("host_to_device", cuda_elapsedTime);

  cudaEventRecord(eStart, 0);
  computeScore << <grid, block>>> (dev_feat_vect, dev_means_vect,
                                   dev_precs_vect, dev_weight_vect,
                                   dev_factor_vect, dev_score_vect);

  cudaEventRecord(eStop, 0);
  cudaEventSynchronize(eStop);

  cudaEventElapsedTime(&cuda_elapsedTime, eStart, eStop);
  PRINT_STAT_DOUBLE("gpu_gmm", cuda_elapsedTime);

  cudaEventRecord(eStart, 0);
  cudaMemcpy(score_vect, dev_score_vect, senone_size * sizeof(float),
             cudaMemcpyDeviceToHost);
  cudaEventRecord(eStop, 0);
  cudaEventSynchronize(eStop);
  cudaEventElapsedTime(&cuda_elapsedTime, eStart, eStop);
  PRINT_STAT_DOUBLE("device_to_host", cuda_elapsedTime);

  STATS_END();

#if TESTING
  FILE *f = fopen("../input/gmm_scoring.gpu", "w");

  for (int i = 0; i < senone_size; ++i) fprintf(f, "%.0f\n", score_vect[i]);

  fclose(f);
#endif

  cudaEventRecord(eStop, 0);
  cudaEventSynchronize(eStop);

  cudaEventElapsedTime(&cuda_elapsedTime, eStart, eStop);

  free(means_vect);
  free(precs_vect);

  free(weight_vect);
  free(factor_vect);

  free(score_vect);

  cudaFree(dev_means_vect);
  cudaFree(dev_precs_vect);
  cudaFree(dev_weight_vect);
  cudaFree(dev_factor_vect);

  cudaFree(dev_feat_vect);
  cudaFree(dev_score_vect);
}
