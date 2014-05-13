#include <pocketsphinx.h>
#include <stdio.h>
#include <string.h>

#include "pocketsphinx_internal.h"
#include "ps_lattice_internal.h"
#include "test_macros.h"
#include "ps_test.c"

int
test_nodes_and_stuff(ps_lattice_t *dag)
{
	ps_latnode_iter_t *itor;
	ps_latlink_iter_t *litor;
	ps_latnode_t *forward = NULL;

	TEST_ASSERT(itor = ps_latnode_iter(dag));
	while ((itor = ps_latnode_iter_next(itor))) {
		int16 sf, fef, lef;
		ps_latnode_t *node;
		float64 post;

		node = ps_latnode_iter_node(itor);
		sf = ps_latnode_times(node, &fef, &lef);
		post = logmath_exp(ps_lattice_get_logmath(dag),
				   ps_latnode_prob(dag, node, NULL));
		if (post > 0.0001)
			printf("%s %s %d -> (%d,%d) %f\n",
			       ps_latnode_baseword(dag, node),
			       ps_latnode_word(dag, node),
			       sf, fef, lef, post);
		if (0 == strcmp(ps_latnode_baseword(dag, node), "forward"))
			forward = node;
	}
	TEST_ASSERT(forward);

	printf("FORWARD entries:\n");
	for (litor = ps_latnode_entries(forward);
	     litor; litor = ps_latlink_iter_next(litor)) {
		ps_latlink_t *link = ps_latlink_iter_link(litor);
		int16 sf, ef;
		float64 post;
		int32 ascr;

		ef = ps_latlink_times(link, &sf);
		post = logmath_exp(ps_lattice_get_logmath(dag),
				   ps_latlink_prob(dag, link, &ascr));
		if (post > 0.0001)
			printf("%s %d -> %d prob %f ascr %d\n",
			       ps_latlink_baseword(dag, link),
			       sf, ef, post, ascr);
	}
	printf("FORWARD exits:\n");
	for (litor = ps_latnode_exits(forward);
	     litor; litor = ps_latlink_iter_next(litor)) {
		ps_latlink_t *link = ps_latlink_iter_link(litor);
		int16 sf, ef;
		float64 post;
		int32 ascr;

		ef = ps_latlink_times(link, &sf);
		post = logmath_exp(ps_lattice_get_logmath(dag),
				   ps_latlink_prob(dag, link, &ascr));
		if (post > 0.0001)
			printf("%d -> %d prob %f ascr %d\n",
			       sf, ef, post, ascr);
	}
	return 0;
}

int
test_remaining_nodes(ps_lattice_t *dag) {
	ps_latnode_iter_t *itor;
	ps_latlink_iter_t *litor;
	int count, lcount;
    
	count = 0;
	lcount = 0;
	for (itor = ps_latnode_iter(dag); itor; itor = ps_latnode_iter_next(itor)) {
	    ps_latnode_t* node = ps_latnode_iter_node(itor);
	    for (litor = ps_latnode_entries(node);
		litor; litor = ps_latlink_iter_next(litor)) {
		lcount++;
	    }
	    count++;	
	}
	TEST_ASSERT(count == 3);
	TEST_ASSERT(lcount == 2);
	printf("Remaining %d nodes %d links\n", count, lcount);

	return 0;    
}


int
main(int argc, char *argv[])
{
	ps_decoder_t *ps;
	ps_lattice_t *dag;
	cmd_ln_t *config;
	FILE *rawfh;
	char const *hyp;
	char const *uttid;
	int32 score;

	TEST_ASSERT(config =
		    cmd_ln_init(NULL, ps_args(), TRUE,
				"-hmm", MODELDIR "/hmm/en_US/hub4wsj_sc_8k",
				"-lm", MODELDIR "/lm/en_US/wsj0vp.5000.DMP",
				"-dict", MODELDIR "/lm/en_US/cmu07a.dic",
				"-fwdtree", "yes",
				"-fwdflat", "no",
				"-bestpath", "no",
				"-input_endian", "little",
				"-samprate", "16000", NULL));
	TEST_ASSERT(ps = ps_init(config));
	TEST_ASSERT(rawfh = fopen(DATADIR "/goforward.raw", "rb"));
	ps_decode_raw(ps, rawfh, "goforward", -1);
	fclose(rawfh);
	hyp = ps_get_hyp(ps, &score, &uttid);
	printf("FWDFLAT (%s): %s (%d)\n", uttid, hyp, score);
	TEST_ASSERT(dag = ps_get_lattice(ps));
	ps_lattice_bestpath(dag, ps_get_lm(ps, PS_DEFAULT_SEARCH), 1.0, 1.0/15.0);
	score = ps_lattice_posterior(dag, ps_get_lm(ps, PS_DEFAULT_SEARCH), 1.0/15.0);
	printf("P(S|O) = %d\n", score);
	test_nodes_and_stuff(dag);
	ps_lattice_posterior_prune(dag, logmath_log(ps_lattice_get_logmath(dag), 1e-2)); 
	test_nodes_and_stuff(dag);

	TEST_EQUAL(0, ps_lattice_write(dag, "goforward.lat"));

	dag = ps_lattice_read(ps, "goforward.lat");
	TEST_ASSERT(dag);
	ps_lattice_bestpath(dag, ps_get_lm(ps, PS_DEFAULT_SEARCH), 1.0, 1.0/15.0);
	score = ps_lattice_posterior(dag, ps_get_lm(ps, PS_DEFAULT_SEARCH), 1.0/15.0);
	printf("P(S|O) = %d\n", score);
	test_nodes_and_stuff(dag);
	ps_lattice_free(dag);
	ps_free(ps);
	cmd_ln_free_r(config);

	/* Now test standalone lattices. */
	dag = ps_lattice_read(NULL, "goforward.lat");
	TEST_ASSERT(dag);
	test_nodes_and_stuff(dag);
	ps_lattice_free(dag);

	/* Test stripping the unreachable nodes. */
	dag = ps_lattice_read(NULL, DATADIR "/unreachable.lat");
	TEST_ASSERT(dag);
	ps_lattice_delete_unreachable(dag);
	test_remaining_nodes(dag);
	ps_lattice_free(dag);
	
	return 0;
}
