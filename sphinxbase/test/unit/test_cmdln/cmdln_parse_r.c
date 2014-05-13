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
    { "-l", ARG_STRING_LIST, NULL, "This is the fifth argument." },
    { NULL, 0, NULL, NULL }
};

int
main(int argc, char *argv[])
{
    cmd_ln_t *config;

    config = cmd_ln_parse_r(NULL, defs, argc, argv, TRUE);
    if (config == NULL)
        return 1;
    printf("%d %s %d %f\n",
           cmd_ln_int32_r(config, "-a"),
           cmd_ln_str_r(config, "-b") ? cmd_ln_str_r(config, "-b") : "(null)",
           cmd_ln_boolean_r(config, "-c"),
           cmd_ln_float64_r(config, "-d"));
    cmd_ln_free_r(config);

    config = cmd_ln_init(NULL, NULL, FALSE,
                         "-b", "foobie", NULL);
    if (config == NULL)
        return 1;
    cmd_ln_free_r(config);

    config = cmd_ln_init(NULL, defs, TRUE,
                         "-b", "foobie", NULL);
    if (config == NULL)
        return 1;
    printf("%d %s %d %f\n",
           cmd_ln_int32_r(config, "-a"),
           cmd_ln_str_r(config, "-b") ? cmd_ln_str_r(config, "-b") : "(null)",
           cmd_ln_boolean_r(config, "-c"),
           cmd_ln_float64_r(config, "-d"));
    cmd_ln_free_r(config);

    config = cmd_ln_init(NULL, NULL, FALSE,
                         "-b", "foobie", NULL);
    if (config == NULL)
        return 1;
    printf("%s\n",
           cmd_ln_str_r(config, "-b") ? cmd_ln_str_r(config, "-b") : "(null)");
    cmd_ln_set_str_r(config, "-b", "blatz");
    printf("%s\n",
           cmd_ln_str_r(config, "-b") ? cmd_ln_str_r(config, "-b") : "(null)");
    cmd_ln_free_r(config);
           
    return 0;
}
