#include "SENNA_nn.h"
#include "SENNA_utils.h"

#include <string.h>
#include <float.h>

#ifdef USE_ATLAS_BLAS
#define USE_BLAS
#include "cblas.h"
#endif

#ifdef USE_MKL_BLAS
#define USE_BLAS
#include "mkl_cblas.h"
#endif

void SENNA_nn_lookup(float *dest, int dest_stride, const float *wordweights,
                     int wordsize, int maxwordidx, const int *wordindices,
                     int nword, int padidx, int npad) {
  int i;

  if (padidx < 0 || padidx >= maxwordidx)
    SENNA_error("lookup: padding index out of range");

  for (i = 0; i < npad; i++)
    memcpy(dest + i * dest_stride, wordweights + padidx * wordsize,
           wordsize * sizeof(float));

  for (i = 0; i < nword; i++) {
    int wordidx = wordindices[i];
    if (wordidx < 0 || wordidx >= maxwordidx)
      SENNA_error("lookup: index out of range");

    memcpy(dest + (i + npad) * dest_stride, wordweights + wordidx * wordsize,
           wordsize * sizeof(float));
  }

  for (i = 0; i < npad; i++)
    memcpy(dest + (i + npad + nword) * dest_stride,
           wordweights + padidx * wordsize, wordsize * sizeof(float));
}

void SENNA_nn_hardtanh(float *output, float *input, int size) {
  int i;

  for (i = 0; i < size; i++) {
    float z = input[i];
    if (z >= -1 && z <= 1)
      output[i] = z;
    else if (z < -1)
      output[i] = -1;
    else
      output[i] = 1;
  }
}

void SENNA_nn_linear(float *output, int output_size, float *weights,
                     float *biases, float *input, int input_size) {
#ifdef USE_BLAS
  if (biases) cblas_scopy(output_size, biases, 1, output, 1);
  cblas_sgemv(CblasColMajor, CblasTrans, input_size, output_size, 1.0, weights,
              input_size, input, 1, (biases ? 1.0 : 0.0), output, 1);
#else
  int i, j;

  for (i = 0; i < output_size; i++) {
    float z = (biases ? biases[i] : 0);
    float *weights_row = weights + i * input_size;
    for (j = 0; j < input_size; j++) z += input[j] * weights_row[j];
    output[i] = z;
  }
#endif
}

void SENNA_nn_max(float *value_, int *idx_, float *input, int input_size) {
  float value = -FLT_MAX;
  int idx = -1;
  int i;

  for (i = 0; i < input_size; i++) {
    if (input[i] > value) {
      value = input[i];
      idx = i;
    }
  }

  if (value_) *value_ = value;

  if (idx_) *idx_ = idx;
}

void SENNA_nn_temporal_convolution(float *output, int output_frame_size,
                                   float *weights, float *biases, float *input,
                                   int input_frame_size, int n_frames,
                                   int k_w) {
#ifdef USE_BLAS
  if (k_w == 1) {
    if (biases) {
      int t;
      for (t = 0; t < n_frames; t++)
        cblas_scopy(output_frame_size, biases, 1,
                    output + t * output_frame_size, 1);
    }
    cblas_sgemm(CblasColMajor, CblasTrans, CblasNoTrans, output_frame_size,
                n_frames, input_frame_size, 1.0, weights, input_frame_size,
                input, input_frame_size, (biases ? 1.0 : 0.0), output,
                output_frame_size);
  } else
#endif
  {
    int t;

    for (t = 0; t < n_frames - k_w + 1; t++)
      SENNA_nn_linear(output + t * output_frame_size, output_frame_size,
                      weights, biases, input + t * input_frame_size,
                      input_frame_size * k_w);
  }
}

void SENNA_nn_temporal_max_convolution(float *output, float *bias, float *input,
                                       int input_frame_size, int n_frames,
                                       int k_w) {
  int i, j, k;
  int h_k_w = (k_w - 1) / 2;

  for (k = 0; k < n_frames; k++) {
    for (i = 0; i < input_frame_size; i++) {
      float maxval = -FLT_MAX;
      for (j = -k; j < n_frames - k; j++) {
        int jbias = j + h_k_w;
        int jinput = k + j;
        float z;

        if (jbias < 0) jbias = 0;
        if (jbias >= k_w) jbias = k_w - 1;

        z = input[i + jinput * input_frame_size] +
            bias[i + jbias * input_frame_size];
        if (z > maxval) maxval = z;
      }
      output[i + k * input_frame_size] = maxval;
    }
  }
}

void SENNA_nn_temporal_max(float *output, float *input, int N, int T) {
  int n, t;

  for (n = 0; n < N; n++) {
    float z = -FLT_MAX;
    for (t = 0; t < T; t++) {
      if (input[t * N + n] > z) z = input[t * N + n];
    }
    output[n] = z;
  }
}

#define NN_MIN(a, b) ((a) < (b) ? (a) : (b))
#define NN_MAX(a, b) ((a) > (b) ? (a) : (b))

void SENNA_nn_distance(int *dest, int idx, int max_idx, int sentence_size,
                       int padding_size) {
  int i;

  max_idx = (max_idx - 1) / 2;

  for (i = 0; i < padding_size; i++)
    dest[i] =
        NN_MAX(NN_MIN(i - padding_size - idx, max_idx), -max_idx) + max_idx;

  for (i = 0; i < sentence_size; i++)
    dest[i + padding_size] =
        NN_MAX(NN_MIN(i - idx, max_idx), -max_idx) + max_idx;

  for (i = 0; i < padding_size; i++)
    dest[i + padding_size + sentence_size] =
        NN_MAX(NN_MIN(i + sentence_size - idx, max_idx), -max_idx) + max_idx;
}

void SENNA_nn_viterbi(int *path, float *init, float *transition,
                      float *emission, int N, int T) {
  float *delta, *deltap;
  int *phi;
  int i, j, t;

  /* misc allocations */
  delta = SENNA_malloc(sizeof(float), N);
  deltap = SENNA_malloc(sizeof(float), N);
  phi = SENNA_malloc(sizeof(float), N * T);

  /* init */
  for (i = 0; i < N; i++) deltap[i] = init[i] + emission[i];

  /* recursion */
  for (t = 1; t < T; t++) {
    float *deltan = delta;
    for (j = 0; j < N; j++) {
      float maxValue = -FLT_MAX;
      int maxIndex = 0;
      for (i = 0; i < N; i++) {
        float z = deltap[i] + transition[i + j * N];
        if (z > maxValue) {
          maxValue = z;
          maxIndex = i;
        }
      }
      delta[j] = maxValue + emission[j + t * N];
      phi[j + t * N] = maxIndex;
    }
    delta = deltap;
    deltap = deltan;
  }

  {
    float maxValue = -FLT_MAX;
    int maxIndex = 0;
    for (j = 0; j < N; j++) {
      if (deltap[j] > maxValue) {
        maxValue = deltap[j];
        maxIndex = j;
      }
    }
    path[T - 1] = maxIndex;
  }

  for (t = T - 2; t >= 0; t--) path[t] = phi[path[t + 1] + (t + 1) * N];

  SENNA_free(delta);
  SENNA_free(deltap);
  SENNA_free(phi);
}
