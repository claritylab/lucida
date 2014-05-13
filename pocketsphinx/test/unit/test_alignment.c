#include <pocketsphinx.h>

#include "ps_alignment.h"
#include "pocketsphinx_internal.h"

#include "test_macros.h"

int
main(int argc, char *argv[])
{
	bin_mdef_t *mdef;
	dict_t *dict;
	dict2pid_t *d2p;
	ps_alignment_t *al;
	ps_alignment_iter_t *itor;
	cmd_ln_t *config;

	config = cmd_ln_init(NULL, NULL, FALSE,
			     "-dict", MODELDIR "/lm/en_US/cmu07a.dic",
			     "-fdict", MODELDIR "/hmm/en_US/hub4wsj_sc_8k/noisedict",
			     NULL);
	mdef = bin_mdef_read(NULL, MODELDIR "/hmm/en_US/hub4wsj_sc_8k/mdef");
	dict = dict_init(config, mdef);
	d2p = dict2pid_build(mdef, dict);

	al = ps_alignment_init(d2p);
	TEST_EQUAL(1, ps_alignment_add_word(al, dict_wordid(dict, "<s>"), 0));
	TEST_EQUAL(2, ps_alignment_add_word(al, dict_wordid(dict, "hello"), 0));
	TEST_EQUAL(3, ps_alignment_add_word(al, dict_wordid(dict, "world"), 0));
	TEST_EQUAL(4, ps_alignment_add_word(al, dict_wordid(dict, "</s>"), 0));
	TEST_EQUAL(0, ps_alignment_populate(al));

	itor = ps_alignment_words(al);
	TEST_EQUAL(ps_alignment_iter_get(itor)->id.wid, dict_wordid(dict, "<s>"));
	itor = ps_alignment_iter_next(itor);
	TEST_EQUAL(ps_alignment_iter_get(itor)->id.wid, dict_wordid(dict, "hello"));
	itor = ps_alignment_iter_next(itor);
	TEST_EQUAL(ps_alignment_iter_get(itor)->id.wid, dict_wordid(dict, "world"));
	itor = ps_alignment_iter_next(itor);
	TEST_EQUAL(ps_alignment_iter_get(itor)->id.wid, dict_wordid(dict, "</s>"));
	itor = ps_alignment_iter_next(itor);
	TEST_EQUAL(itor, NULL);

	printf("%d words %d phones %d states\n",
	       ps_alignment_n_words(al),
	       ps_alignment_n_phones(al),
	       ps_alignment_n_states(al));

	ps_alignment_free(al);
	dict_free(dict);
	dict2pid_free(d2p);
	bin_mdef_free(mdef);
	cmd_ln_free_r(config);

	return 0;
}
