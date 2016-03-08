#ifndef SENNA_SRL_H
#define SENNA_SRL_H

typedef struct SENNA_SRL_ {
  /* sizes */
  int window_size;
  int ll_word_size;
  int ll_word_max_idx;
  int ll_caps_size;
  int ll_caps_max_idx;
  int ll_chkl_size;
  int ll_chkl_max_idx;
  int ll_posv_size;
  int ll_posv_max_idx;
  int ll_posw_size;
  int ll_posw_max_idx;
  int input_state_size;
  int hidden_state1_size;
  int hidden_state3_size;
  int output_state_size;

  /* weights */
  float *ll_word_weight;
  float *ll_caps_weight;
  float *ll_chkl_weight;
  float *ll_posv_weight;
  float *ll_posw_weight;
  float *l1_weight_wcc;
  float *l1_weight_pw;
  float *l1_weight_pv;
  float *l1_bias;
  float *l3_weight;
  float *l3_bias;
  float *l4_weight;
  float *l4_bias;
  float *viterbi_score_init;
  float *viterbi_score_trans;

  /* extra inputs */
  int *sentence_posv;
  int *sentence_posw;

  /* states */
  float *input_state;
  float *input_state_wcc;
  float *input_state_pw;
  float *input_state_pv;
  float *hidden_state1;
  float *hidden_state1_wcc;
  float *hidden_state1_pw;
  float *hidden_state1_pv;
  float *hidden_state2;
  float *hidden_state3;
  float *output_state;
  int **labels;
  int labels_size;

  /* padding indices */
  int ll_word_padding_idx;
  int ll_caps_padding_idx;
  int ll_chkl_padding_idx;

  /* service flag */
  bool service;
  bool debug;

  /* profiling */
  int calls;
  unsigned int apptime;
  unsigned int dnntime;

} SENNA_SRL;

SENNA_SRL *SENNA_SRL_new(const char *path, const char *subpath);
int **SENNA_SRL_forward(SENNA_SRL *srl, const int *sentence_words,
                        const int *sentence_caps, const int *sentence_chkl,
                        const int *sentence_isvb, int sentence_size,
                        int socketfd);
void SENNA_SRL_free(SENNA_SRL *srl);

#endif
