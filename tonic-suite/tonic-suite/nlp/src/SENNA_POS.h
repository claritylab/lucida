#ifndef SENNA_POS_H
#define SENNA_POS_H

#include "tonic.h"

typedef struct SENNA_POS_ {
  /* sizes */
  int window_size;
  int ll_word_size;
  int ll_word_max_idx;
  int ll_caps_size;
  int ll_caps_max_idx;
  int ll_suff_size;
  int ll_suff_max_idx;
  int input_state_size;
  int hidden_state_size;
  int output_state_size;

  /* weights */
  float *ll_word_weight;
  float *ll_caps_weight;
  float *ll_suff_weight;
  float *l1_weight;
  float *l1_bias;
  float *l2_weight;
  float *l2_bias;
  float *viterbi_score_init;
  float *viterbi_score_trans;

  /* states */
  float *input_state;
  float *hidden_state;
  float *output_state;
  int *labels;

  /* padding indices */
  int ll_word_padding_idx;
  int ll_caps_padding_idx;
  int ll_suff_padding_idx;

} SENNA_POS;

SENNA_POS *SENNA_POS_new(const char *path, const char *subpath);
int *SENNA_POS_forward(SENNA_POS *pos, const int *sentence_words,
                       const int *sentence_caps, const int *sentence_suff,
                       TonicSuiteApp app);
void SENNA_POS_free(SENNA_POS *pos);

#endif
