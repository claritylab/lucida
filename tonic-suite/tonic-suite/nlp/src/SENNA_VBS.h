#ifndef SENNA_VBS_H
#define SENNA_VBS_H

#include "SENNA_Hash.h"

typedef struct SENNA_VBS_ {
  /* sizes */
  int window_size;
  int ll_word_size;
  int ll_word_max_idx;
  int ll_caps_size;
  int ll_caps_max_idx;
  int ll_posl_size;
  int ll_posl_max_idx;
  int input_state_size;
  int hidden_state_size;
  int output_state_size;

  /* weights */
  float *ll_word_weight;
  float *ll_caps_weight;
  float *ll_posl_weight;
  float *l1_weight;
  float *l1_bias;
  float *l2_weight;
  float *l2_bias;

  /* states */
  float *input_state;
  float *hidden_state;
  float *output_state;
  int *labels;

  /* padding indices */
  int ll_word_padding_idx;
  int ll_caps_padding_idx;
  int ll_posl_padding_idx;

  /* service flag */
  bool service;
  bool debug;

  /* internal socket */
  int socketfd;

  /* profiling */
  int calls;
  unsigned int apptime;
  unsigned int dnntime;

} SENNA_VBS;

SENNA_VBS *SENNA_VBS_new(const char *path, const char *subpath);
int *SENNA_VBS_forward(SENNA_VBS *vbs, const int *sentence_words,
                       const int *sentence_caps, const int *sentence_posl,
                       int sentence_size, int socketfd);
void SENNA_VBS_free(SENNA_VBS *vbs);

#endif
