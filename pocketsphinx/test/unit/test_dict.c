#include <stdio.h>
#include <string.h>

#include <pocketsphinx.h>
#include <bin_mdef.h>

#include "dict.h"
#include "test_macros.h"

int
main(int argc, char *argv[])
{
	bin_mdef_t *mdef;
	dict_t *dict;
	cmd_ln_t *config;

	int i;
	char buf[100];

	TEST_ASSERT(config = cmd_ln_init(NULL, NULL, FALSE,
						   "-dict", MODELDIR "/lm/en_US/cmu07a.dic",
						   "-fdict", MODELDIR "/hmm/en_US/hub4wsj_sc_8k/noisedict",
						   NULL));

	/* Test dictionary in standard fashion. */
	TEST_ASSERT(mdef = bin_mdef_read(NULL, MODELDIR "/hmm/en_US/hub4wsj_sc_8k/mdef"));
	TEST_ASSERT(dict = dict_init(config, mdef));

	printf("Word ID (CARNEGIE) = %d\n",
	       dict_wordid(dict, "CARNEGIE"));
	printf("Word ID (ASDFASFASSD) = %d\n",
	       dict_wordid(dict, "ASDFASFASSD"));

	TEST_EQUAL(0, dict_write(dict, "_cmu07a.dic", NULL));
	TEST_EQUAL(0, system("diff -uw " MODELDIR "/lm/en_US/cmu07a.dic _cmu07a.dic"));

	dict_free(dict);
	bin_mdef_free(mdef);

	/* Now test an empty dictionary. */
	TEST_ASSERT(dict = dict_init(NULL, NULL));
	printf("Word ID(<s>) = %d\n", dict_wordid(dict, "<s>"));
	TEST_ASSERT(BAD_S3WID != dict_add_word(dict, "FOOBIE", NULL, 0));
	TEST_ASSERT(BAD_S3WID != dict_add_word(dict, "BLETCH", NULL, 0));
	printf("Word ID(FOOBIE) = %d\n", dict_wordid(dict, "FOOBIE"));
	printf("Word ID(BLETCH) = %d\n", dict_wordid(dict, "BLETCH"));
	TEST_ASSERT(dict_real_word(dict, dict_wordid(dict, "FOOBIE")));
	TEST_ASSERT(dict_real_word(dict, dict_wordid(dict, "BLETCH")));
	TEST_ASSERT(!dict_real_word(dict, dict_wordid(dict, "</s>")));
	dict_free(dict);

	/* Test to add 500k words. */
	TEST_ASSERT(dict = dict_init(NULL, NULL));
	for (i = 0; i < 500000; i++) {
	    sprintf(buf, "word_%d", i);
    	    TEST_ASSERT(BAD_S3WID != dict_add_word(dict, buf, NULL, 0));
	}
	dict_free(dict);

	cmd_ln_free_r(config);

	return 0;
}
