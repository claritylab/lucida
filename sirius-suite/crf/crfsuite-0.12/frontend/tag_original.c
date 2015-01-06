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

#include <os.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include <crfsuite.h>
#include "option.h"
#include "iwa.h"

#define    SAFE_RELEASE(obj)    if ((obj) != NULL) { (obj)->release(obj); (obj) = NULL; }

void show_copyright(FILE *fp);

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

static char* mystrdup(const char *src)
{
    char *dst = (char*)malloc(strlen(src)+1);
    if (dst != NULL) {
        strcpy(dst, src);
    }
    return dst;
}

static void tagger_option_init(tagger_option_t* opt)
{
    memset(opt, 0, sizeof(*opt));
    opt->fpi = stdin;
    opt->fpo = stdout;
    opt->fpe = stderr;
    opt->model = mystrdup("");
}

static void tagger_option_finish(tagger_option_t* opt)
{
    int i;

    free(opt->input);
    free(opt->model);
    for (i = 0;i < opt->num_params;++i) {
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
        opt->params = (char **)realloc(opt->params, sizeof(char*) * (opt->num_params + 1));
        opt->params[opt->num_params] = mystrdup(arg);
        ++opt->num_params;

END_OPTION_MAP()

static void show_usage(FILE *fp, const char *argv0, const char *command)
{
    fprintf(fp, "USAGE: %s %s [OPTIONS] [DATA]\n", argv0, command);
    fprintf(fp, "Assign suitable labels to the instances in the data set given by a file (DATA).\n");
    fprintf(fp, "If the argument DATA is omitted or '-', this utility reads a data from STDIN.\n");
    fprintf(fp, "Evaluate the performance of the model on labeled instances (with -t option).\n");
    fprintf(fp, "\n");
    fprintf(fp, "OPTIONS:\n");
    fprintf(fp, "    -m, --model=MODEL   Read a model from a file (MODEL)\n");
    fprintf(fp, "    -t, --test          Report the performance of the model on the data\n");
    fprintf(fp, "    -r, --reference     Output the reference labels in the input data\n");
    fprintf(fp, "    -p, --probability   Output the probability of the label sequences\n");
    fprintf(fp, "    -i, --marginal      Output the marginal probabilities of items\n");
    fprintf(fp, "    -q, --quiet         Suppress tagging results (useful for test mode)\n");
    fprintf(fp, "    -h, --help          Show the usage of this command and exit\n");
}



static void
output_result(
    FILE *fpo,
    crfsuite_tagger_t *tagger,
    const crfsuite_instance_t *inst,
    int *output,
    crfsuite_dictionary_t *labels,
    floatval_t score,
    const tagger_option_t* opt
    )
{
    int i;

    if (opt->probability) {
        floatval_t lognorm;
        tagger->lognorm(tagger, &lognorm);
        fprintf(fpo, "@probability\t%f\n", exp(score - lognorm));
    }

    for (i = 0;i < inst->num_items;++i) {
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

static void
output_instance(
    FILE *fpo,
    const crfsuite_instance_t *inst,
    crfsuite_dictionary_t *labels,
    crfsuite_dictionary_t *attrs
    )
{
    int i, j;

    for (i = 0;i < inst->num_items;++i) {
        const char *label = NULL;
        labels->to_string(labels, inst->labels[i], &label);
        fprintf(fpo, "%s", label);
        labels->free(labels, label);

        for (j = 0;j < inst->items[i].num_contents;++j) {
            const char *attr = NULL;
            attrs->to_string(attrs, inst->items[i].contents[j].aid, &attr);
            fprintf(fpo, "\t%s:%f", attr, inst->items[i].contents[j].value);
            attrs->free(attrs, attr);
        }

        fprintf(fpo, "\n");
    }
    fprintf(fpo, "\n");
}

static int message_callback(void *instance, const char *format, va_list args)
{
    FILE *fp = (FILE*)instance;
    vfprintf(fp, format, args);
    fflush(fp);
    return 0;
}

static int tag(tagger_option_t* opt, crfsuite_model_t* model)
{
    int N = 0, L = 0, ret = 0, lid = -1;
    clock_t clk0, clk1;
    crfsuite_instance_t inst;
    crfsuite_item_t item;
    crfsuite_attribute_t cont;
    crfsuite_evaluation_t eval;
    char *comment = NULL;
    iwa_t* iwa = NULL;
    const iwa_token_t* token = NULL;
    crfsuite_tagger_t *tagger = NULL;
    crfsuite_dictionary_t *attrs = NULL, *labels = NULL;
    FILE *fp = NULL, *fpi = opt->fpi, *fpo = opt->fpo, *fpe = opt->fpe;

    /* Obtain the dictionary interface representing the labels in the model. */
    if (ret = model->get_labels(model, &labels)) {
        goto force_exit;
    }

    /* Obtain the dictionary interface representing the attributes in the model. */
    if (ret = model->get_attrs(model, &attrs)) {
        goto force_exit;
    }

    /* Obtain the tagger interface. */
    if (ret = model->get_tagger(model, &tagger)) {
        goto force_exit;
    }

    /* Initialize the objects for instance and evaluation. */
    L = labels->num(labels);
    crfsuite_instance_init(&inst);
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
        fprintf(fpe, "ERROR: Failed to initialize the parser for the input data.\n");
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
            crfsuite_item_init(&item);
            free(comment);
            comment = NULL;
            break;
        case IWA_EOI:
            /* Append the item to the instance. */
            crfsuite_instance_append(&inst, &item, lid);
            crfsuite_item_finish(&item);
            break;
        case IWA_ITEM:
            if (lid == -1) {
                /* The first field in a line presents a label. */
                lid = labels->to_id(labels, token->attr);
                if (lid < 0) lid = L;    /* #L stands for a unknown label. */
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
                    crfsuite_item_append_attribute(&item, &cont);
                }
            }
            break;
        case IWA_NONE:
        case IWA_EOF:
            if (!crfsuite_instance_empty(&inst)) {
                /* Initialize the object to receive the tagging result. */
                floatval_t score = 0;
                int *output = calloc(sizeof(int), inst.num_items);

                /* Set the instance to the tagger. */
                if ((ret = tagger->set(tagger, &inst))) {
                    goto force_exit;
                }

                /* Obtain the viterbi label sequence. */
                if ((ret = tagger->viterbi(tagger, output, &score))) {
                    goto force_exit;
                }

                ++N;

                /* Accumulate the tagging performance. */
                if (opt->evaluate) {
                    crfsuite_evaluation_accmulate(&eval, inst.labels, output, inst.num_items);
                }

                if (!opt->quiet) {
                    output_result(fpo, tagger, &inst, output, labels, score, opt);
                }

                free(output);
                crfsuite_instance_finish(&inst);
            }
            break;
        }
    }
    clk1 = clock();

    /* Compute the performance if specified. */
    if (opt->evaluate) {
        double sec = (clk1 - clk0) / (double)CLOCKS_PER_SEC;
        crfsuite_evaluation_finalize(&eval);
        crfsuite_evaluation_output(&eval, labels, message_callback, stdout);
        fprintf(fpo, "Elapsed time: %f [sec] (%.1f [instance/sec])\n", sec, N / sec);
    }

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
    crfsuite_instance_finish(&inst);
    crfsuite_evaluation_finish(&eval);

    SAFE_RELEASE(tagger);
    SAFE_RELEASE(attrs);
    SAFE_RELEASE(labels);

    return ret;
}

int main_tag(int argc, char *argv[], const char *argv0)
{
    int ret = 0, arg_used = 0;
    tagger_option_t opt;
    const char *command = argv[0];
    FILE *fp = NULL, *fpi = stdin, *fpo = stdout, *fpe = stderr;
    crfsuite_model_t *model = NULL;

    /* Parse the command-line option. */
    tagger_option_init(&opt);
    arg_used = option_parse(++argv, --argc, parse_tagger_options, &opt);
    if (arg_used < 0) {
        ret = 1;
        goto force_exit;
    }

    /* Show the help message for this command if specified. */
    if (opt.help) {
        show_copyright(fpo);
        show_usage(fpo, argv0, command);
        goto force_exit;
    }

    /* Set an input file. */
    if (arg_used < argc) {
        opt.input = mystrdup(argv[arg_used]);
    } else {
        opt.input = mystrdup("-");    /* STDIN. */
    }

    /* Read the model. */
    if (opt.model != NULL) {
        /* Create a model instance corresponding to the model file. */
        if (ret = crfsuite_create_instance_from_file(opt.model, (void**)&model)) {
            goto force_exit;
        }

        /* Tag the input data. */
        if (ret = tag(&opt, model)) {
            goto force_exit;
        }
    }

force_exit:
    SAFE_RELEASE(model);
    tagger_option_finish(&opt);
    return ret;
}
