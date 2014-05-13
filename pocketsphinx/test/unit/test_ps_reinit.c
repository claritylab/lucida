#include <pocketsphinx.h>
#include <stdio.h>
#include <string.h>

#include "test_macros.h"

int
main(int argc, char *argv[])
{
	ps_decoder_t *ps;
	cmd_ln_t *config;

	TEST_ASSERT(config =
		    cmd_ln_init(NULL, ps_args(), TRUE,
				"-hmm", MODELDIR "/hmm/en_US/hub4wsj_sc_8k",
				"-lm", MODELDIR "/lm/en_US/wsj0vp.5000.DMP",
				"-dict", MODELDIR "/lm/en_US/cmu07a.dic",
				"-fwdtree", "yes",
				"-fwdflat", "yes",
				"-bestpath", "yes",
				"-input_endian", "little",
				"-samprate", "16000", NULL));
	TEST_ASSERT(ps = ps_init(config));
	cmd_ln_free_r(config);

	TEST_ASSERT(config =
		    cmd_ln_init(NULL, ps_args(), TRUE,
				"-hmm", MODELDIR "/hmm/en/tidigits",
				"-lm", MODELDIR "/lm/en/tidigits.DMP",
				"-dict", MODELDIR "/lm/en/tidigits.dic",
				"-fwdtree", "yes",
				"-fwdflat", "yes",
				"-bestpath", "yes",
				"-input_endian", "little",
				"-samprate", "16000", NULL));
	TEST_EQUAL(0, ps_reinit(ps, config));

	ps_free(ps);
	cmd_ln_free_r(config);

	return 0;
}
