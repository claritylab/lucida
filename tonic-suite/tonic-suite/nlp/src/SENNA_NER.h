#ifndef SENNA_NER_H
#define SENNA_NER_H

#include "tonic.h"

typedef struct SENNA_NER_ {
  /* sizes */
  int window_size;
  int ll_word_size;
  int ll_word_max_idx;
  int ll_caps_size;
  int ll_caps_max_idx;
  int ll_gazl_size;
  int ll_gazl_max_idx;
  int ll_gazm_size;
  int ll_gazm_max_idx;
  int ll_gazo_size;
  int ll_gazo_max_idx;
  int ll_gazp_size;
  int ll_gazp_max_idx;
  int input_state_size;
  int hidden_state_size;
  int output_state_size;

  /* weights */
  float *ll_word_weight;
  float *ll_caps_weight;
  float *ll_gazl_weight;
  float *ll_gazm_weight;
  float *ll_gazo_weight;
  float *ll_gazp_weight;
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
  int ll_gazt_padding_idx;

  /* service flag */
  bool service;
  bool debug;

  /* profiling */
  int calls;
  unsigned int apptime;
  unsigned int dnntime;

} SENNA_NER;

SENNA_NER *SENNA_NER_new(const char *path, const char *subpath);

int *SENNA_NER_forward(SENNA_NER *ner, const int *sentence_words,
                       const int *sentence_caps, const int *sentence_gazl,
                       const int *sentence_gazm, const int *sentence_gazo,
                       const int *sentence_gazp, TonicSuiteApp app);

void SENNA_NER_free(SENNA_NER *ner);

#endif
