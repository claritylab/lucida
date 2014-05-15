


#nvcc -lineinfo -g -G -arch=sm_30 gmm_scoring.cu -o gmm_scoring
#nvcc -lineinfo -g -G -pg -arch=sm_30 gmm_scoring.cu -o gmm_scoring
#nvcc -lineinfo -arch=sm_30 gmm_scoring.cu -o gmm_scoring

#nvcc -arch=sm_30 porter.cu -o cuda_porter -lrt

gcc -std=gnu99 -o porter porter.c -lpthread -lrt

