#include <pocketsphinx.h>
#include <stdio.h>
#include <string.h>

#include "pocketsphinx_internal.h"
#include "test_macros.h"
#include "ps_test.c"

int
main(int argc, char *argv[])
{
    cmd_ln_t *config;

    TEST_ASSERT(config =
            cmd_ln_init(NULL, ps_args(), TRUE,
                "-hmm", MODELDIR "/hmm/en_US/hub4wsj_sc_8k",
                "-lm", MODELDIR "/lm/en_US/wsj0vp.5000.DMP",
                "-dict", MODELDIR "/lm/en_US/cmu07a.dic",
                "-fwdtree", "yes",
                "-fwdflat", "yes",
                "-bestpath", "yes",
                "-mfclogdir", ".",
                "-rawlogdir", ".",
                "-input_endian", "little",
                "-samprate", "16000", NULL));
    return ps_decoder_test(config, "BESTPATH", "go forward ten readers");
}
