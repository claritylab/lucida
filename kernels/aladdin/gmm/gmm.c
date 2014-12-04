

#include "gmm.h"

void computeScore_seq(float* feature_vect, float* means_vect, float * precs_vect, float* weight_vect, float* factor_vect, float * score_vect) {

	float logZero = -3.4028235E38;

	//float logBase = 1.0001;

	float maxLogValue = 7097004.5;
	float minLogValue = -7443538.0;

	float naturalLogBase = (float) 1.00011595E-4;
	float inverseNaturalLogBase = 9998.841;

	int comp_size = 32;
	int feat_size = 29;
//	int senone_size = 5120;
	int senone_size = 5;
        int i,j,k;

	for (i = 0; i < senone_size; i++) {

		score_vect[i] = logZero;
		//            int sen_id = senone_ids[i];

		for (j = 0; j < comp_size; j++) {

			// getScore
			// idx = k + D*j + i*W*D
			float logDval = 0.0f;
			for (k = 0; k < feat_size; k++) {
				//float logDiff = featureVector[k] - mean_trans[k];
				//logDval += logDiff * logDiff * prec_trans[k];
				int idx = k + comp_size*j + i*comp_size*comp_size;
				float logDiff = feature_vect[k] - means_vect[idx];
				logDval += logDiff * logDiff * precs_vect[idx];
			}
			// System.out.println("NEW comp: " + i + " logDval:"+logDval + " after feature_vect");


			// Convert to the appropriate base.
			//logDval = logMath.lnToLog(logDval);
			if (logDval != logZero) {
				logDval = logDval * inverseNaturalLogBase;
			}

			int idx2 = j + i*comp_size;

			// Add the precomputed factor, with the appropriate sign.
			//  logDval -= mixtureComponents[i].getLogPreComputedGaussianFactor();
			logDval -= factor_vect[idx2];

			/*      if (Float.isNaN(logDval)) {
                    System.out.println("gs is Nan, converting to 0");
                    logDval = logZero;
                }*/

			if (logDval < logZero) {
				logDval = logZero;
			}
			// end of getScore
			//          System.out.println("NEW comp: " + i + " logDval:"+logDval + " after getScore, preFactor_array");

			//      float logVal2 = logDval + logMixtureWeights[i];
			float logVal2 = logDval + weight_vect[idx2];
			//        System.out.println("NEW comp: " + i + " logVal2:"+logVal2 + " after mixWeightArray");

			//float logVal2 = mixtureComponents[i].getScore(featureVector) + logMixtureWeights[i];
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

/*float calculateMiliseconds(struct timeval t1, struct timeval t2) {
	float elapsedTime;
	elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;
	elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;
	return elapsedTime;
}*/


int main()
{
//	struct timeval t1,t2;
//	float cpu_elapsedTime;

	int comp_size = 32;
//	int senone_size = 5120;
	int senone_size = 5;

	int means_array_size = senone_size*comp_size*comp_size;
	int comp_array_size = senone_size*comp_size;

	means_vect = (float *)malloc(means_array_size * sizeof(float));
	precs_vect = (float *)malloc(means_array_size * sizeof(float));
	weight_vect = (float *)malloc(comp_array_size * sizeof(float));
	factor_vect = (float *)malloc(comp_array_size * sizeof(float));

	score_vect = (float *)malloc(senone_size * sizeof(float));
        int i,j,k;

	// load model from file
	FILE *fp = fopen("/home/gpuser/cuda/gmm_data_51.txt", "r");
	if (fp == NULL) { //checks for the file
		printf("\n Can’t open file");
		exit(-1);
	}
/*	FILE *fpw = fopen("/home/gpuser/cuda/gmm_data_51.txt", "w");
	if (fpw == NULL) { //checks for the file
		printf("\n Can’t open file");
		exit(-1);
	}
*/

       //int idx = k + comp_size*j + i*comp_size*comp_size;
       //int idx2 = j + i*comp_size;

	int idx = 0;
	for (i = 0; i < senone_size; i++) {
		for (j = 0; j < comp_size; j++) {
			for (k = 0; k < comp_size; k++) {
				float elem;
				fscanf(fp, "%f", &elem);
				means_vect[idx] = elem;
				idx = idx + 1;
			}
		}
	}

	idx = 0;
	for (i = 0; i < senone_size; i++) {
		for (j = 0; j < comp_size; j++) {
			for (k = 0; k < comp_size; k++) {
				float elem;
				fscanf(fp, "%f", &elem);
				precs_vect[idx] = elem;
				idx = idx + 1;
			}
		}
	}

	idx = 0;
	for (i = 0; i < senone_size; i++) {
		for (j = 0; j < comp_size; j++) {
			float elem;
			fscanf(fp, "%f", &elem);
			weight_vect[idx] = elem;
			idx = idx + 1;
		}
	}

	idx = 0;
	for (i = 0; i < senone_size; i++) {
		for (j = 0; j < comp_size; j++) {
			float elem;
			fscanf(fp, "%f", &elem);
			factor_vect[idx] = elem;
			idx = idx + 1;
		}
	}

/*
        // write subset
        for (i = 0; i < senone_size_small; i++) {
                for (j = 0; j < comp_size; j++) {
                        for (k = 0; k < comp_size; k++) {
                                int idx = k + comp_size*j + i*comp_size*comp_size;
                                fprintf(fpw, "%f", means_vect[idx]);
                        }
                }
        }

        idx = 0;
        for (i = 0; i < senone_size_small; i++) {
                for (j = 0; j < comp_size; j++) {
                        for (k = 0; k < comp_size; k++) {
                                int idx = k + comp_size*j + i*comp_size*comp_size;
                                fprintf(fpw, "%f", precs_vect[idx]);
                                //float elem;
                                //fscanf(fp, "%f", &elem);
                                //precs_vect[idx] = elem;
                                //idx = idx + 1;
                        }
                }
        }

        idx = 0;
        for (i = 0; i < senone_size_small; i++) {
                for (j = 0; j < comp_size; j++) {
                     //   float elem;
                      //  fscanf(fp, "%f", &elem);
                      //  weight_vect[idx] = elem;
                       // idx = idx + 1;
                       int idx2 = j + i*comp_size;
                       fprintf(fpw, "%f", weight_vect[idx]);
                }
        }

        idx = 0;
        for (i = 0; i < senone_size_small; i++) {
                for (j = 0; j < comp_size; j++) {
                       int idx2 = j + i*comp_size;
                       fprintf(fpw, "%f", factor_vect[idx]);
                }
        }
*/

	fclose(fp);
//        fclose(fpw);

	// CPU side

//	gettimeofday(&t1, NULL);
	computeScore_seq(feature_vect, means_vect, precs_vect, weight_vect, factor_vect, score_vect);
//	gettimeofday(&t2, NULL);

//	cpu_elapsedTime = calculateMiliseconds(t1,t2);
//	printf("\nCPU Time=%4.3f ms\n",  cpu_elapsedTime);

	free(means_vect);
	free(precs_vect);

	free(weight_vect);
	free(factor_vect);

	free(score_vect);

}
