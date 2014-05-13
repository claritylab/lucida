/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2007 Carnegie Mellon University.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * This work was supported in part by funding from the Defense Advanced 
 * Research Projects Agency and the National Science Foundation of the 
 * United States of America, and the CMU Sphinx Speech Consortium.
 *
 * THIS SOFTWARE IS PROVIDED BY CARNEGIE MELLON UNIVERSITY ``AS IS'' AND 
 * ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
 * NOR ITS EMPLOYEES BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ====================================================================
 *
 */

#include <string.h>

#include <sphinxbase/hash_table.h>
#include <sphinxbase/fsg_model.h>
#include <sphinxbase/jsgf.h>
#include <sphinxbase/err.h>
#include <sphinxbase/strfuncs.h>

static const arg_t defn[] = {
  { "-help",
    ARG_BOOLEAN,
    "no",
    "Shows the usage of the tool"},

  { "-jsgf",
    REQARG_STRING,
    NULL,
    "Input grammar in jsgf format (required)"},

  { "-rule",
    ARG_STRING,
    NULL,
    "Root rule name (optional)"},

  { "-fsg",
    ARG_STRING,
    NULL,
    "Output grammar in fsg format"},

  { "-fsm",
    ARG_STRING,
    NULL,
    "Output grammar in FSM format"},

  { "-symtab",
    ARG_STRING,
    NULL,
    "Output symtab for grammar in FSM format"},

  { "-compile",
    ARG_BOOLEAN,
    "no",
    "Compute grammar closure to speedup loading"},

  { NULL, 0, NULL, NULL }
};


static void
usagemsg(char *pgm)
{
    E_INFO("Usage: %s -jsgf <input.jsgf> -rule <rule name>\\\n", pgm);
    E_INFOCONT("\t[-fsm yes/no] [-compile yes/no]\n");
    E_INFOCONT("\t-fsg <output.fsg>\n");

    exit(0);
}

static fsg_model_t *
get_fsg(jsgf_t *grammar, const char *name)
{
    jsgf_rule_iter_t *itor;
    logmath_t *lmath = logmath_init(1.0001, 0, 0);
    fsg_model_t *fsg = NULL;

    for (itor = jsgf_rule_iter(grammar); itor;
         itor = jsgf_rule_iter_next(itor)) {
        jsgf_rule_t *rule = jsgf_rule_iter_rule(itor);
        char const *rule_name = jsgf_rule_name(rule);

        if ((name == NULL && jsgf_rule_public(rule))
            || (name && strlen(rule_name)-2 == strlen(name) &&
                0 == strncmp(rule_name + 1, name, strlen(rule_name) - 2))) {
            fsg = jsgf_build_fsg_raw(grammar, rule, logmath_retain(lmath), 1.0);
            jsgf_rule_iter_free(itor);
            break;
        }
    }

    logmath_free(lmath);
    return fsg;
}

int
main(int argc, char *argv[])
{
    jsgf_t *jsgf;
    fsg_model_t *fsg;
    cmd_ln_t *config;
        
    if ((config = cmd_ln_parse_r(NULL, defn, argc, argv, TRUE)) == NULL)
	return 1;
		
    if (cmd_ln_boolean_r(config, "-help")) {
        usagemsg(argv[0]);
    }

    jsgf = jsgf_parse_file(cmd_ln_str_r(config, "-jsgf"), NULL);
    if (jsgf == NULL) {
        return 1;
    }

    fsg = get_fsg(jsgf, cmd_ln_str_r(config, "-rule") ? cmd_ln_str_r(config, "-rule") : NULL);
    
    if (cmd_ln_boolean_r(config, "-compile")) {
	fsg_model_null_trans_closure(fsg, NULL);
    }

    
    if (cmd_ln_str_r(config, "-fsm")) {
	const char* outfile = cmd_ln_str_r(config, "-fsm");
	const char* symfile = cmd_ln_str_r(config, "-symtab");
        if (outfile)
            fsg_model_writefile_fsm(fsg, outfile);
        else
            fsg_model_write_fsm(fsg, stdout);
        if (symfile)
            fsg_model_writefile_symtab(fsg, symfile);
    }
    else {
        const char *outfile = cmd_ln_str_r(config, "-fsg");
        if (outfile)
            fsg_model_writefile(fsg, outfile);
        else
            fsg_model_write(fsg, stdout);
    }
    fsg_model_free(fsg);
    jsgf_grammar_free(jsgf);

    return 0;
}


#if defined(_WIN32_WCE)
#pragma comment(linker,"/entry:mainWCRTStartup")
#include <windows.h>

/* Windows Mobile has the Unicode main only */
int wmain(int32 argc, wchar_t *wargv[]) {
    char** argv;
    size_t wlen;
    size_t len;
    int i;

    argv = malloc(argc*sizeof(char*));
    for (i = 0; i < argc; i++){
        wlen = lstrlenW(wargv[i]);
        len = wcstombs(NULL, wargv[i], wlen);
        argv[i] = malloc(len+1);
        wcstombs(argv[i], wargv[i], wlen);
    }

    /* assuming ASCII parameters */
    return main(argc, argv);
}
#endif
