#include <stdio.h>
#include <stdlib.h>

#include <math.h>
#include <float.h>
#include <math.h>
#include <sys/time.h>
#include "simt.h"

#include <pthread.h>

#define NTHREADS      8
#define ARRAYSIZE   5120
#define ITERATIONS   ARRAYSIZE / NTHREADS

float feature_vect[] = { 2.240018, 2.2570236, 0.11304555, -0.21307051,
    0.8988138, 0.039065503, 0.023874786, 0.13153112, 0.15324382, 0.16986738,
    -0.020297153, -0.26773554, 0.40202165, 0.35923952,
    0.060746543, 0.35402644, 0.086052455, -0.10499257, 0.04395058, 0.026407119,
    -0.48301497, 0.120889395, 0.67980754, -0.19875681, -0.5443737, -0.039534688,
    0.20888293, 0.054865785, -0.4846478, 0.1, 0.1, 0.1};

float *means_vect;
float *precs_vect;
float *weight_vect;
float *factor_vect;

float *score_vect;
float *cpu_score_vect;
float *pthread_score_vect;

//pthread_mutex_t sum_mutex;

float logZero = -3.4028235E38;

float logBase = 1.0001;

float maxLogValue = 7097004.5;
float minLogValue = -7443538.0;

float naturalLogBase = (float) 1.00011595E-4;
float inverseNaturalLogBase = 9998.841;

int comp_size   = 32;
int feat_size   = 29;
int senone_size = 5120;

void computeScore(const float *feature_vect, float *means_vect, float *precs_vect, float *weight_vect, float *factor_vect, float *score_vect)
{
//     int i = blockIdx.x * blockDim.x + threadIdx.x;
//
//     if (i < senone_size) {
//
//         float local_score_vect = logZero;
//
// #pragma unroll 32
//         for (int j = 0; j < comp_size; j++) {
//             // getScore
//             float logDval = 0.0f;
// #pragma unroll 29
//             for (int k = 0; k < feat_size; k++) {
//                 int idx = i + senone_size*j + k*comp_size*senone_size;
//                 float logDiff = feature_vect[k] - means_vect[idx];
//                 logDval += logDiff * logDiff * precs_vect[idx];
//             }
//
//             // Convert to the appropriate base.
//             //logDval = logMath.lnToLog(logDval);
//             if (logDval != logZero) {
//                 logDval = logDval * inverseNaturalLogBase;
//             }
//
//             int idx2 = i + j*senone_size;
//             //  int idx2 = j + i*comp_size;
//
//             //    int idx2 = senone_id + j*comp_size;
//
//             // Add the precomputed factor, with the appropriate sign.
//             //  logDval -= mixtureComponents[i].getLogPreComputedGaussianFactor();
//             logDval -= factor_vect[idx2];
//
//             /*      if (Float.isNaN(logDval)) {
//                     System.out.println("gs is Nan, converting to 0");
//                     logDval = logZero;
//                     }*/
//
//             if (logDval < logZero) {
//                 logDval = logZero;
//             }
//             // end of getScore
//
//             float logVal2 = logDval + weight_vect[idx2];
//
//             float logHighestValue = local_score_vect;
//             float logDifference = local_score_vect - logVal2;
//
//             // difference is always a positive number
//             //            float logHighestValue = (logDifference1 < 0)? logVal2 : local_score_vect;
//             //           float logDifference =  (logDifference1 < 0)? -logDifference1 : logDifference1;
//             if (logDifference < 0) {
//                 logHighestValue = logVal2;
//                 logDifference = -logDifference;
//             }
//
//             //double logInnerSummation = logToLinear(-logDifference);
//             float logValue = -logDifference;
//             float logInnerSummation;
//             if (logValue < minLogValue) {
//                 logInnerSummation = 0.0;
//             } else if (logValue > maxLogValue) {
//                 logInnerSummation = FLT_MAX;
//             } else {
//                 if (logValue == logZero) {
//                     logValue = logZero;
//                 } else {
//                     logValue = logValue * naturalLogBase;
//                 }
//                 logInnerSummation = __expf(logValue);
//             }
//
//             logInnerSummation += 1.0;
//
//             //float actual_comp = linearToLog(logInnerSummation);
//             float returnLogValue;
//             if (logInnerSummation <= 0.0) {
//                 returnLogValue = logZero;
//             } else {
//                 returnLogValue = __logf(logInnerSummation) * inverseNaturalLogBase;
//                 if (returnLogValue > FLT_MAX) {
//                     returnLogValue = FLT_MAX;
//                 } else if (returnLogValue < -FLT_MAX) {
//                     returnLogValue = -FLT_MAX;
//                 }
//             }
//             // sum log
//             local_score_vect = logHighestValue + returnLogValue;
//         }
//
//         score_vect[i] = local_score_vect;
//
//         //        i += blockDim.x * gridDim.x;
//
//     }
//     // __syncthreads();
//
}

void computeScore_seq(float* feature_vect, float* means_vect, float * precs_vect, float* weight_vect, float* factor_vect, float * score_vect)
{

    float logZero = -3.4028235E38;
    float maxLogValue = 7097004.5;
    float minLogValue = -7443538.0;
    float naturalLogBase = (float) 1.00011595E-4;
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
                int idx = k + comp_size*j + i*comp_size*comp_size;
                float logDiff = feature_vect[k] - means_vect[idx];
                logDval += logDiff * logDiff * precs_vect[idx];
            }

            // Convert to the appropriate base.
            if (logDval != logZero) {
                logDval = logDval * inverseNaturalLogBase;
            }

            int idx2 = j + i*comp_size;
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

            //double logInnerSummation = logToLinear(-logDifference);
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

            //float actual_comp = linearToLog(logInnerSummation);
            float returnLogValue;
            if (logInnerSummation <= 0.0) {
                returnLogValue = logZero;

            } else {
                returnLogValue = (float) (log(logInnerSummation) * inverseNaturalLogBase);
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

void tau_computeScore(const float *feature_vect, float *means_vect, float *precs_vect, float *weight_vect, float *factor_vect, float *score_vect)
{
    float logZero = -3.4028235E38;
    float maxLogValue = 7097004.5;
    float minLogValue = -7443538.0;
    float naturalLogBase = (float) 1.00011595E-4;
    float inverseNaturalLogBase = 9998.841;

    int senone_size = 5120;
    int comp_size   = 32;
    int feat_size   = 29;

    ar::simt_tau::par_for(senone_size*comp_size,[&, score_vect](size_t idx){
        float logDval = 0.0f;
        int out = idx / senone_size;
        int in  = idx % comp_size;

        score_vect[out] = logZero;

        for (int k = 0; k < feat_size; k++)
        {
            int idx = k + in*comp_size + out*comp_size*comp_size;
            float logDiff = feature_vect[k] - means_vect[idx];
            logDval += logDiff * logDiff * precs_vect[idx];
        }

        // Convert to the appropriate base.
        if (logDval != logZero) {
            logDval = logDval * inverseNaturalLogBase;
        }

        int idx2 = in + out*comp_size;
        logDval -= factor_vect[idx2];

        if (logDval < logZero) {
            logDval = logZero;
        }

        float logVal2 = logDval + weight_vect[idx2];

        float logHighestValue = score_vect[out*comp_size];
        float logDifference = score_vect[out*comp_size] - logVal2;
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
            returnLogValue = (float) (log(logInnerSummation) * inverseNaturalLogBase);
            returnLogValue = (returnLogValue > FLT_MAX)  ? FLT_MAX 
                           : (returnLogValue < -FLT_MAX) ? -FLT_MAX 
                           : returnLogValue;
        }
        // sum log
        score_vect[out] = logHighestValue + returnLogValue;
    });
}

void *computeScore_thread(void *tid)
{
    // float logZero = -3.4028235E38;
    //
    // //float logBase = 1.0001;
    //
    // float maxLogValue = 7097004.5;
    // float minLogValue = -7443538.0;
    //
    // float naturalLogBase = (float) 1.00011595E-4;
    // float inverseNaturalLogBase = 9998.841;
    //
    // int comp_size = 32;
    // int feat_size = 29;
    // int senone_size = 5120;
    //
    // int i, start, *mytid, end;
    //
    // mytid = (int *) tid;
    // start = (*mytid * ITERATIONS);
    // end = start + ITERATIONS;
    // printf ("Thread %d doing iterations %d to %d\n",*mytid,start,end-1);
    //
    // //for (int i = 0; i < senone_size; i++) {
    // for (i=start; i < end ; i++) {
    //
    //     pthread_score_vect[i] = logZero;
    //     //            int sen_id = senone_ids[i];
    //
    //     for (int j = 0; j < comp_size; j++) {
    //
    //         // getScore
    //         // idx = k + D*j + i*W*D
    //         float logDval = 0.0f;
    //         for (int k = 0; k < feat_size; k++) {
    //             //float logDiff = featureVector[k] - mean_trans[k];
    //             //logDval += logDiff * logDiff * prec_trans[k];
    //             int idx = k + comp_size*j + i*comp_size*comp_size;
    //             float logDiff = feature_vect[k] - means_vect[idx];
    //             logDval += logDiff * logDiff * precs_vect[idx];
    //         }
    //         // System.out.println("NEW comp: " + i + " logDval:"+logDval + " after feature_vect");
    //
    //
    //         // Convert to the appropriate base.
    //         //logDval = logMath.lnToLog(logDval);
    //         if (logDval != logZero) {
    //             logDval = logDval * inverseNaturalLogBase;
    //         }
    //
    //         int idx2 = j + i*comp_size;
    //
    //         // Add the precomputed factor, with the appropriate sign.
    //         //  logDval -= mixtureComponents[i].getLogPreComputedGaussianFactor();
    //         logDval -= factor_vect[idx2];
    //
    //         /*      if (Float.isNaN(logDval)) {
    //                 System.out.println("gs is Nan, converting to 0");
    //                 logDval = logZero;
    //                 }*/
    //
    //         if (logDval < logZero) {
    //             logDval = logZero;
    //         }
    //         // end of getScore
    //         //          System.out.println("NEW comp: " + i + " logDval:"+logDval + " after getScore, preFactor_array");
    //
    //         //      float logVal2 = logDval + logMixtureWeights[i];
    //         float logVal2 = logDval + weight_vect[idx2];
    //         //        System.out.println("NEW comp: " + i + " logVal2:"+logVal2 + " after mixWeightArray");
    //
    //         //float logVal2 = mixtureComponents[i].getScore(featureVector) + logMixtureWeights[i];
    //         float logHighestValue = score_vect[i];
    //         float logDifference = score_vect[i] - logVal2;
    //
    //         // difference is always a positive number
    //         if (logDifference < 0) {
    //             logHighestValue = logVal2;
    //             logDifference = -logDifference;
    //         }
    //
    //         //double logInnerSummation = logToLinear(-logDifference);
    //         float logValue = -logDifference;
    //         float logInnerSummation;
    //         if (logValue < minLogValue) {
    //             logInnerSummation = 0.0;
    //         } else if (logValue > maxLogValue) {
    //             logInnerSummation = FLT_MAX;
    //
    //         } else {
    //             if (logValue == logZero) {
    //                 logValue = logZero;
    //             } else {
    //                 logValue = logValue * naturalLogBase;
    //             }
    //             logInnerSummation = exp(logValue);
    //         }
    //
    //         logInnerSummation += 1.0;
    //
    //         //float actual_comp = linearToLog(logInnerSummation);
    //         float returnLogValue;
    //         if (logInnerSummation <= 0.0) {
    //             returnLogValue = logZero;
    //
    //         } else {
    //             returnLogValue = (float) (log(logInnerSummation) * inverseNaturalLogBase);
    //             if (returnLogValue > FLT_MAX) {
    //                 returnLogValue = FLT_MAX;
    //             } else if (returnLogValue < -FLT_MAX) {
    //                 returnLogValue = -FLT_MAX;
    //             }
    //         }
    //         // sum log
    //         pthread_score_vect[i] = logHighestValue + returnLogValue;
    //     }
    // }
    //
    // pthread_exit(NULL);
    //    }
}

float calculateMiliseconds(timeval t1,timeval t2)
{
    float elapsedTime;
    elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;
    elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;
    return elapsedTime;
}

int main()
{
    float *dev_feat_vect;

    timeval t1,t2;
    float tau_elapsedTime;
    float cpu_elapsedTime;
    float par_elapsedTime;

    int comp_size = 32;
    int senone_size = 5120;

    int means_array_size = senone_size*comp_size*comp_size;
    int comp_array_size = senone_size*comp_size;

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
    cpu_score_vect = (float *)malloc(senone_size * sizeof(float));
    pthread_score_vect = (float *)malloc(senone_size * sizeof(float));

    float *dev_score_vect;

    // load model from file
    // FILE *fp = fopen("/home/jahausw/umvoice/kernels/gmm/gmm_data.txt", "r");
    // if (fp == NULL) { //checks for the file
    //     printf("\n Can’t open file");
    //     exit(-1);
    // }
    // printf("here\n");

    int idx = 0;
    for (int i = 0; i < senone_size; i++) {
        for (int j = 0; j < comp_size; j++) {
            for (int k = 0; k < comp_size; k++) {
                float elem;
                // fscanf(fp, "%f", &elem);
                // means_vect[idx] = elem;
                means_vect[idx] = idx;
                ++idx;
            }
        }
    }

    idx = 0;
    for (int i = 0; i < senone_size; i++) {
        for (int j = 0; j < comp_size; j++) {
            for (int k = 0; k < comp_size; k++) {
                float elem;
                // fscanf(fp, "%f", &elem);
                // precs_vect[idx] = elem;
                precs_vect[idx] = idx;
                idx = idx + 1;
            }
        }
    }

    idx = 0;
    for (int i = 0; i < senone_size; i++) {
        for (int j = 0; j < comp_size; j++) {
            float elem;
            // fscanf(fp, "%f", &elem);
            // weight_vect[idx] = elem;
            weight_vect[idx] = idx;
            idx = idx + 1;
        }
    }

    idx = 0;
    for (int i = 0; i < senone_size; i++) {
        for (int j = 0; j < comp_size; j++) {
            float elem;
            // fscanf(fp, "%f", &elem);
            factor_vect[idx] = idx;
            idx = idx + 1;
        }
    }

    // fclose(fp);

    int idx3 = 0;
    for (int j = 0; j < comp_size; j++) {
        for (int i = 0; i < senone_size; i++) {
            int ij = j + i*comp_size;
            weight_vect2[idx3] = weight_vect[ij];
            factor_vect2[idx3] = factor_vect[ij];
            idx3 += 1;
        }
    }

    int idx4 = 0;
    for (int k = 0; k < comp_size; k++) {
        for (int j = 0; j < comp_size; j++) {
            for (int i = 0; i < senone_size; i++) {
                int ijk = k + comp_size*j + i*comp_size*comp_size;
                means_vect2[idx4] = means_vect[ijk];
                precs_vect2[idx4] = precs_vect[ijk];
                idx4 += 1;
            }
        }
    }

    for (int i = 0; i < senone_size; i++) {
        for (int j = 0; j < comp_size; j++) {
            for (int k = 0; k < 29; k++) {
                int ijk = k + comp_size*j + i*comp_size*comp_size;
                int kji = i + senone_size*j + k*comp_size*senone_size;
                if (means_vect2[kji] != means_vect[ijk]) {
                    printf("%f != %f\n", means_vect2[kji], means_vect[ijk]);
                }
            }
        }
    }
    gettimeofday(&t1, NULL);
    tau_computeScore(feature_vect, means_vect, precs_vect, weight_vect, factor_vect, score_vect);
    gettimeofday(&t2, NULL);
    //
    tau_elapsedTime = calculateMiliseconds(t1,t2);
    printf("TAU Time=%4.3f ms\n", tau_elapsedTime);
    
    
    // CPU side
    gettimeofday(&t1, NULL);
    computeScore_seq(feature_vect, means_vect, precs_vect, weight_vect, factor_vect, cpu_score_vect);
    gettimeofday(&t2, NULL);
    
    cpu_elapsedTime = calculateMiliseconds(t1,t2);
    printf("\nCPU Time=%4.3f ms\n",  cpu_elapsedTime);
    
    for (int k = 0; k < senone_size; k++) {
        if (abs(abs(cpu_score_vect[k] - score_vect[k]) / cpu_score_vect[k]) > 0.01) {
            printf("ERROR on computing scores: CPU %.3f != GPU %.3f\n", cpu_score_vect[k], score_vect[k]);
            //  printf(abs(cpu_score_vect[k] - score_vect[k]) / cpu_score_vect[k]);
        }
    }
    
    // gettimeofday(&t1, NULL);
    //
    // pthread_attr_init(&attr);
    // pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    // for (i=0; i<NTHREADS; i++) {
    //     tids[i] = i;
    //     pthread_create(&threads[i], &attr, computeScore_thread, (void *) &tids[i]);
    // }
    //
    // for (i=0; i<NTHREADS; i++) {
    //     pthread_join(threads[i], NULL);
    // }
    //
    // gettimeofday(&t2, NULL);
    
    // par_elapsedTime = calculateMiliseconds(t1,t2);
    // printf("\nCPU Par Time=%4.3f ms\n",  par_elapsedTime);
    
    
    // printf("\nCPU Par speedup over CPU = %4.3f\n",  cpu_elapsedTime/par_elapsedTime);
    
    printf("\nTAU speedup over CPU = %4.3f\n",  cpu_elapsedTime/tau_elapsedTime);
    
    // printf("\nGPU speedup over CPU Par = %4.3f\n",  par_elapsedTime/tau_elapsedTime);
    
    free(means_vect);
    free(precs_vect);
    
    free(weight_vect);
    free(factor_vect);
    
    free(score_vect);
    free(cpu_score_vect);
    free(pthread_score_vect);

    /* Clean up and exit */
    // pthread_attr_destroy(&attr);
    //pthread_exit (NULL);
    
    return 0;
}
