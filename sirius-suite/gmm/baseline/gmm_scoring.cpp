#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <math.h>
#include <string>

#include "../../utils/timer.h"
#include "../../utils/memoryman.h"

using namespace std;

float *means_vect;
float *precs_vect;
float *weight_vect;
float *factor_vect;

float logZero = -3.4028235E38;
float logBase = 1.0001;
float maxLogValue = 7097004.5;
float minLogValue = -7443538.0;
float naturalLogBase = (float)1.00011595E-4;
float inverseNaturalLogBase = 9998.841;

int comp_size = 32;
int feat_size = 29;
int senone_size = 5120;

void computeScore_seq(float *feature_vect, float *means_vect, float *precs_vect,
                      float *weight_vect, float *factor_vect, float *score_vect) {

  for (int i = 0; i < senone_size; i++) {
    for (int j = 0; j < comp_size; j++) {
      float logDval = 0.0f;

      for (int k = 0; k < feat_size; k++) {
        int idx = k + comp_size * j + i * comp_size * comp_size;
        float logDiff = feature_vect[k] - means_vect[idx];
        logDval += logDiff * logDiff * precs_vect[idx];
      }

      // Convert to the appropriate base.
      if (logDval != logZero) {
        logDval = logDval * inverseNaturalLogBase;
      }

      int idx2 = j + i * comp_size;
      logDval -= factor_vect[idx2];

      if (logDval < logZero) {
        logDval = logZero;
      }

      float logVal2 = logDval + weight_vect[idx2];

      float logHighestValue = score_vect[i];
      float logDifference = score_vect[i] - logVal2;

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
        logInnerSummation = exp(logValue);
      }

      logInnerSummation += 1.0;

      float returnLogValue;
      if (logInnerSummation <= 0.0) {
        returnLogValue = logZero;

      } else {
        returnLogValue =
            (float)(log(logInnerSummation) * inverseNaturalLogBase);
        if (returnLogValue > FLT_MAX) {
          returnLogValue = FLT_MAX;
        } else if (returnLogValue < -FLT_MAX) {
          returnLogValue = -FLT_MAX;
        }
      }
      // sum log
      score_vect[i] = logHighestValue + returnLogValue;
    }
  }
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    fprintf(stderr, "[ERROR] Input file required.\n\n");
    fprintf(stderr, "Usage: %s [NUM SCORES] [INPUT FILE]\n\n", argv[0]);
    exit(0);
  }

  float feature_vect[] = {2.240018,    2.2570236,    0.11304555,   -0.21307051,
    0.8988138,   0.039065503,  0.023874786,  0.13153112,
    0.15324382,  0.16986738,   -0.020297153, -0.26773554,
    0.40202165,  0.35923952,   0.060746543,  0.35402644,
    0.086052455, -0.10499257,  0.04395058,   0.026407119,
    -0.48301497, 0.120889395,  0.67980754,   -0.19875681,
    -0.5443737,  -0.039534688, 0.20888293,   0.054865785,
    -0.4846478};

  STATS_INIT("kernel", "gaussian_mixture_model");
  PRINT_STAT_STRING("abrv", "gmm");

  int means_array_size = senone_size * comp_size * comp_size;
  int comp_array_size = senone_size * comp_size;

  means_vect = (float *)sirius_malloc(means_array_size * sizeof(float));
  precs_vect = (float *)sirius_malloc(means_array_size * sizeof(float));
  weight_vect = (float *)sirius_malloc(comp_array_size * sizeof(float));
  factor_vect = (float *)sirius_malloc(comp_array_size * sizeof(float));

  int SCORES = atoi(argv[1]);
  PRINT_STAT_INT("scores", SCORES);
  float **score_vect = (float **)sirius_malloc(SCORES * sizeof(float *));
  for (int i = 0; i < SCORES; i++) {
    score_vect[i] = (float *)sirius_malloc(senone_size * sizeof(float));
    for(int j = 0; j < senone_size; ++j) {
      score_vect[i][j] = logZero;
    }
  }

  float **feat_vect = (float **)sirius_malloc(SCORES * sizeof(float *));
  for(int i = 0; i < SCORES; ++i) {
    feat_vect[i] = (float *)sirius_malloc(feat_size * sizeof(float));
    for(int j = 0; j < feat_size; ++j) {
      feat_vect[i][j] = feature_vect[j];
    }
  }

  // load model from file
  FILE *fp = fopen(argv[2], "r");
  if (fp == NULL) {
    printf("Can’t open file\n");
    exit(-1);
  }

  int idx = 0;
  for (int i = 0; i < senone_size; i++) {
    for (int j = 0; j < comp_size; j++) {
      for (int k = 0; k < comp_size; k++) {
        float elem;
        if (!fscanf(fp, "%f", &elem)) break;
        means_vect[idx] = elem;
        ++idx;
      }
    }
  }

  idx = 0;
  for (int i = 0; i < senone_size; i++) {
    for (int j = 0; j < comp_size; j++) {
      for (int k = 0; k < comp_size; k++) {
        float elem;
        if (!fscanf(fp, "%f", &elem)) break;
        precs_vect[idx] = elem;
        idx = idx + 1;
      }
    }
  }

  idx = 0;
  for (int i = 0; i < senone_size; i++) {
    for (int j = 0; j < comp_size; j++) {
      float elem;
      if (!fscanf(fp, "%f", &elem)) break;
      weight_vect[idx] = elem;
      idx = idx + 1;
    }
  }

  idx = 0;
  for (int i = 0; i < senone_size; i++) {
    for (int j = 0; j < comp_size; j++) {
      float elem;
      if (!fscanf(fp, "%f", &elem)) break;
      factor_vect[idx] = elem;
      idx = idx + 1;
    }
  }

  fclose(fp);

  tic();
  for(int i = 0; i < SCORES; ++i)
    computeScore_seq(feat_vect[i], means_vect, precs_vect, weight_vect,
        factor_vect, score_vect[i]);
  PRINT_STAT_DOUBLE("gmm", toc());

  STATS_END();

// write for correctness check
#if TESTING
  FILE *f = fopen("../input/gmm_scoring.baseline", "w");

  for(int i = 0; i < SCORES; ++i) {
    for (int k = 0; k < senone_size; ++k) {
      fprintf(f, "%.0f\n", score_vect[i][k]);
    }
  }

  fclose(f);
#endif

  /* Clean up and exit */
  sirius_free(means_vect);
  sirius_free(precs_vect);

  sirius_free(weight_vect);
  sirius_free(factor_vect);

  for (int i = 0; i < SCORES; ++i) {
    sirius_free(score_vect[i]);
    sirius_free(feat_vect[i]);
  }

  sirius_free(score_vect);
  sirius_free(feat_vect);

  return 0;
}
