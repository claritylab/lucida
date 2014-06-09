/*
 *        Dump command for CRFsuite frontend.
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

#include <crfsuite.h>
#include "option.h"

#define    SAFE_RELEASE(obj)    if ((obj) != NULL) { (obj)->release(obj); (obj) = NULL; }

typedef struct {
    int help;
} dump_option_t;

static void dump_option_init(dump_option_t* opt)
{
    memset(opt, 0, sizeof(*opt));
}

static void dump_option_finish(dump_option_t* opt)
{
}

BEGIN_OPTION_MAP(parse_dump_options, dump_option_t)

    ON_OPTION(SHORTOPT('h') || LONGOPT("help"))
        opt->help = 1;

END_OPTION_MAP()

static void show_usage(FILE *fp, const char *argv0, const char *command)
{
    fprintf(fp, "USAGE: %s %s [OPTIONS] <MODEL>\n", argv0, command);
    fprintf(fp, "Output the model stored in the file (MODEL) in a plain-text format\n");
    fprintf(fp, "\n");
    fprintf(fp, "OPTIONS:\n");
    fprintf(fp, "    -h, --help      Show the usage of this command and exit\n");
}

int main_dump(int argc, char *argv[], const char *argv0)
{
    int ret = 0, arg_used = 0;
    dump_option_t opt;
    const char *command = argv[0];
    FILE *fp = NULL, *fpi = stdin, *fpo = stdout, *fpe = stderr;
    crfsuite_model_t *model = NULL;

    /* Parse the command-line option. */
    dump_option_init(&opt);
    arg_used = option_parse(++argv, --argc, parse_dump_options, &opt);
    if (arg_used < 0) {
        ret = 1;
        goto force_exit;
    }

    /* Show the help message for this command if specified. */
    if (opt.help) {
        show_usage(fpo, argv0, command);
        goto force_exit;
    }

    /* Check for the existence of the model file. */
    if (argc <= arg_used) {
        fprintf(fpe, "ERROR: No model specified.\n");
        ret = 1;
        goto force_exit;
    }

    /* Create a model instance corresponding to the model file. */
    if (ret = crfsuite_create_instance_from_file(argv[arg_used], (void**)&model)) {
        goto force_exit;
    }
        
    /* Dump the model. */
    if (ret = model->dump(model, fpo)) {
        goto force_exit;
    }

force_exit:
    SAFE_RELEASE(model);
    dump_option_finish(&opt);
    return ret;
}
