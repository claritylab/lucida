#include <sys/time.h>
#include "SENNA_SRL.h"
#include "SENNA_utils.h"
#include "SENNA_nn.h"
#include "socket.h"

int **SENNA_SRL_forward(SENNA_SRL *srl, const int *sentence_words,
                        const int *sentence_caps, const int *sentence_chkl,
                        const int *sentence_isvb, int sentence_size,
                        int socketfd) {
  int vbidx;
  int idx;
  int n_verbs = 0;
  int i;
  struct timeval tv1, tv2;

  gettimeofday(&tv1, NULL);
  srl->sentence_posv = SENNA_realloc(srl->sentence_posv, sizeof(int),
                                     sentence_size + srl->window_size - 1);
  srl->sentence_posw = SENNA_realloc(srl->sentence_posw, sizeof(int),
                                     sentence_size + srl->window_size - 1);

  srl->input_state_wcc = SENNA_realloc(
      srl->input_state_wcc, sizeof(float),
      (sentence_size + srl->window_size - 1) *
          (srl->ll_word_size + srl->ll_caps_size + srl->ll_chkl_size));
  srl->input_state_pv =
      SENNA_realloc(srl->input_state_pv, sizeof(float),
                    (sentence_size + srl->window_size - 1) * srl->ll_posv_size);
  srl->input_state_pw =
      SENNA_realloc(srl->input_state_pw, sizeof(float),
                    (sentence_size + srl->window_size - 1) * srl->ll_posw_size);
  srl->hidden_state1_wcc =
      SENNA_realloc(srl->hidden_state1_wcc, sizeof(float),
                    sentence_size * srl->hidden_state1_size);
  srl->hidden_state1_pv =
      SENNA_realloc(srl->hidden_state1_pv, sizeof(float),
                    sentence_size * srl->hidden_state1_size);
  srl->hidden_state1_pw =
      SENNA_realloc(srl->hidden_state1_pw, sizeof(float),
                    sentence_size * srl->hidden_state1_size);
  srl->hidden_state1 = SENNA_realloc(srl->hidden_state1, sizeof(float),
                                     sentence_size * srl->hidden_state1_size);
  srl->hidden_state2 =
      SENNA_realloc(srl->hidden_state2, sizeof(float), srl->hidden_state1_size);
  srl->hidden_state3 =
      SENNA_realloc(srl->hidden_state3, sizeof(float), srl->hidden_state3_size);
  srl->output_state = SENNA_realloc(srl->output_state, sizeof(float),
                                    sentence_size * srl->output_state_size);

  /* words and caps are common for all words and all verbs */
  SENNA_nn_lookup(srl->input_state_wcc,
                  srl->ll_word_size + srl->ll_caps_size + srl->ll_chkl_size,
                  srl->ll_word_weight, srl->ll_word_size, srl->ll_word_max_idx,
                  sentence_words, sentence_size, srl->ll_word_padding_idx,
                  (srl->window_size - 1) / 2);
  SENNA_nn_lookup(srl->input_state_wcc + srl->ll_word_size,
                  srl->ll_word_size + srl->ll_caps_size + srl->ll_chkl_size,
                  srl->ll_caps_weight, srl->ll_caps_size, srl->ll_caps_max_idx,
                  sentence_caps, sentence_size, srl->ll_caps_padding_idx,
                  (srl->window_size - 1) / 2);
  SENNA_nn_lookup(srl->input_state_wcc + srl->ll_word_size + srl->ll_caps_size,
                  srl->ll_word_size + srl->ll_caps_size + srl->ll_chkl_size,
                  srl->ll_chkl_weight, srl->ll_chkl_size, srl->ll_chkl_max_idx,
                  sentence_chkl, sentence_size, srl->ll_chkl_padding_idx,
                  (srl->window_size - 1) / 2);

  SENNA_nn_temporal_convolution(
      srl->hidden_state1_wcc, srl->hidden_state1_size, srl->l1_weight_wcc,
      srl->l1_bias, srl->input_state_wcc,
      srl->ll_word_size + srl->ll_caps_size + srl->ll_chkl_size,
      sentence_size + srl->window_size - 1, srl->window_size);
  gettimeofday(&tv2, NULL);
  srl->apptime +=
      (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);

  /* for all verbs... */
  for (vbidx = 0; vbidx < sentence_size; vbidx++) {
    gettimeofday(&tv1, NULL);
    if (!sentence_isvb[vbidx]) continue;

    SENNA_nn_distance(srl->sentence_posv, vbidx, srl->ll_posv_max_idx,
                      sentence_size, (srl->window_size - 1) / 2);
    SENNA_nn_lookup(srl->input_state_pv, srl->ll_posv_size, srl->ll_posv_weight,
                    srl->ll_posv_size, srl->ll_posv_max_idx, srl->sentence_posv,
                    sentence_size + srl->window_size - 1, 0, 0);
    SENNA_nn_temporal_convolution(
        srl->hidden_state1_pv, srl->hidden_state1_size, srl->l1_weight_pv, NULL,
        srl->input_state_pv, srl->ll_posv_size,
        sentence_size + srl->window_size - 1, srl->window_size);
    gettimeofday(&tv2, NULL);
    srl->apptime +=
        (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);

    /* for all words... */
    for (idx = 0; idx < sentence_size; idx++) {
      gettimeofday(&tv1, NULL);
      SENNA_nn_distance(srl->sentence_posw, idx, srl->ll_posw_max_idx,
                        sentence_size, (srl->window_size - 1) / 2);
      SENNA_nn_lookup(srl->input_state_pw, srl->ll_posw_size,
                      srl->ll_posw_weight, srl->ll_posw_size,
                      srl->ll_posw_max_idx, srl->sentence_posw,
                      sentence_size + srl->window_size - 1, 0, 0);
      SENNA_nn_temporal_convolution(
          srl->hidden_state1_pw, srl->hidden_state1_size, srl->l1_weight_pw,
          NULL, srl->input_state_pw, srl->ll_posw_size,
          sentence_size + srl->window_size - 1, srl->window_size);

      memcpy(srl->hidden_state1, srl->hidden_state1_wcc,
             sizeof(float) * srl->hidden_state1_size * sentence_size);

      for (i = 0; i < srl->hidden_state1_size * sentence_size; i++)
        srl->hidden_state1[i] += srl->hidden_state1_pv[i];

      for (i = 0; i < srl->hidden_state1_size * sentence_size; i++)
        srl->hidden_state1[i] += srl->hidden_state1_pw[i];

      SENNA_nn_temporal_max(srl->hidden_state2, srl->hidden_state1,
                            srl->hidden_state1_size, sentence_size);
      gettimeofday(&tv2, NULL);
      srl->apptime +=
          (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);

      gettimeofday(&tv1, NULL);
      if (srl->service) {
        SOCKET_send(socketfd, (char *)(srl->hidden_state2),
                    srl->hidden_state1_size * sizeof(float), srl->debug);
        SOCKET_receive(socketfd, (char *)(srl->output_state +
                                          idx * srl->output_state_size),
                       srl->output_state_size * sizeof(float), srl->debug);
      } else {
        SENNA_nn_linear(srl->hidden_state3, srl->hidden_state3_size,
                        srl->l3_weight, srl->l3_bias, srl->hidden_state2,
                        srl->hidden_state1_size);
        SENNA_nn_hardtanh(srl->hidden_state3, srl->hidden_state3,
                          srl->hidden_state3_size);
        SENNA_nn_linear(srl->output_state + idx * srl->output_state_size,
                        srl->output_state_size, srl->l4_weight, srl->l4_bias,
                        srl->hidden_state3, srl->hidden_state3_size);
      }
      gettimeofday(&tv2, NULL);
      srl->dnntime +=
          (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);
      srl->calls++;
    }

    gettimeofday(&tv1, NULL);
    if (n_verbs >= srl->labels_size) {
      srl->labels = SENNA_realloc(srl->labels, sizeof(int *), n_verbs + 1);
      for (i = srl->labels_size; i < n_verbs + 1; i++) srl->labels[i] = NULL;
      srl->labels_size = n_verbs + 1;
    }
    srl->labels[n_verbs] =
        SENNA_realloc(srl->labels[n_verbs], sizeof(int), sentence_size);
    SENNA_nn_viterbi(srl->labels[n_verbs], srl->viterbi_score_init,
                     srl->viterbi_score_trans, srl->output_state,
                     srl->output_state_size, sentence_size);

    n_verbs++;
    gettimeofday(&tv2, NULL);
    srl->apptime +=
        (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);
  }

  return srl->labels;
}

SENNA_SRL *SENNA_SRL_new(const char *path, const char *subpath) {
  SENNA_SRL *srl = SENNA_malloc(sizeof(SENNA_SRL), 1);
  FILE *f;
  float dummy;
  int dummy_size;

  f = SENNA_fopen(path, subpath, "rb");

  SENNA_fread(&srl->window_size, sizeof(int), 1, f);
  SENNA_fread_tensor_2d(&srl->ll_word_weight, &srl->ll_word_size,
                        &srl->ll_word_max_idx, f);
  SENNA_fread_tensor_2d(&srl->ll_caps_weight, &srl->ll_caps_size,
                        &srl->ll_caps_max_idx, f);
  SENNA_fread_tensor_2d(&srl->ll_chkl_weight, &srl->ll_chkl_size,
                        &srl->ll_chkl_max_idx, f);
  SENNA_fread_tensor_2d(&srl->ll_posv_weight, &srl->ll_posv_size,
                        &srl->ll_posv_max_idx, f);
  SENNA_fread_tensor_2d(&srl->ll_posw_weight, &srl->ll_posw_size,
                        &srl->ll_posw_max_idx, f);
  SENNA_fread_tensor_2d(&srl->l1_weight_wcc, &dummy_size,
                        &srl->hidden_state1_size, f);
  SENNA_fread_tensor_2d(&srl->l1_weight_pv, &dummy_size,
                        &srl->hidden_state1_size, f);
  SENNA_fread_tensor_2d(&srl->l1_weight_pw, &dummy_size,
                        &srl->hidden_state1_size, f);
  SENNA_fread_tensor_1d(&srl->l1_bias, &srl->hidden_state1_size, f);
  SENNA_fread_tensor_2d(&srl->l3_weight, &srl->hidden_state1_size,
                        &srl->hidden_state3_size, f);
  SENNA_fread_tensor_1d(&srl->l3_bias, &srl->hidden_state3_size, f);
  SENNA_fread_tensor_2d(&srl->l4_weight, &srl->hidden_state3_size,
                        &srl->output_state_size, f);
  SENNA_fread_tensor_1d(&srl->l4_bias, &srl->output_state_size, f);
  SENNA_fread_tensor_1d(&srl->viterbi_score_init, &srl->output_state_size, f);
  SENNA_fread_tensor_2d(&srl->viterbi_score_trans, &srl->output_state_size,
                        &srl->output_state_size, f);

  SENNA_fread(&srl->ll_word_padding_idx, sizeof(int), 1, f);
  SENNA_fread(&srl->ll_caps_padding_idx, sizeof(int), 1, f);
  SENNA_fread(&srl->ll_chkl_padding_idx, sizeof(int), 1, f);

  SENNA_fread(&dummy, sizeof(float), 1, f);
  SENNA_fclose(f);

  if ((int)dummy != 777)
    SENNA_error("srl: data corrupted (or not IEEE floating computer)");

  /* states */
  srl->sentence_posv = NULL;
  srl->sentence_posw = NULL;
  srl->input_state = NULL;
  srl->input_state_wcc = NULL;
  srl->input_state_pv = NULL;
  srl->input_state_pw = NULL;
  srl->hidden_state1 = NULL;
  srl->hidden_state1_wcc = NULL;
  srl->hidden_state1_pv = NULL;
  srl->hidden_state1_pw = NULL;
  srl->hidden_state2 = NULL;
  srl->hidden_state3 = NULL;
  srl->output_state = NULL;
  srl->labels = NULL;
  srl->labels_size = 0;

  srl->service = false;
  srl->debug = false;
  srl->calls = 0;
  srl->dnntime = 0;
  srl->apptime = 0;

  /* some info if you want verbose */
  SENNA_message("srl: window size: %d", srl->window_size);
  SENNA_message("srl: vector size in word lookup table: %d", srl->ll_word_size);
  SENNA_message("srl: word lookup table size: %d", srl->ll_word_max_idx);
  SENNA_message("srl: vector size in caps lookup table: %d", srl->ll_caps_size);
  SENNA_message("srl: caps lookup table size: %d", srl->ll_caps_max_idx);
  SENNA_message("srl: vector size in verb position lookup table: %d",
                srl->ll_posv_size);
  SENNA_message("srl: verb position lookup table size: %d",
                srl->ll_posv_max_idx);
  SENNA_message("srl: vector size in word position lookup table: %d",
                srl->ll_posw_size);
  SENNA_message("srl: word position lookup table size: %d",
                srl->ll_posw_max_idx);
  SENNA_message("srl: number of hidden units (convolution): %d",
                srl->hidden_state1_size);
  SENNA_message("srl: number of hidden units (hidden layer): %d",
                srl->hidden_state3_size);
  SENNA_message("srl: number of classes: %d", srl->output_state_size);

  return srl;
}

void SENNA_SRL_free(SENNA_SRL *srl) {
  int i;

  /* weights */
  SENNA_free(srl->ll_word_weight);
  SENNA_free(srl->ll_caps_weight);
  SENNA_free(srl->ll_chkl_weight);
  SENNA_free(srl->ll_posv_weight);
  SENNA_free(srl->ll_posw_weight);
  SENNA_free(srl->l1_weight_wcc);
  SENNA_free(srl->l1_weight_pv);
  SENNA_free(srl->l1_weight_pw);
  SENNA_free(srl->l1_bias);
  SENNA_free(srl->l3_weight);
  SENNA_free(srl->l3_bias);
  SENNA_free(srl->l4_weight);
  SENNA_free(srl->l4_bias);
  SENNA_free(srl->viterbi_score_init);
  SENNA_free(srl->viterbi_score_trans);

  /* extra inputs */
  SENNA_free(srl->sentence_posw);
  SENNA_free(srl->sentence_posv);

  /* states */
  SENNA_free(srl->input_state);
  SENNA_free(srl->input_state_wcc);
  SENNA_free(srl->input_state_pv);
  SENNA_free(srl->input_state_pw);
  SENNA_free(srl->hidden_state1);
  SENNA_free(srl->hidden_state1_wcc);
  SENNA_free(srl->hidden_state1_pv);
  SENNA_free(srl->hidden_state1_pw);
  SENNA_free(srl->hidden_state2);
  SENNA_free(srl->hidden_state3);
  SENNA_free(srl->output_state);
  for (i = 0; i < srl->labels_size; i++) SENNA_free(srl->labels[i]);
  SENNA_free(srl->labels);

  /* the end */
  SENNA_free(srl);
}
