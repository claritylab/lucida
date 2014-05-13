/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */

#include <stdio.h>
#include <string.h>

#include "cmd_ln.h"
#include "ckd_alloc.h"

const arg_t defs[] = {
    { "-a", ARG_INT32, "42", "This is the first argument." },
    { "-b", ARG_STRING, NULL, "This is the second argument." },
    { "-c", ARG_BOOLEAN, "no", "This is the third argument." },
    { "-d", ARG_FLOAT64, "1e-50", "This is the fourth argument." },
    { NULL, 0, NULL, NULL }
};

static char *orig_argv[] = {
    "foobie",
    "-a", "69"
};
static int orig_argc = sizeof(orig_argv) / sizeof(orig_argv[0]);

int
main(int argc, char *argv[])
{
    cmd_ln_parse(defs, orig_argc, orig_argv, TRUE);
    cmd_ln_parse(defs, argc, argv, FALSE);
    printf("%d %s %d %f\n",
           cmd_ln_int32("-a"),
           cmd_ln_str("-b") ? cmd_ln_str("-b") : "(null)",
           cmd_ln_boolean("-c"),
           cmd_ln_float64("-d"));
    cmd_ln_free();
           
    return 0;
}
