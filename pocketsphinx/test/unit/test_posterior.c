#include <pocketsphinx.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "pocketsphinx_internal.h"
#include "ngram_search_fwdtree.h"
#include "ps_lattice_internal.h"
#include "test_macros.h"

int
test_decode(ps_decoder_t *ps)
{
	FILE *rawfh;
	int16 buf[2048];
	size_t nread;
	int16 const *bptr;
	int nfr;
	ps_lattice_t *dag;
	acmod_t *acmod;
	ngram_search_t *ngs;
	int i, j;
	ps_latlink_t *link;
	ps_latnode_t *node;
	latlink_list_t *x;
	int32 norm, post;

	ngs = (ngram_search_t *)ps->search;
	acmod = ps->acmod;

	/* Decode stuff and build a DAG. */
	TEST_ASSERT(rawfh = fopen(DATADIR "/goforward.raw", "rb"));
	TEST_EQUAL(0, acmod_start_utt(acmod));
	ngram_fwdtree_start(ngs);
	while (!feof(rawfh)) {
		nread = fread(buf, sizeof(*buf), 2048, rawfh);
		bptr = buf;
		while ((nfr = acmod_process_raw(acmod, &bptr, &nread, FALSE)) > 0) {
			while (acmod->n_feat_frame > 0) {
				ngram_fwdtree_search(ngs, acmod->output_frame);
				acmod_advance(acmod);
			}
		}
	}
	ngram_fwdtree_finish(ngs);
	printf("FWDTREE: %s\n",
	       ngram_search_bp_hyp(ngs, ngram_search_find_exit(ngs, -1, NULL, NULL)));

	TEST_ASSERT(acmod_end_utt(acmod) >= 0);
	fclose(rawfh);

	dag = ngram_search_lattice(ps->search);
	if (dag == NULL) {
		E_ERROR("Failed to build DAG!\n");
		return -1;
	}

	/* Write lattice to disk. */
	TEST_EQUAL(0, ps_lattice_write(dag, "test_posterior.lat"));

	/* Do a bunch of checks on the DAG generation and traversal code: */
	/* Verify that forward and backward iteration give the same number of edges. */
	i = j = 0;
	for (link = ps_lattice_traverse_edges(dag, NULL, NULL);
	     link; link = ps_lattice_traverse_next(dag, NULL)) {
		++i;
	}
	for (link = ps_lattice_reverse_edges(dag, NULL, NULL);
	     link; link = ps_lattice_reverse_next(dag, NULL)) {
		++j;
	}
	printf("%d forward edges, %d reverse edges\n", i, j);
	TEST_EQUAL(i,j);
	/* Verify that the same links are reachable via entries and exits. */
	for (node = dag->nodes; node; node = node->next) {
		for (x = node->exits; x; x = x->next)
			x->link->alpha = -42;
	}
	for (node = dag->nodes; node; node = node->next) {
		for (x = node->entries; x; x = x->next)
			TEST_EQUAL(x->link->alpha, -42);
	}
	/* Verify that forward iteration is properly ordered. */
	for (link = ps_lattice_traverse_edges(dag, NULL, NULL);
	     link; link = ps_lattice_traverse_next(dag, NULL)) {
		link->alpha = 0;
		for (x = link->from->entries; x; x = x->next) {
			TEST_EQUAL(x->link->alpha, 0);
		}
	}
	/* Verify that backward iteration is properly ordered. */
	for (node = dag->nodes; node; node = node->next) {
		for (x = node->exits; x; x = x->next)
			x->link->alpha = -42;
	}
	for (link = ps_lattice_reverse_edges(dag, NULL, NULL);
	     link; link = ps_lattice_reverse_next(dag, NULL)) {
		link->alpha = 0;
		for (x = link->to->exits; x; x = x->next) {
			TEST_EQUAL(x->link->alpha, 0);
		}
	}
	
	/* Find and print best path. */
	link = ps_lattice_bestpath(dag, ngs->lmset, 1.0, 1.0/20.0);
	printf("BESTPATH: %s\n", ps_lattice_hyp(dag, link));

	/* Calculate betas. */
	post = ps_lattice_posterior(dag, ngs->lmset, 1.0/20.0);
	printf("Best path score: %d\n",
	       link->path_scr + dag->final_node_ascr);
	printf("P(S|O) = %d\n", post);

	/* Verify that sum of final alphas and initial alphas+betas is
	 * sufficiently similar. */
	norm = logmath_get_zero(acmod->lmath);
	for (x = dag->start->exits; x; x = x->next)
		norm = logmath_add(acmod->lmath, norm, x->link->beta + x->link->alpha);
	E_INFO("Sum of final alphas+betas = %d\n", dag->norm);
	E_INFO("Sum of initial alphas+betas = %d\n", norm);
	TEST_EQUAL_LOG(dag->norm, norm);

	/* Print posterior probabilities for each link in best path. */
	while (link) {
		printf("P(%s,%d) = %d = %f\n",
		       dict_wordstr(ps->search->dict, link->from->wid),
		       link->ef,
		       link->alpha + link->beta - dag->norm,
		       logmath_exp(acmod->lmath, link->alpha + link->beta - dag->norm));
		link = link->best_prev;
	}

	return 0;
}

int
main(int argc, char *argv[])
{
	ps_decoder_t *ps;
	cmd_ln_t *config;
	int rv;

	TEST_ASSERT(config =
		    cmd_ln_init(NULL, ps_args(), TRUE,
				"-hmm", MODELDIR "/hmm/en_US/hub4wsj_sc_8k",
				"-lm", MODELDIR "/lm/en_US/wsj0vp.5000.DMP",
				"-dict", MODELDIR "/lm/en_US/cmu07a.dic",
				"-fwdtree", "yes",
				"-fwdflat", "no",
				"-bestpath", "yes",
				"-input_endian", "little",
				"-cmninit", "37",
				"-samprate", "16000", NULL));
	TEST_ASSERT(ps = ps_init(config));
	rv = test_decode(ps);
	ps_free(ps);
	cmd_ln_free_r(config);

	return rv;
}
