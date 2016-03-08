#include <sys/time.h>
#include "SENNA_PT0.h"
#include "SENNA_utils.h"
#include "SENNA_nn.h"
#include "socket.h"

int *SENNA_PT0_forward(SENNA_PT0 *pt0, const int *sentence_words,
                       const int *sentence_caps, const int *sentence_posl,
                       int sentence_size, int socketfd) {
  int idx;
  struct timeval tv1, tv2;

  gettimeofday(&tv1, NULL);
  pt0->input_state = SENNA_realloc(
      pt0->input_state, sizeof(float),
      (sentence_size + pt0->window_size - 1) *
          (pt0->ll_word_size + pt0->ll_caps_size + pt0->ll_posl_size));
  pt0->output_state = SENNA_realloc(pt0->output_state, sizeof(float),
                                    sentence_size * pt0->output_state_size);

  SENNA_nn_lookup(pt0->input_state,
                  pt0->ll_word_size + pt0->ll_caps_size + pt0->ll_posl_size,
                  pt0->ll_word_weight, pt0->ll_word_size, pt0->ll_word_max_idx,
                  sentence_words, sentence_size, pt0->ll_word_padding_idx,
                  (pt0->window_size - 1) / 2);
  SENNA_nn_lookup(pt0->input_state + pt0->ll_word_size,
                  pt0->ll_word_size + pt0->ll_caps_size + pt0->ll_posl_size,
                  pt0->ll_caps_weight, pt0->ll_caps_size, pt0->ll_caps_max_idx,
                  sentence_caps, sentence_size, pt0->ll_caps_padding_idx,
                  (pt0->window_size - 1) / 2);
  SENNA_nn_lookup(pt0->input_state + pt0->ll_word_size + pt0->ll_caps_size,
                  pt0->ll_word_size + pt0->ll_caps_size + pt0->ll_posl_size,
                  pt0->ll_posl_weight, pt0->ll_posl_size, pt0->ll_posl_max_idx,
                  sentence_posl, sentence_size, pt0->ll_posl_padding_idx,
                  (pt0->window_size - 1) / 2);
  gettimeofday(&tv2, NULL);
  pt0->apptime +=
      (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);

  int input_size = pt0->ll_word_size + pt0->ll_caps_size + pt0->ll_posl_size;

  char *input_data = (char *)malloc(
      sentence_size * (pt0->window_size * (input_size)) * sizeof(float));

  for (idx = 0; idx < sentence_size; idx++) {
    memcpy((char *)(input_data +
                    idx * (pt0->window_size) * (input_size) * sizeof(float)),
           (char *)(pt0->input_state + idx * input_size),
           pt0->window_size * input_size * sizeof(float));
  }
  if (pt0->service) {
    SOCKET_send(socketfd, input_data,
                pt0->window_size * (input_size) * sizeof(float) * sentence_size,
                pt0->debug);
    SOCKET_receive(socketfd, (char *)(pt0->output_state),
                   pt0->output_state_size * sizeof(float) * sentence_size,
                   pt0->debug);
  }

  /*
    for(idx = 0; idx < sentence_size; idx++)
    {
        gettimeofday(&tv1,NULL);
        if(pt0->service) {
            SOCKET_send(socketfd,
                        (char*)(pt0->input_state+idx*(pt0->ll_word_size+pt0->ll_caps_size+pt0->ll_posl_size)),
                        pt0->window_size*(pt0->ll_word_size+pt0->ll_caps_size+pt0->ll_posl_size)*sizeof(float),
                        pt0->debug
                        );
            SOCKET_receive(socketfd,
                           (char*)(pt0->output_state+idx*pt0->output_state_size),
                           pt0->output_state_size*sizeof(float),
                           pt0->debug
                           );
        } else {
            SENNA_nn_linear(pt0->hidden_state,
                            pt0->hidden_state_size,
                            pt0->l1_weight,
                            pt0->l1_bias,
                            pt0->input_state+idx*(pt0->ll_word_size+pt0->ll_caps_size+pt0->ll_posl_size),
                            pt0->window_size*(pt0->ll_word_size+pt0->ll_caps_size+pt0->ll_posl_size));
            SENNA_nn_hardtanh(pt0->hidden_state,
                              pt0->hidden_state,
                              pt0->hidden_state_size);
            SENNA_nn_linear(pt0->output_state+idx*pt0->output_state_size,
                            pt0->output_state_size,
                            pt0->l2_weight,
                            pt0->l2_bias,
                            pt0->hidden_state,
                            pt0->hidden_state_size);
        }
        gettimeofday(&tv2,NULL);
        pt0->dnntime += (tv2.tv_sec-tv1.tv_sec)*1000000 +
    (tv2.tv_usec-tv1.tv_usec);
        pt0->calls++;

        gettimeofday(&tv1,NULL);
        pt0->labels = SENNA_realloc(pt0->labels, sizeof(int), sentence_size);
        SENNA_nn_viterbi(pt0->labels, pt0->viterbi_score_init,
    pt0->viterbi_score_trans, pt0->output_state, pt0->output_state_size,
    sentence_size);
        gettimeofday(&tv2,NULL);
        pt0->apptime += (tv2.tv_sec-tv1.tv_sec)*1000000 +
    (tv2.tv_usec-tv1.tv_usec);
    }
    */
  gettimeofday(&tv2, NULL);
  pt0->dnntime +=
      (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);
  pt0->calls++;

  gettimeofday(&tv1, NULL);

  pt0->labels = SENNA_realloc(pt0->labels, sizeof(int), sentence_size);
  SENNA_nn_viterbi(pt0->labels, pt0->viterbi_score_init,
                   pt0->viterbi_score_trans, pt0->output_state,
                   pt0->output_state_size, sentence_size);
  gettimeofday(&tv2, NULL);
  pt0->apptime +=
      (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);

  return pt0->labels;
}

SENNA_PT0 *SENNA_PT0_new(const char *path, const char *subpath) {
  SENNA_PT0 *pt0 = SENNA_malloc(sizeof(SENNA_PT0), 1);
  FILE *f;
  float dummy;

  memset(pt0, 0, sizeof(SENNA_PT0));

  f = SENNA_fopen(path, subpath, "rb");

  SENNA_fread(&pt0->window_size, sizeof(int), 1, f);
  SENNA_fread_tensor_2d(&pt0->ll_word_weight, &pt0->ll_word_size,
                        &pt0->ll_word_max_idx, f);
  SENNA_fread_tensor_2d(&pt0->ll_caps_weight, &pt0->ll_caps_size,
                        &pt0->ll_caps_max_idx, f);
  SENNA_fread_tensor_2d(&pt0->ll_posl_weight, &pt0->ll_posl_size,
                        &pt0->ll_posl_max_idx, f);
  SENNA_fread_tensor_2d(&pt0->l1_weight, &pt0->input_state_size,
                        &pt0->hidden_state_size, f);
  SENNA_fread_tensor_1d(&pt0->l1_bias, &pt0->hidden_state_size, f);
  SENNA_fread_tensor_2d(&pt0->l2_weight, &pt0->hidden_state_size,
                        &pt0->output_state_size, f);
  SENNA_fread_tensor_1d(&pt0->l2_bias, &pt0->output_state_size, f);
  SENNA_fread_tensor_1d(&pt0->viterbi_score_init, &pt0->output_state_size, f);
  SENNA_fread_tensor_2d(&pt0->viterbi_score_trans, &pt0->output_state_size,
                        &pt0->output_state_size, f);

  SENNA_fread(&pt0->ll_word_padding_idx, sizeof(int), 1, f);
  SENNA_fread(&pt0->ll_caps_padding_idx, sizeof(int), 1, f);
  SENNA_fread(&pt0->ll_posl_padding_idx, sizeof(int), 1, f);

  SENNA_fread(&dummy, sizeof(float), 1, f);
  SENNA_fclose(f);

  if ((int)dummy != 777)
    SENNA_error("pt0: data corrupted (or not IEEE floating computer)");

  pt0->input_state = NULL;
  pt0->hidden_state = SENNA_malloc(sizeof(float), pt0->hidden_state_size);
  pt0->output_state = NULL;
  pt0->labels = NULL;

  /* some info if you want verbose */
  SENNA_message("pt0: window size: %d", pt0->window_size);
  SENNA_message("pt0: vector size in word lookup table: %d", pt0->ll_word_size);
  SENNA_message("pt0: word lookup table size: %d", pt0->ll_word_max_idx);
  SENNA_message("pt0: vector size in caps lookup table: %d", pt0->ll_caps_size);
  SENNA_message("pt0: caps lookup table size: %d", pt0->ll_caps_max_idx);
  SENNA_message("pt0: vector size in pos lookup table: %d", pt0->ll_posl_size);
  SENNA_message("pt0: pos lookup table size: %d", pt0->ll_posl_max_idx);
  SENNA_message("pt0: number of hidden units: %d", pt0->hidden_state_size);
  SENNA_message("pt0: number of classes: %d", pt0->output_state_size);

  pt0->service = false;
  pt0->debug = false;
  pt0->socketfd = -1;
  pt0->calls = 0;
  pt0->dnntime = 0;
  pt0->apptime = 0;

  return pt0;
}

void SENNA_PT0_free(SENNA_PT0 *pt0) {
  SENNA_free(pt0->ll_word_weight);
  SENNA_free(pt0->ll_caps_weight);
  SENNA_free(pt0->ll_posl_weight);
  SENNA_free(pt0->l1_weight);
  SENNA_free(pt0->l1_bias);
  SENNA_free(pt0->l2_weight);
  SENNA_free(pt0->l2_bias);
  SENNA_free(pt0->viterbi_score_init);
  SENNA_free(pt0->viterbi_score_trans);

  SENNA_free(pt0->input_state);
  SENNA_free(pt0->hidden_state);
  SENNA_free(pt0->output_state);
  SENNA_free(pt0->labels);

  SENNA_free(pt0);
}
