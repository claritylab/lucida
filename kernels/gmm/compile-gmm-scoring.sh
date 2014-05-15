


#nvcc -lineinfo -g -G -arch=sm_30 gmm_scoring.cu -o gmm_scoring
#nvcc -lineinfo -g -G -pg -arch=sm_30 gmm_scoring.cu -o gmm_scoring
#nvcc -lineinfo -arch=sm_30 gmm_scoring.cu -o gmm_scoring

nvcc -arch=sm_30 gmm_scoring.cu -o gmm_scoring
