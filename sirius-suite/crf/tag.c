/*
 *        Tag command for CRFsuite frontend.
 *
 * Copyright (c) 2007-2010, Naoaki Okazaki
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the names of the authors nor the names of its contributors
 *       may be used to endorse or promote products derived from this
 *       software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* $Id$ */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include <crfsuite.h>
#include "option.h"
#include "iwa.h"

#include "../lib/crf/src/crf1d.h"

#include <float.h>
#include <sys/time.h>
#include <pthread.h>

#define NTHREADS 8

#define SAFE_RELEASE(obj) \
  if ((obj) != NULL) {    \
    (obj)->release(obj);  \
    (obj) = NULL;         \
  }

void show_copyright(FILE *fp);

crfsuite_tagger_t *tagger;
crfsuite_instance_t **inst_vect;
int N = 0;

int **global_out;

enum {
  LEVEL_NONE = 0,
  LEVEL_SET,
  LEVEL_ALPHABETA,
};

typedef struct {
  crf1dm_t *model;      /**< CRF model. */
  crf1d_context_t *ctx; /**< CRF context. */
  int num_labels;       /**< Number of distinct output labels (L). */
  int num_attributes;   /**< Number of distinct attributes (A). */
  int level;
} crf1dt_t;

/*
typedef struct {
  int *array;
  size_t used;
  size_t size;
} Array;

void initArray(Array *a, size_t initialSize) {
  a->array = (int *)malloc(initialSize * sizeof(int));
  a->used = 0;
  a->size = initialSize;
}

void insertArray(Array *a, int element) {
  if (a->used == a->size) {
    a->size *= 2;
    a->array = (int *)realloc(a->array, a->size * sizeof(int));
  }
  a->array[a->used++] = element;
}

void freeArray(Array *a) {
  free(a->array);
  a->array = NULL;
  a->used = a->size = 0;
}
*/

typedef struct {
  char *input;
  char *model;
  int evaluate;
  int probability;
  int marginal;
  int quiet;
  int reference;
  int help;

  int num_params;
  char **params;

  FILE *fpi;
  FILE *fpo;
  FILE *fpe;
} tagger_option_t;

static char *mystrdup(const char *src) {
  char *dst = (char *)malloc(strlen(src) + 1);
  if (dst != NULL) {
    strcpy(dst, src);
  }
  return dst;
}

static void tagger_option_init(tagger_option_t *opt) {
  memset(opt, 0, sizeof(*opt));
  opt->fpi = stdin;
  opt->fpo = stdout;
  opt->fpe = stderr;
  opt->model = mystrdup("");
}

static void tagger_option_finish(tagger_option_t *opt) {
  int i;

  free(opt->input);
  free(opt->model);
  for (i = 0; i < opt->num_params; ++i) {
    free(opt->params[i]);
  }
  free(opt->params);
}

BEGIN_OPTION_MAP(parse_tagger_options, tagger_option_t)

ON_OPTION_WITH_ARG(SHORTOPT('m') || LONGOPT("model"))
free(opt->model);
opt->model = mystrdup(arg);

ON_OPTION(SHORTOPT('t') || LONGOPT("test"))
opt->evaluate = 1;

ON_OPTION(SHORTOPT('r') || LONGOPT("reference"))
opt->reference = 1;

ON_OPTION(SHORTOPT('p') || LONGOPT("probability"))
opt->probability = 1;

ON_OPTION(SHORTOPT('i') || LONGOPT("marginal"))
opt->marginal = 1;

ON_OPTION(SHORTOPT('q') || LONGOPT("quiet"))
opt->quiet = 1;

ON_OPTION(SHORTOPT('h') || LONGOPT("help"))
opt->help = 1;

ON_OPTION_WITH_ARG(SHORTOPT('p') || LONGOPT("param"))
opt->params =
    (char **)realloc(opt->params, sizeof(char *) * (opt->num_params + 1));
opt->params[opt->num_params] = mystrdup(arg);
++opt->num_params;

END_OPTION_MAP()

static void show_usage(FILE *fp, const char *argv0, const char *command) {
  fprintf(fp, "USAGE: %s %s [OPTIONS] [DATA]\n", argv0, command);
  fprintf(fp,
          "Assign suitable labels to the instances in the data set given "
          "by a file (DATA).\n");
  fprintf(fp,
          "If the argument DATA is omitted or '-', this utility reads a "
          "data from STDIN.\n");
  fprintf(fp,
          "Evaluate the performance of the model on labeled instances "
          "(with -t option).\n");
  fprintf(fp, "\n");
  fprintf(fp, "OPTIONS:\n");
  fprintf(fp,
          "    -m, --model=MODEL   Read a model from a file (MODEL)\n");
  fprintf(fp,
          "    -t, --test          Report the performance of the model on "
          "the data\n");
  fprintf(fp,
          "    -r, --reference     Output the reference labels in the "
          "input data\n");
  fprintf(fp,
          "    -p, --probability   Output the probability of the label "
          "sequences\n");
  fprintf(fp,
          "    -i, --marginal      Output the marginal probabilities of "
          "items\n");
  fprintf(fp,
          "    -q, --quiet         Suppress tagging results (useful for "
          "test mode)\n");
  fprintf(fp,
          "    -h, --help          Show the usage of this command and exit\n");
}

static void output_result(FILE *fpo, crfsuite_tagger_t *tagger,
                          const crfsuite_instance_t *inst, int *output,
                          crfsuite_dictionary_t *labels, floatval_t score,
                          const tagger_option_t *opt) {
  int i;

  if (opt->probability) {
    floatval_t lognorm;
    tagger->lognorm(tagger, &lognorm);
    fprintf(fpo, "@probability\t%f\n", exp(score - lognorm));
  }

  for (i = 0; i < inst->num_items; ++i) {
    const char *label = NULL;

    if (opt->reference) {
      labels->to_string(labels, inst->labels[i], &label);
      fprintf(fpo, "%s\t", label);
      labels->free(labels, label);
    }

    labels->to_string(labels, output[i], &label);
    fprintf(fpo, "%s", label);
    labels->free(labels, label);

    if (opt->marginal) {
      floatval_t prob;
      tagger->marginal_point(tagger, output[i], i, &prob);
      fprintf(fpo, ":%f", prob);
    }

    fprintf(fpo, "\n");
  }
  fprintf(fpo, "\n");
}

static void output_instance(FILE *fpo, const crfsuite_instance_t *inst,
                            crfsuite_dictionary_t *labels,
                            crfsuite_dictionary_t *attrs) {
  int i, j;

  for (i = 0; i < inst->num_items; ++i) {
    const char *label = NULL;
    labels->to_string(labels, inst->labels[i], &label);
    fprintf(fpo, "%s", label);
    labels->free(labels, label);

    for (j = 0; j < inst->items[i].num_contents; ++j) {
      const char *attr = NULL;
      attrs->to_string(attrs, inst->items[i].contents[j].aid, &attr);
      fprintf(fpo, "\t%s:%f", attr, inst->items[i].contents[j].value);
      attrs->free(attrs, attr);
    }

    fprintf(fpo, "\n");
  }
  fprintf(fpo, "\n");
}

static int message_callback(void *instance, const char *format, va_list args) {
  FILE *fp = (FILE *)instance;
  vfprintf(fp, format, args);
  fflush(fp);
  return 0;
}

static int tag_read(tagger_option_t *opt, crfsuite_model_t *model) {
  int L = 0, ret = 0, lid = -1;
  clock_t clk0, clk1;
  crfsuite_item_t *item;
  crfsuite_attribute_t cont;
  crfsuite_evaluation_t eval;
  char *comment = NULL;
  iwa_t *iwa = NULL;
  const iwa_token_t *token = NULL;
  crfsuite_tagger_t *tagger = NULL;
  crfsuite_dictionary_t *attrs = NULL, *labels = NULL;
  FILE *fp = NULL, *fpi = opt->fpi, *fpo = opt->fpo, *fpe = opt->fpe;
  int cur_inst_size = 10000;
  int k;
  int M;

  /* Obtain the dictionary interface representing the labels in the model. */
  if (ret = model->get_labels(model, &labels)) {
    goto force_exit;
  }

  /* Obtain the dictionary interface representing the attributes in the model.
   */
  if (ret = model->get_attrs(model, &attrs)) {
    goto force_exit;
  }

  /* Obtain the tagger interface. */
  if (ret = model->get_tagger(model, &tagger)) {
    goto force_exit;
  }

  inst_vect = (crfsuite_instance_t **)malloc(cur_inst_size *
                                             sizeof(crfsuite_instance_t *));
  inst_vect[N] = (crfsuite_instance_t *)malloc(sizeof(crfsuite_instance_t));

  /* Initialize the objects for instance and evaluation. */
  L = labels->num(labels);
  crfsuite_instance_init(inst_vect[N]);
  crfsuite_evaluation_init(&eval, L);

  /* Open the stream for the input data. */
  fp = (strcmp(opt->input, "-") == 0) ? fpi : fopen(opt->input, "r");
  if (fp == NULL) {
    fprintf(fpe, "ERROR: failed to open the stream for the input data,\n");
    fprintf(fpe, "  %s\n", opt->input);
    ret = 1;
    goto force_exit;
  }

  /* Open a IWA reader. */
  iwa = iwa_reader(fp);
  if (iwa == NULL) {
    fprintf(fpe,
            "ERROR: Failed to initialize the parser for the input data.\n");
    ret = 1;
    goto force_exit;
  }

  /* Read the input data and assign labels. */
  clk0 = clock();
  while (token = iwa_read(iwa), token != NULL) {
    switch (token->type) {
      case IWA_BOI:
        /* Initialize an item. */
        lid = -1;
        item = (crfsuite_item_t *)malloc(sizeof(crfsuite_item_t));
        crfsuite_item_init(item);
        free(comment);
        comment = NULL;
        break;
      case IWA_EOI:
        /* Append the item to the instance. */
        crfsuite_instance_append(inst_vect[N], item, lid);
        // crfsuite_item_finish(item_vect[N]);
        break;
      case IWA_ITEM:
        if (lid == -1) {
          /* The first field in a line presents a label. */
          lid = labels->to_id(labels, token->attr);
          if (lid < 0) lid = L; /* #L stands for a unknown label. */
        } else {
          /* Fields after the first field present attributes. */
          int aid = attrs->to_id(attrs, token->attr);
          /* Ignore attributes 'unknown' to the model. */
          if (0 <= aid) {
            /* Associate the attribute with the current item. */
            if (token->value && *token->value) {
              crfsuite_attribute_set(&cont, aid, atof(token->value));
            } else {
              crfsuite_attribute_set(&cont, aid, 1.0);
            }
            crfsuite_item_append_attribute(item, &cont);
          }
        }
        break;
      case IWA_NONE:
      case IWA_EOF:

        if (!crfsuite_instance_empty(inst_vect[N])) {
          N++;
          inst_vect[N] =
              (crfsuite_instance_t *)malloc(sizeof(crfsuite_instance_t));
          crfsuite_instance_init(inst_vect[N]);
        }
    }
  }
  clk1 = clock();

force_exit:
  /* Close the IWA parser. */
  iwa_delete(iwa);
  iwa = NULL;

  /* Close the input stream if necessary. */
  if (fp != NULL && fp != fpi) {
    fclose(fp);
    fp = NULL;
  }

  free(comment);

  return ret;
}

float calculateMiliseconds(struct timeval t1, struct timeval t2) {
  float elapsedTime;
  elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;
  elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;
  return elapsedTime;
}

void *tag_thread(void *tid) {
  int k, start, *mytid, end;

  int iterations = N / NTHREADS;

  mytid = (int *)tid;
  start = (*mytid * iterations);
  end = start + iterations;

  for (k = start; k < end; k++) {
    floatval_t score = 0;
    // Set the instance to the tagger.
    tagger->set(tagger, inst_vect[k]);
    // Obtain the viterbi label sequence.
    tagger->viterbi(tagger, global_out[k], &score);
  }
}

static floatval_t viterbi(crf1d_context_t *ctx, int *labels) {
  int i, j, t;
  int *back = NULL;
  floatval_t max_score, score, *cur = NULL;
  const floatval_t *prev = NULL, *state = NULL, *trans = NULL;
  const int T = ctx->num_items;
  const int L = ctx->num_labels;

  /*
      This function assumes state and trans scores to be in the logarithm
     domain.
   */

  /* Compute the scores at (0, *). */
  cur = ALPHA_SCORE(ctx, 0);
  state = STATE_SCORE(ctx, 0);
  for (j = 0; j < L; ++j) {
    cur[j] = state[j];
  }

  /* Compute the scores at (t, *). */
  for (t = 1; t < T; ++t) {
    prev = ALPHA_SCORE(ctx, t - 1);
    cur = ALPHA_SCORE(ctx, t);
    state = STATE_SCORE(ctx, t);
    back = BACKWARD_EDGE_AT(ctx, t);

    /* Compute the score of (t, j). */
    for (j = 0; j < L; ++j) {
      max_score = -FLOAT_MAX;

      for (i = 0; i < L; ++i) {
        /* Transit from (t-1, i) to (t, j). */
        trans = TRANS_SCORE(ctx, i);
        score = prev[i] + trans[j];

        /* Store this path if it has the maximum score. */
        if (max_score < score) {
          max_score = score;
          /* Backward link (#t, #j) -> (#t-1, #i). */
          back[j] = i;
        }
      }
      /* Add the state score on (t, j). */
      cur[j] = max_score + state[j];
    }
  }

  /* Find the node (#T, #i) that reaches EOS with the maximum score. */
  max_score = -FLOAT_MAX;
  prev = ALPHA_SCORE(ctx, T - 1);
  for (i = 0; i < L; ++i) {
    if (max_score < prev[i]) {
      max_score = prev[i];
      labels[T - 1] = i; /* Tag the item #T. */
    }
  }

  /* Tag labels by tracing the backward links. */
  for (t = T - 2; 0 <= t; --t) {
    back = BACKWARD_EDGE_AT(ctx, t + 1);
    labels[t] = back[labels[t + 1]];
  }

  /* Return the maximum score (without the normalization factor subtracted). */
  return max_score;
}

#define CHUNK_SIZE 12

int read_uint32(uint8_t *buffer, uint32_t *value) {
  *value = ((uint32_t)buffer[0]);
  *value |= ((uint32_t)buffer[1] << 8);
  *value |= ((uint32_t)buffer[2] << 16);
  *value |= ((uint32_t)buffer[3] << 24);
  return sizeof(*value);
}

void crf1dt_state_score(crf1dt_t *crf1dt, crfsuite_instance_t *inst) {
  int a, i, l, t, r, fid;
  crf1dm_feature_t f;
  feature_refs_t attr;
  floatval_t value, *state = NULL;
  crf1dm_t *model = crf1dt->model;
  crf1d_context_t *ctx = crf1dt->ctx;
  const crfsuite_item_t *item = NULL;
  const int T = inst->num_items;
  const int L = crf1dt->num_labels;

  // printf("size of T = %d\n", T);

  /* Loop over the items in the sequence. */
  for (t = 0; t < T; ++t) {
    item = &inst->items[t];
    state = STATE_SCORE(ctx, t);

    // printf("size of item contents = %d\n", item->num_contents);

    /* Loop over the contents (attributes) attached to the item. */
    for (i = 0; i < item->num_contents; ++i) {
      /* Access the list of state features associated with the attribute. */
      a = item->contents[i].aid;
      crf1dm_get_attrref(model, a, &attr);
      /* A scale usually represents the atrribute frequency in the item. */
      value = item->contents[i].value;

      /* Loop over the state features associated with the attribute. */
      for (r = 0; r < attr.num_features; ++r) {
        /* The state feature #(attr->fids[r]), which is represented by
           the attribute #a, outputs the label #(f->dst). */
        fid = crf1dm_get_featureid(&attr, r);
        crf1dm_get_feature(model, fid, &f);
        l = f.dst;
        state[l] += f.weight * value;
      }
    }
  }
}

#define FEATURE_SIZE 20

static int tagger_set(crfsuite_tagger_t *tagger, crfsuite_instance_t *inst) {
  crf1dt_t *crf1dt = (crf1dt_t *)tagger->internal;
  crf1d_context_t *ctx = crf1dt->ctx;
  crf1dc_set_num_items(ctx, inst->num_items);
  crf1dc_reset(crf1dt->ctx, RF_STATE);

  crf1dt_state_score(crf1dt, inst);

  crf1dt->level = LEVEL_SET;
  return 0;
}

int main_tag(int argc, char *argv[], const char *argv0) {
  int ret = 0, arg_used = 0;
  tagger_option_t opt;
  const char *command = argv[0];
  FILE *fp = NULL, *fpi = stdin, *fpo = stdout, *fpe = stderr;
  crfsuite_model_t *model = NULL;
  crfsuite_data_t *input_data;
  int n, k;

  struct timeval t1, t2;
  float cuda_elapsedTime;
  float cpu_elapsedTime;
  float par_elapsedTime;

  int i, start, tids[NTHREADS];
  pthread_t threads[NTHREADS];
  pthread_attr_t attr;

  /* Parse the command-line option. */
  tagger_option_init(&opt);
  arg_used = option_parse(++argv, --argc, parse_tagger_options, &opt);
  if (arg_used < 0) {
    ret = 1;
  }

  /* Show the help message for this command if specified. */
  if (opt.help) {
    show_copyright(fpo);
    show_usage(fpo, argv0, command);
  }

  /* Set an input file. */
  if (arg_used < argc) {
    opt.input = mystrdup(argv[arg_used]);
  } else {
    opt.input = mystrdup("-"); /* STDIN. */
  }

  /* Read the model. */
  if (opt.model != NULL) {
    /* Create a model instance corresponding to the model file. */
    if (ret = crfsuite_create_instance_from_file(opt.model, (void **)&model)) {
    }

    /* Open the stream for the input data. */
    fpi = (strcmp(opt.input, "-") == 0) ? fpi : fopen(opt.input, "r");
    if (fpi == NULL) {
      fprintf(fpo, "ERROR: failed to open the stream for the input data,\n");
      fprintf(fpo, "  %s\n", opt.input);
      ret = 1;
    }

    /* read the input data. */
    tag_read(&opt, model);

    /* Obtain the tagger interface. */
    if (ret = model->get_tagger(model, &tagger)) {
      //   goto force_exit;
    }

    // SEQ ALGO

    printf("Threads=%d Array size = %d\n", NTHREADS, N);

    gettimeofday(&t1, NULL);

    global_out = (int **)calloc(sizeof(int *), N);

    for (k = 0; k < N; k++) {
      floatval_t score = 0;
      int *output1 = (int *)calloc(sizeof(int), inst_vect[k]->num_items);
      global_out[k] = (int *)calloc(sizeof(int), inst_vect[k]->num_items);

      // Set the instance to the tagger.
      tagger_set(tagger, inst_vect[k]);
      // Obtain the viterbi label sequence.
      tagger->viterbi(tagger, output1, &score);

      free(output1);
    }

    gettimeofday(&t2, NULL);

    cpu_elapsedTime = calculateMiliseconds(t1, t2);

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    for (i = 0; i < NTHREADS; i++) {
      tids[i] = i;
      pthread_create(&threads[i], &attr, tag_thread, (void *)&tids[i]);
    }

    gettimeofday(&t1, NULL);

    for (i = 0; i < NTHREADS; i++) {
      pthread_join(threads[i], NULL);
    }

    gettimeofday(&t2, NULL);

    par_elapsedTime = calculateMiliseconds(t1, t2);
    printf("CRF Viterbi Parallel Time=%4.3f ms\n", par_elapsedTime);
    printf("CRF Viterbi CPU Time=%4.3f ms\n", cpu_elapsedTime);
    printf("Speedup=%4.3f\n", (float)cpu_elapsedTime / (float)par_elapsedTime);
  }

  return ret;
}
