#include <sys/time.h>
#include "SENNA_VBS.h"
#include "SENNA_utils.h"
#include "SENNA_nn.h"
#include "socket.h"

int *SENNA_VBS_forward(SENNA_VBS *vbs, const int *sentence_words,
                       const int *sentence_caps, const int *sentence_posl,
                       int sentence_size, int socketfd) {
  int idx;
  struct timeval tv1, tv2;

  gettimeofday(&tv1, NULL);
  vbs->input_state = SENNA_realloc(
      vbs->input_state, sizeof(float),
      (sentence_size + vbs->window_size - 1) *
          (vbs->ll_word_size + vbs->ll_caps_size + vbs->ll_posl_size));
  vbs->output_state = SENNA_realloc(vbs->output_state, sizeof(float),
                                    sentence_size * vbs->output_state_size);

  SENNA_nn_lookup(vbs->input_state,
                  vbs->ll_word_size + vbs->ll_caps_size + vbs->ll_posl_size,
                  vbs->ll_word_weight, vbs->ll_word_size, vbs->ll_word_max_idx,
                  sentence_words, sentence_size, vbs->ll_word_padding_idx,
                  (vbs->window_size - 1) / 2);
  SENNA_nn_lookup(vbs->input_state + vbs->ll_word_size,
                  vbs->ll_word_size + vbs->ll_caps_size + vbs->ll_posl_size,
                  vbs->ll_caps_weight, vbs->ll_caps_size, vbs->ll_caps_max_idx,
                  sentence_caps, sentence_size, vbs->ll_caps_padding_idx,
                  (vbs->window_size - 1) / 2);
  SENNA_nn_lookup(vbs->input_state + vbs->ll_word_size + vbs->ll_caps_size,
                  vbs->ll_word_size + vbs->ll_caps_size + vbs->ll_posl_size,
                  vbs->ll_posl_weight, vbs->ll_posl_size, vbs->ll_posl_max_idx,
                  sentence_posl, sentence_size, vbs->ll_posl_padding_idx,
                  (vbs->window_size - 1) / 2);
  gettimeofday(&tv2, NULL);
  vbs->apptime +=
      (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);

  gettimeofday(&tv1, NULL);

  int input_size = vbs->ll_word_size + vbs->ll_caps_size + vbs->ll_posl_size;

  char *input_data = (char *)malloc(
      sentence_size * (vbs->window_size * input_size) * sizeof(float));

  for (idx = 0; idx < sentence_size; idx++) {
    memcpy((char *)(input_data +
                    idx * (vbs->window_size) * (input_size) * sizeof(float)),
           (char *)(vbs->input_state + idx * input_size),
           vbs->window_size * input_size * sizeof(float));
  }

  if (vbs->service) {
    SOCKET_send(socketfd, input_data,
                sentence_size * (vbs->window_size * input_size * sizeof(float)),
                vbs->debug);
    SOCKET_receive(socketfd, (char *)(vbs->output_state),
                   vbs->output_state_size * sizeof(float) * sentence_size,
                   vbs->debug);
  }

  /*
    for(idx = 0; idx < sentence_size; idx++)
    {
        if(vbs->service) {
          SOCKET_send(socketfd,
                        (char*)(vbs->input_state+idx*(vbs->ll_word_size+vbs->ll_caps_size+vbs->ll_posl_size)),
                        vbs->window_size*(vbs->ll_word_size+vbs->ll_caps_size+vbs->ll_posl_size)*sizeof(float),
                        vbs->debug
                        );
          SOCKET_receive(socketfd,
                         (char*)(vbs->output_state+idx*vbs->output_state_size),
                         vbs->output_state_size*sizeof(float),
                         vbs->debug
                         );
        }
        else{
        SENNA_nn_linear(vbs->hidden_state,
                        vbs->hidden_state_size,
                        vbs->l1_weight,
                        vbs->l1_bias,
                        vbs->input_state+idx*(vbs->ll_word_size+vbs->ll_caps_size+vbs->ll_posl_size),
                        vbs->window_size*(vbs->ll_word_size+vbs->ll_caps_size+vbs->ll_posl_size));
        SENNA_nn_hardtanh(vbs->hidden_state,
                          vbs->hidden_state,
                          vbs->hidden_state_size);
        SENNA_nn_linear(vbs->output_state+idx*vbs->output_state_size,
                        vbs->output_state_size,
                        vbs->l2_weight,
                        vbs->l2_bias,
                        vbs->hidden_state,
                        vbs->hidden_state_size);
        }
        vbs->calls++;
    }
    */
  gettimeofday(&tv2, NULL);
  vbs->dnntime +=
      (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);

  gettimeofday(&tv1, NULL);
  vbs->labels = SENNA_realloc(vbs->labels, sizeof(int), sentence_size);
  for (idx = 0; idx < sentence_size; idx++)
    SENNA_nn_max(NULL, &vbs->labels[idx],
                 vbs->output_state + idx * vbs->output_state_size,
                 vbs->output_state_size);
  gettimeofday(&tv2, NULL);
  vbs->apptime +=
      (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);

  return vbs->labels;
}

SENNA_VBS *SENNA_VBS_new(const char *path, const char *subpath) {
  SENNA_VBS *vbs = SENNA_malloc(sizeof(SENNA_VBS), 1);
  FILE *f;
  float dummy;

  f = SENNA_fopen(path, subpath, "rb");

  SENNA_fread(&vbs->window_size, sizeof(int), 1, f);
  SENNA_fread_tensor_2d(&vbs->ll_word_weight, &vbs->ll_word_size,
                        &vbs->ll_word_max_idx, f);
  SENNA_fread_tensor_2d(&vbs->ll_caps_weight, &vbs->ll_caps_size,
                        &vbs->ll_caps_max_idx, f);
  SENNA_fread_tensor_2d(&vbs->ll_posl_weight, &vbs->ll_posl_size,
                        &vbs->ll_posl_max_idx, f);
  SENNA_fread_tensor_2d(&vbs->l1_weight, &vbs->input_state_size,
                        &vbs->hidden_state_size, f);
  SENNA_fread_tensor_1d(&vbs->l1_bias, &vbs->hidden_state_size, f);
  SENNA_fread_tensor_2d(&vbs->l2_weight, &vbs->hidden_state_size,
                        &vbs->output_state_size, f);
  SENNA_fread_tensor_1d(&vbs->l2_bias, &vbs->output_state_size, f);

  SENNA_fread(&vbs->ll_word_padding_idx, sizeof(int), 1, f);
  SENNA_fread(&vbs->ll_caps_padding_idx, sizeof(int), 1, f);
  SENNA_fread(&vbs->ll_posl_padding_idx, sizeof(int), 1, f);

  SENNA_fread(&dummy, sizeof(float), 1, f);
  SENNA_fclose(f);

  if ((int)dummy != 777)
    SENNA_error("vbs: data corrupted (or not IEEE floating computer)");

  vbs->input_state = NULL;
  vbs->hidden_state = SENNA_malloc(sizeof(float), vbs->hidden_state_size);
  vbs->output_state = NULL;
  vbs->labels = NULL;

  /* some info if you want verbose */
  SENNA_message("vbs: window size: %d", vbs->window_size);
  SENNA_message("vbs: vector size in word lookup table: %d", vbs->ll_word_size);
  SENNA_message("vbs: word lookup table size: %d", vbs->ll_word_max_idx);
  SENNA_message("vbs: vector size in caps lookup table: %d", vbs->ll_caps_size);
  SENNA_message("vbs: caps lookup table size: %d", vbs->ll_caps_max_idx);
  SENNA_message("vbs: vector size in pos lookup table: %d", vbs->ll_posl_size);
  SENNA_message("vbs: pos lookup table size: %d", vbs->ll_posl_max_idx);
  SENNA_message("vbs: number of hidden units: %d", vbs->hidden_state_size);
  SENNA_message("vbs: number of classes: %d", vbs->output_state_size);

  vbs->service = false;
  vbs->debug = false;
  vbs->socketfd = -1;
  vbs->calls = 0;
  vbs->dnntime = 0;
  vbs->apptime = 0;

  return vbs;
}

void SENNA_VBS_free(SENNA_VBS *vbs) {
  SENNA_free(vbs->ll_word_weight);
  SENNA_free(vbs->ll_caps_weight);
  SENNA_free(vbs->ll_posl_weight);
  SENNA_free(vbs->l1_weight);
  SENNA_free(vbs->l1_bias);
  SENNA_free(vbs->l2_weight);
  SENNA_free(vbs->l2_bias);

  SENNA_free(vbs->input_state);
  SENNA_free(vbs->hidden_state);
  SENNA_free(vbs->output_state);
  SENNA_free(vbs->labels);

  SENNA_free(vbs);
}
