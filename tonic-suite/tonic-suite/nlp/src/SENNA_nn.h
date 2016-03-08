#ifndef SENNA_NN_H
#define SENNA_NN_H

void SENNA_nn_lookup(float *dest, int dest_stride, const float *wordweights,
                     int wordsize, int maxwordidx, const int *wordindices,
                     int nword, int padidx, int npad);
void SENNA_nn_hardtanh(float *output, float *input, int size);
void SENNA_nn_linear(float *output, int output_size, float *weights,
                     float *biases, float *input, int input_size);
void SENNA_nn_max(float *value_, int *idx_, float *input, int input_size);
void SENNA_nn_temporal_convolution(float *output, int output_frame_size,
                                   float *weights, float *biases, float *input,
                                   int input_frame_size, int n_frames, int k_w);
void SENNA_nn_temporal_max_convolution(float *output, float *bias, float *input,
                                       int input_frame_size, int n_frames,
                                       int k_w);
void SENNA_nn_temporal_max(float *output, float *input, int N, int T);
void SENNA_nn_distance(int *dest, int idx, int max_idx, int sentence_size,
                       int padding_size);
void SENNA_nn_viterbi(int *path, float *init, float *transition,
                      float *emission, int N, int T);

#endif
