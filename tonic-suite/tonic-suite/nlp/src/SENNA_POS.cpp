#include <sys/time.h>
#include <unistd.h>
#include "SENNA_POS.h"
#include "SENNA_utils.h"
#include "SENNA_nn.h"
#include "socket.h"

extern bool debug;

int *SENNA_POS_forward(SENNA_POS *pos, const int *sentence_words,
                       const int *sentence_caps, const int *sentence_suff,
                       TonicSuiteApp app) {
  pos->input_state = SENNA_realloc(
      pos->input_state, sizeof(float),
      (app.pl.num + pos->window_size - 1) *
          (pos->ll_word_size + pos->ll_caps_size + pos->ll_suff_size));
  pos->output_state = SENNA_realloc(pos->output_state, sizeof(float),
                                    app.pl.num * pos->output_state_size);

  SENNA_nn_lookup(pos->input_state,
                  pos->ll_word_size + pos->ll_caps_size + pos->ll_suff_size,
                  pos->ll_word_weight, pos->ll_word_size, pos->ll_word_max_idx,
                  sentence_words, app.pl.num, pos->ll_word_padding_idx,
                  (pos->window_size - 1) / 2);
  SENNA_nn_lookup(pos->input_state + pos->ll_word_size,
                  pos->ll_word_size + pos->ll_caps_size + pos->ll_suff_size,
                  pos->ll_caps_weight, pos->ll_caps_size, pos->ll_caps_max_idx,
                  sentence_caps, app.pl.num, pos->ll_caps_padding_idx,
                  (pos->window_size - 1) / 2);
  SENNA_nn_lookup(pos->input_state + pos->ll_word_size + pos->ll_caps_size,
                  pos->ll_word_size + pos->ll_caps_size + pos->ll_suff_size,
                  pos->ll_suff_weight, pos->ll_suff_size, pos->ll_suff_max_idx,
                  sentence_suff, app.pl.num, pos->ll_suff_padding_idx,
                  (pos->window_size - 1) / 2);

  app.pl.data = (char *)malloc(
      app.pl.num * (pos->window_size * (pos->ll_word_size + pos->ll_caps_size +
                                        pos->ll_suff_size)) *
      sizeof(float));

  for (int idx = 0; idx < app.pl.num; idx++) {
    memcpy((char *)(app.pl.data +
                    idx * (pos->window_size) *
                        (pos->ll_word_size + pos->ll_caps_size +
                         pos->ll_suff_size) *
                        sizeof(float)),
           (char *)(pos->input_state +
                    idx * (pos->ll_word_size + pos->ll_caps_size +
                           pos->ll_suff_size)),
           pos->window_size *
               (pos->ll_word_size + pos->ll_caps_size + pos->ll_suff_size) *
               sizeof(float));
  }

  if (app.djinn) {
    SOCKET_send(app.socketfd, (char *)app.pl.data,
                app.pl.num * app.pl.size * sizeof(float), debug);

    SOCKET_receive(app.socketfd, (char *)(pos->output_state),
                   app.pl.num * (pos->output_state_size) * sizeof(float),
                   debug);
  } else {
    float loss;
    vector<Blob<float> *> in_blobs = app.net->input_blobs();
    in_blobs[0]->set_cpu_data((float *)app.pl.data);
    vector<Blob<float> *> out_blobs = app.net->ForwardPrefilled(&loss);
    memcpy((pos->output_state), out_blobs[0]->cpu_data(),
           app.pl.num * (pos->output_state_size) * sizeof(float));
  }

  pos->labels = SENNA_realloc(pos->labels, sizeof(int), app.pl.num);

  SENNA_nn_viterbi(pos->labels, pos->viterbi_score_init,
                   pos->viterbi_score_trans, pos->output_state,
                   pos->output_state_size, app.pl.num);

  return pos->labels;
}

SENNA_POS *SENNA_POS_new(const char *path, const char *subpath) {
  SENNA_POS *pos = SENNA_malloc(sizeof(SENNA_POS), 1);
  FILE *f;
  float dummy;

  memset(pos, 0, sizeof(SENNA_POS));

  f = SENNA_fopen(path, subpath, "rb");

  SENNA_fread(&pos->window_size, sizeof(int), 1, f);
  SENNA_fread_tensor_2d(&pos->ll_word_weight, &pos->ll_word_size,
                        &pos->ll_word_max_idx, f);
  SENNA_fread_tensor_2d(&pos->ll_caps_weight, &pos->ll_caps_size,
                        &pos->ll_caps_max_idx, f);
  SENNA_fread_tensor_2d(&pos->ll_suff_weight, &pos->ll_suff_size,
                        &pos->ll_suff_max_idx, f);
  SENNA_fread_tensor_2d(&pos->l1_weight, &pos->input_state_size,
                        &pos->hidden_state_size, f);
  SENNA_fread_tensor_1d(&pos->l1_bias, &pos->hidden_state_size, f);
  SENNA_fread_tensor_2d(&pos->l2_weight, &pos->hidden_state_size,
                        &pos->output_state_size, f);
  SENNA_fread_tensor_1d(&pos->l2_bias, &pos->output_state_size, f);
  SENNA_fread_tensor_1d(&pos->viterbi_score_init, &pos->output_state_size, f);
  SENNA_fread_tensor_2d(&pos->viterbi_score_trans, &pos->output_state_size,
                        &pos->output_state_size, f);

  SENNA_fread(&pos->ll_word_padding_idx, sizeof(int), 1, f);
  SENNA_fread(&pos->ll_caps_padding_idx, sizeof(int), 1, f);
  SENNA_fread(&pos->ll_suff_padding_idx, sizeof(int), 1, f);

  SENNA_fread(&dummy, sizeof(float), 1, f);
  SENNA_fclose(f);

  if ((int)dummy != 777)
    SENNA_error("pos: data corrupted (or not IEEE floating computer)");

  pos->input_state = NULL;
  pos->hidden_state = SENNA_malloc(sizeof(float), pos->hidden_state_size);
  pos->output_state = NULL;
  pos->labels = NULL;

  /* some info if you want verbose */
  SENNA_message("pos: window size: %d", pos->window_size);
  SENNA_message("pos: vector size in word lookup table: %d", pos->ll_word_size);
  SENNA_message("pos: word lookup table size: %d", pos->ll_word_max_idx);
  SENNA_message("pos: vector size in caps lookup table: %d", pos->ll_caps_size);
  SENNA_message("pos: caps lookup table size: %d", pos->ll_caps_max_idx);
  SENNA_message("pos: vector size in suffix lookup table: %d",
                pos->ll_suff_size);
  SENNA_message("pos: suffix lookup table size: %d", pos->ll_suff_max_idx);
  SENNA_message("pos: number of hidden units: %d", pos->hidden_state_size);
  SENNA_message("pos: number of classes: %d", pos->output_state_size);

  return pos;
}

void SENNA_POS_free(SENNA_POS *pos) {
  SENNA_free(pos->ll_word_weight);
  SENNA_free(pos->ll_caps_weight);
  SENNA_free(pos->ll_suff_weight);
  SENNA_free(pos->l1_weight);
  SENNA_free(pos->l1_bias);
  SENNA_free(pos->l2_weight);
  SENNA_free(pos->l2_bias);
  SENNA_free(pos->viterbi_score_init);
  SENNA_free(pos->viterbi_score_trans);

  SENNA_free(pos->input_state);
  SENNA_free(pos->hidden_state);
  SENNA_free(pos->output_state);
  SENNA_free(pos->labels);

  SENNA_free(pos);
}
