/*
 *        Learn command for CRFsuite frontend.
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

#include <crfsuite.h>
#include "option.h"
#include "readdata.h"

#define    SAFE_RELEASE(obj)    if ((obj) != NULL) { (obj)->release(obj); (obj) = NULL; }
#define    MAX(a, b)    ((a) < (b) ? (b) : (a))


typedef struct {
    char *type;
    char *algorithm;
    char *model;
    char *logbase;

    int split;
    int cross_validation;
    int holdout;
    int logfile;

    int help;
    int help_params;

    int num_params;
    char **params;
} learn_option_t;

static char* mystrdup(const char *src)
{
    char *dst = (char*)malloc(strlen(src)+1);
    if (dst != NULL) {
        strcpy(dst, src);
    }
    return dst;
}

static char* mystrcat(char *dst, const char *src)
{
    int n = (dst != 0 ? strlen(dst) : 0);
    dst = (char*)realloc(dst, n + strlen(src) + 1);
    strcat(dst, src);
    return dst;
}

static void learn_option_init(learn_option_t* opt)
{
    memset(opt, 0, sizeof(*opt));
    opt->num_params = 0;
    opt->holdout = -1;
    opt->type = mystrdup("crf1d");
    opt->algorithm = mystrdup("lbfgs");
    opt->model = mystrdup("");
    opt->logbase = mystrdup("log.crfsuite");
}

static void learn_option_finish(learn_option_t* opt)
{
    int i;

    free(opt->model);

    for (i = 0;i < opt->num_params;++i) {
        free(opt->params[i]);
    }
    free(opt->params);
}

BEGIN_OPTION_MAP(parse_learn_options, learn_option_t)

    ON_OPTION_WITH_ARG(SHORTOPT('t') || LONGOPT("type"))
        if (strcmp(arg, "1d") == 0) {
            free(opt->type);
            opt->type = mystrdup("crf1d");
        } else {
            fprintf(stderr, "ERROR: Unknown graphical model: %s\n", arg);
            return 1;
        }

    ON_OPTION_WITH_ARG(SHORTOPT('a') || LONGOPT("algorithm"))
        if (strcmp(arg, "lbfgs") == 0) {
            free(opt->algorithm);
            opt->algorithm = mystrdup("lbfgs");
        } else if (strcmp(arg, "l2sgd") == 0) {
            free(opt->algorithm);
            opt->algorithm = mystrdup("l2sgd");
        } else if (strcmp(arg, "ap") == 0 || strcmp(arg, "averaged-perceptron") == 0) {
            free(opt->algorithm);
            opt->algorithm = mystrdup("averaged-perceptron");
        } else if (strcmp(arg, "pa") == 0 || strcmp(arg, "passive-aggressive") == 0) {
            free(opt->algorithm);
            opt->algorithm = mystrdup("passive-aggressive");
        } else if (strcmp(arg, "arow") == 0) {
            free(opt->algorithm);
            opt->algorithm = mystrdup("arow");
        } else {
            fprintf(stderr, "ERROR: Unknown algorithm: %s\n", arg);
            return 1;
        }

    ON_OPTION_WITH_ARG(SHORTOPT('p') || LONGOPT("set"))
        opt->params = (char **)realloc(opt->params, sizeof(char*) * (opt->num_params + 1));
        opt->params[opt->num_params] = mystrdup(arg);
        ++opt->num_params;

    ON_OPTION_WITH_ARG(SHORTOPT('m') || LONGOPT("model"))
        free(opt->model);
        opt->model = mystrdup(arg);

    ON_OPTION_WITH_ARG(SHORTOPT('g') || LONGOPT("split"))
        opt->split = atoi(arg);

    ON_OPTION_WITH_ARG(SHORTOPT('e') || LONGOPT("holdout"))
        opt->holdout = atoi(arg)-1;

    ON_OPTION(SHORTOPT('x') || LONGOPT("cross-validate"))
        opt->cross_validation = 1;

    ON_OPTION(SHORTOPT('l') || LONGOPT("log-to-file"))
        opt->logfile = 1;

    ON_OPTION_WITH_ARG(SHORTOPT('L') || LONGOPT("logbase"))
        free(opt->logbase);
        opt->logbase = mystrdup(arg);

    ON_OPTION(SHORTOPT('h') || LONGOPT("help"))
        opt->help = 1;

    ON_OPTION(SHORTOPT('H') || LONGOPT("help-params"))
        opt->help_params = 1;

END_OPTION_MAP()

static void show_usage(FILE *fp, const char *argv0, const char *command)
{
    fprintf(fp, "USAGE: %s %s [OPTIONS] [DATA1] [DATA2] ...\n", argv0, command);
    fprintf(fp, "Trains a model using training data set(s).\n");
    fprintf(fp, "\n");
    fprintf(fp, "  DATA    file(s) corresponding to data set(s) for training; if multiple N files\n");
    fprintf(fp, "          are specified, this utility assigns a group number (1...N) to the\n");
    fprintf(fp, "          instances in each file; if a file name is '-', the utility reads a\n");
    fprintf(fp, "          data set from STDIN\n");
    fprintf(fp, "\n");
    fprintf(fp, "OPTIONS:\n");
    fprintf(fp, "  -t, --type=TYPE       specify a graphical model (DEFAULT='1d'):\n");
    fprintf(fp, "                        (this option is reserved for the future use)\n");
    fprintf(fp, "      1d                    1st-order Markov CRF with state and transition\n");
    fprintf(fp, "                            features; transition features are not conditioned\n");
    fprintf(fp, "                            on observations\n");
    fprintf(fp, "  -a, --algorithm=NAME  specify a training algorithm (DEFAULT='lbfgs')\n");
    fprintf(fp, "      lbfgs                 L-BFGS with L1/L2 regularization\n");
    fprintf(fp, "      l2sgd                 SGD with L2-regularization\n");
    fprintf(fp, "      ap                    Averaged Perceptron\n");
    fprintf(fp, "      pa                    Passive Aggressive\n");
    fprintf(fp, "      arow                  Adaptive Regularization of Weights (AROW)\n");
    fprintf(fp, "  -p, --set=NAME=VALUE  set the algorithm-specific parameter NAME to VALUE;\n");
    fprintf(fp, "                        use '-H' or '--help-parameters' with the algorithm name\n");
    fprintf(fp, "                        specified by '-a' or '--algorithm' and the graphical\n");
    fprintf(fp, "                        model specified by '-t' or '--type' to see the list of\n");
    fprintf(fp, "                        algorithm-specific parameters\n");
    fprintf(fp, "  -m, --model=FILE      store the model to FILE (DEFAULT=''); if the value is\n");
    fprintf(fp, "                        empty, this utility does not store the model\n");
    fprintf(fp, "  -g, --split=N         split the instances into N groups; this option is\n");
    fprintf(fp, "                        useful for holdout evaluation and cross validation\n");
    fprintf(fp, "  -e, --holdout=M       use the M-th data for holdout evaluation and the rest\n");
    fprintf(fp, "                        for training\n");
    fprintf(fp, "  -x, --cross-validate  repeat holdout evaluations for #i in {1, ..., N} groups\n");
    fprintf(fp, "                        (N-fold cross validation)\n");
    fprintf(fp, "  -l, --log-to-file     write the training log to a file instead of to STDOUT;\n");
    fprintf(fp, "                        The filename is determined automatically by the training\n");
    fprintf(fp, "                        algorithm, parameters, and source files\n");
    fprintf(fp, "  -L, --logbase=BASE    set the base name for a log file (used with -l option)\n");
    fprintf(fp, "  -h, --help            show the usage of this command and exit\n");
    fprintf(fp, "  -H, --help-parameters show the help message of algorithm-specific parameters;\n");
    fprintf(fp, "                        specify an algorithm with '-a' or '--algorithm' option,\n");
    fprintf(fp, "                        and specify a graphical model with '-t' or '--type' option\n");
}



static int message_callback(void *instance, const char *format, va_list args)
{
    vfprintf(stdout, format, args);
    fflush(stdout);
    return 0;
}

int main_learn(int argc, char *argv[], const char *argv0)
{
    int i, n, groups = 1, ret = 0, arg_used = 0;
    time_t ts;
    char timestamp[80];
    char trainer_id[128];
    clock_t clk_begin, clk_current;
    learn_option_t opt;
    const char *command = argv[0];
    FILE *fpi = stdin, *fpo = stdout, *fpe = stderr;
    crfsuite_data_t data;
    crfsuite_trainer_t *trainer = NULL;
    crfsuite_dictionary_t *attrs = NULL, *labels = NULL;

    /* Initializations. */
    learn_option_init(&opt);
    crfsuite_data_init(&data);

    /* Parse the command-line option. */
    arg_used = option_parse(++argv, --argc, parse_learn_options, &opt);
    if (arg_used < 0) {
        ret = 1;
        goto force_exit;
    }

    /* Show the help message for this command if specified. */
    if (opt.help) {
        show_usage(fpo, argv0, command);
        goto force_exit;
    }

    /* Open a log file if necessary. */
    if (opt.logfile) {
        /* Generate a filename for the log file. */
        char *fname = NULL;
        fname = mystrcat(fname, opt.logbase);
        fname = mystrcat(fname, "_");
        fname = mystrcat(fname, opt.algorithm);
        for (i = 0;i < opt.num_params;++i) {
            fname = mystrcat(fname, "_");
            fname = mystrcat(fname, opt.params[i]);
        }

        fpo = fopen(fname, "w");
        if (fpo == NULL) {
            fprintf(fpe, "ERROR: Failed to open the log file.\n");
            ret = 1;
            goto force_exit;
        }
    }

    /* Create dictionaries for attributes and labels. */
    ret = crfsuite_create_instance("dictionary", (void**)&data.attrs);
    if (!ret) {
        fprintf(fpe, "ERROR: Failed to create a dictionary instance.\n");
        ret = 1;
        goto force_exit;
    }
    ret = crfsuite_create_instance("dictionary", (void**)&data.labels);
    if (!ret) {
        fprintf(fpe, "ERROR: Failed to create a dictionary instance.\n");
        ret = 1;
        goto force_exit;
    }

    /* Create a trainer instance. */
    sprintf(trainer_id, "train/%s/%s", opt.type, opt.algorithm);
    ret = crfsuite_create_instance(trainer_id, (void**)&trainer);
    if (!ret) {
        fprintf(fpe, "ERROR: Failed to create a trainer instance.\n");
        ret = 1;
        goto force_exit;
    }

    /* Show the help message for the training algorithm if specified. */
    if (opt.help_params) {
        crfsuite_params_t* params = trainer->params(trainer);

        fprintf(fpo, "PARAMETERS for %s (%s):\n", opt.algorithm, opt.type);
        fprintf(fpo, "\n");

        for (i = 0;i < params->num(params);++i) {
            char *name = NULL;
            char *type = NULL;
            char *value = NULL;
            char *help = NULL;

            params->name(params, i, &name);
            params->get(params, name, &value);
            params->help(params, name, &type, &help);

            fprintf(fpo, "%s %s = %s;\n", type, name, value);
            fprintf(fpo, "%s\n", help);
            fprintf(fpo, "\n");

            params->free(params, help);
            params->free(params, type);
            params->free(params, value);
            params->free(params, name);
        }

        params->release(params);
        goto force_exit;
    }

    /* Set parameters. */
    for (i = 0;i < opt.num_params;++i) {
        char *value = NULL;
        char *name = opt.params[i];
        crfsuite_params_t* params = trainer->params(trainer);
        
        /* Split the parameter argument by the first '=' character. */
        value = strchr(name, '=');
        if (value != NULL) {
            *value++ = 0;
        }

        if (params->set(params, name, value) != 0) {
            fprintf(fpe, "ERROR: paraneter not found: %s\n", name);
            goto force_exit;
        }
        params->release(params);
    }

    /* Log the start time. */
    time(&ts);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", gmtime(&ts));
    fprintf(fpo, "Start time of the training: %s\n", timestamp);
    fprintf(fpo, "\n");

    /* Read the training data. */
    fprintf(fpo, "Reading the data set(s)\n");
    for (i = arg_used;i < argc;++i) {
        FILE *fp = (strcmp(argv[i], "-") == 0) ? fpi : fopen(argv[i], "r");
        if (fp == NULL) {
            fprintf(fpe, "ERROR: Failed to open the data set: %s\n", argv[i]);
            ret = 1;
            goto force_exit;        
        }

        fprintf(fpo, "[%d] %s\n", i-arg_used+1, argv[i]);
        clk_begin = clock();
        n = read_data(fp, fpo, &data, i-arg_used);
        clk_current = clock();
        fprintf(fpo, "Number of instances: %d\n", n);
        fprintf(fpo, "Seconds required: %.3f\n", (clk_current - clk_begin) / (double)CLOCKS_PER_SEC);
        fclose(fp);
    }
    groups = argc-arg_used;
    fprintf(fpo, "\n");

    /* Split into data sets if necessary. */
    if (0 < opt.split) {
        /* Shuffle the instances. */
        for (i = 0;i < data.num_instances;++i) {
            int j = rand() % data.num_instances;
            crfsuite_instance_swap(&data.instances[i], &data.instances[j]);
        }

        /* Assign group numbers. */
        for (i = 0;i < data.num_instances;++i) {
            data.instances[i].group = i % opt.split;
        }
        groups = opt.split;
    }

    /* Report the statistics of the training data. */
    fprintf(fpo, "Statistics the data set(s)\n");
    fprintf(fpo, "Number of data sets (groups): %d\n", groups);
    fprintf(fpo, "Number of instances: %d\n", data.num_instances);
    fprintf(fpo, "Number of items: %d\n", crfsuite_data_totalitems(&data));
    fprintf(fpo, "Number of attributes: %d\n", data.attrs->num(data.attrs));
    fprintf(fpo, "Number of labels: %d\n", data.labels->num(data.labels));
    fprintf(fpo, "\n");
    fflush(fpo);

    /* Set callback procedures that receive messages and taggers. */
    trainer->set_message_callback(trainer, NULL, message_callback);

    /* Start training. */
    if (opt.cross_validation) {
        for (i = 0;i < groups;++i) {
            fprintf(fpo, "===== Cross validation (%d/%d) =====\n", i+1, groups);
            if (ret = trainer->train(trainer, &data, "", i)) {
                goto force_exit;
            }
            fprintf(fpo, "\n");
        }

    } else {
        if (ret = trainer->train(trainer, &data, opt.model, opt.holdout)) {
            goto force_exit;
        }

    }

    /* Log the end time. */
    time(&ts);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", gmtime(&ts));
    fprintf(fpo, "End time of the training: %s\n", timestamp);
    fprintf(fpo, "\n");

force_exit:
    SAFE_RELEASE(trainer);
    SAFE_RELEASE(data.labels);
    SAFE_RELEASE(data.attrs);

    crfsuite_data_finish(&data);
    learn_option_finish(&opt);
    if (fpo != NULL) {
        fclose(fpo);
    }

    return ret;
}
