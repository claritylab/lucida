#include <stdio.h>
#include <stdlib.h>

#include <math.h>
#include <float.h>
#include <math.h>
#include <sys/time.h>

#define LOOP 1

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
float *cpu_score_vect;
float *pthread_score_vect;

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
                      float *weight_vect, float *factor_vect,
                      float *score_vect) {
  float logZero = -3.4028235E38;
  float maxLogValue = 7097004.5;
  float minLogValue = -7443538.0;
  float naturalLogBase = (float)1.00011595E-4;
  float inverseNaturalLogBase = 9998.841;

  int comp_size = 32;
  int feat_size = 29;
  int senone_size = 5120;

  for (int i = 0; i < senone_size; i++) {
    score_vect[i] = logZero;
    // printf("Senone #%d", i);

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

      // double logInnerSummation = logToLinear(-logDifference);
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

      // float actual_comp = linearToLog(logInnerSummation);
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
  //    }
}

float calculateMiliseconds(timeval t1, timeval t2) {
  float elapsedTime;
  elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;
  elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;
  return elapsedTime;
}

int main(int argc, char *argv[]) {
  float *dev_feat_vect;

  timeval t1, t2;
  float tau_elapsedTime;
  float cpu_elapsedTime;
  float par_elapsedTime;

  int means_array_size = senone_size * comp_size * comp_size;
  int comp_array_size = senone_size * comp_size;

  means_vect = (float *)malloc(means_array_size * sizeof(float));
  precs_vect = (float *)malloc(means_array_size * sizeof(float));
  weight_vect = (float *)malloc(comp_array_size * sizeof(float));
  factor_vect = (float *)malloc(comp_array_size * sizeof(float));

  float *dev_means_vect;
  float *dev_precs_vect;
  float *dev_weight_vect;
  float *dev_factor_vect;

  score_vect = (float *)malloc(senone_size * sizeof(float));
  cpu_score_vect = (float *)malloc(senone_size * sizeof(float));
  pthread_score_vect = (float *)malloc(senone_size * sizeof(float));

  float *dev_score_vect;

  // load model from file
  FILE *fp = fopen(argv[1], "r");
  if (fp == NULL) {
    printf("Can’t open file\n");
    exit(-1);
  }

  int idx = 0;
  for (int i = 0; i < senone_size; i++) {
    for (int j = 0; j < comp_size; j++) {
      for (int k = 0; k < comp_size; k++) {
        float elem;
        fscanf(fp, "%f", &elem);
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

  // CPU side
  gettimeofday(&t1, NULL);
  for (int i = 0; i < LOOP; ++i)
    computeScore_seq(feature_vect, means_vect, precs_vect, weight_vect,
                     factor_vect, cpu_score_vect);
  gettimeofday(&t2, NULL);

  cpu_elapsedTime = calculateMiliseconds(t1, t2);
  printf("CPU Time=%4.3f ms\n", (float)cpu_elapsedTime / LOOP);

  /* Clean up and exit */
  free(means_vect);
  free(precs_vect);

  free(weight_vect);
  free(factor_vect);

  free(score_vect);
  free(cpu_score_vect);
  free(pthread_score_vect);

  return 0;
}
