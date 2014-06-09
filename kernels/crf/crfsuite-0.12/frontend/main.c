/*
 *        CRFsuite frontend.
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
#include "option.h"
#include <crfsuite.h>

#define    APPLICATION_S    "CRFSuite"

int main_learn(int argc, char *argv[], const char *argv0);
int main_tag(int argc, char *argv[], const char *argv0);
int main_dump(int argc, char *argv[], const char *argv0);



typedef struct {
    int help;            /**< Show help message and exit. */

    FILE *fpi;
    FILE *fpo;
    FILE *fpe;
} option_t;

static void option_init(option_t* opt)
{
    memset(opt, 0, sizeof(*opt));
}

static void option_finish(option_t* opt)
{
}

BEGIN_OPTION_MAP(parse_options, option_t)

    ON_OPTION(SHORTOPT('h') || LONGOPT("help"))
        opt->help = 1;

END_OPTION_MAP()

void show_copyright(FILE *fp)
{
    fprintf(fp, APPLICATION_S " " CRFSUITE_VERSION "  " CRFSUITE_COPYRIGHT "\n");
    fprintf(fp, "\n");
}

static void show_usage(FILE *fp, const char *argv0)
{
    fprintf(fp, "USAGE: %s <COMMAND> [OPTIONS]\n", argv0);
    fprintf(fp, "    COMMAND     Command name to specify the processing\n");
    fprintf(fp, "    OPTIONS     Arguments for the command (optional; command-specific)\n");
    fprintf(fp, "\n");
    fprintf(fp, "COMMAND:\n");
    fprintf(fp, "    learn       Obtain a model from a training set of instances\n");
    fprintf(fp, "    tag         Assign suitable labels to given instances by using a model\n");
    fprintf(fp, "    dump        Output a model in a plain-text format\n");
    fprintf(fp, "\n");
    fprintf(fp, "For the usage of each command, specify -h option in the command argument.\n");
}


int main(int argc, char *argv[])
{
    option_t opt;
    int arg_used = 0;
    const char *command = NULL;
    const char *argv0 = argv[0];
    FILE *fpi = stdin, *fpo = stdout, *fpe = stderr;

    /* Parse the command-line option. */
    option_init(&opt);
    arg_used = option_parse(++argv, --argc, parse_options, &opt);
    if (arg_used < 0) {
        return 1;
    }

    /* Show the help message if specified. */
    if (opt.help) {
        show_copyright(fpo);
        show_usage(fpo, argv0);
        return 0;
    }

    /* Check whether a command is specified in the command-line. */
    if (argc <= arg_used) {
        fprintf(fpe, "ERROR: No command specified. See help (-h) for the usage.\n");
        return 1;
    }

    /* Execute the command. */
    command = argv[arg_used];
    if (strcmp(command, "learn") == 0) {
        show_copyright(fpo);
        return main_learn(argc-arg_used, argv+arg_used, argv0);
    } else if (strcmp(command, "tag") == 0) {
        return main_tag(argc-arg_used, argv+arg_used, argv0);
    } else if (strcmp(command, "dump") == 0) {
        return main_dump(argc-arg_used, argv+arg_used, argv0);
    } else {
        fprintf(fpe, "ERROR: Unrecognized command (%s) specified.\n", command);    
        return 1;
    }

    return 0;
}
