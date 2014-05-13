#include <ngram_model.h>
#include <logmath.h>
#include <strfuncs.h>

#include "test_macros.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int
main(int argc, char *argv[])
{
	logmath_t *lmath;
	ngram_model_t *model;
	ngram_iter_t *itor;
	int i;

	/* Initialize a logmath object to pass to ngram_read */
	lmath = logmath_init(1.0001, 0, 0);
	/* Read a language model */
	model = ngram_model_read(NULL, LMDIR "/100.arpa.DMP", NGRAM_DMP, lmath);
	TEST_ASSERT(model);

	for (i = 0, itor = ngram_model_mgrams(model, 0);
	     itor; ++i, itor = ngram_iter_next(itor)) {
		int32 score, bowt;
		int32 const *wids = ngram_iter_get(itor, &score, &bowt);

		/*
		printf("%.4f %s %.4f\n",
		       logmath_log_to_log10(lmath, score),
		       ngram_word(model, wids[0]),
		       logmath_log_to_log10(lmath, bowt));
		*/

		if (i == 0) TEST_EQUAL(wids[0], ngram_wid(model, "<UNK>"));
		if (i == 1) TEST_EQUAL(wids[0], ngram_wid(model, "'s"));
	}

	for (i = 0, itor = ngram_model_mgrams(model, 1);
	     itor; ++i, itor = ngram_iter_next(itor)) {
		int32 score, bowt;
		int32 const *wids = ngram_iter_get(itor, &score, &bowt);

		/*
		printf("%.4f %s %s %.4f\n",
		       logmath_log_to_log10(lmath, score),
		       ngram_word(model, wids[0]),
		       ngram_word(model, wids[1]),
		       logmath_log_to_log10(lmath, bowt));
		*/

		/* FIXME: These tests are not sufficient - actually we
		 * need to make sure all word IDs line up
		 * correctly. */
		if (i == 0) TEST_EQUAL(wids[0], ngram_wid(model, "'s"));
		if (i == 0) TEST_EQUAL(wids[1], ngram_wid(model, "an"));
		if (i == 1) TEST_EQUAL(wids[0], ngram_wid(model, "'s"));
		if (i == 1) TEST_EQUAL(wids[1], ngram_wid(model, "going"));
	}

	for (i = 0, itor = ngram_model_mgrams(model, 2);
	     itor; ++i, itor = ngram_iter_next(itor)) {
		int32 score, bowt;
		int32 const *wids = ngram_iter_get(itor, &score, &bowt);

		/*
		printf("%.4f %s %s %s\n",
		       logmath_log_to_log10(lmath, score),
		       ngram_word(model, wids[0]),
		       ngram_word(model, wids[1]),
		       ngram_word(model, wids[2]));
		*/

		/* FIXME: These tests are not sufficient - actually we
		 * need to make sure all word IDs line up
		 * correctly. */
		if (i == 0) TEST_EQUAL(wids[0], ngram_wid(model, "'s"));
		if (i == 0) TEST_EQUAL(wids[1], ngram_wid(model, "an"));
		if (i == 0) TEST_EQUAL(wids[2], ngram_wid(model, "r"));
		if (i == 1) TEST_EQUAL(wids[0], ngram_wid(model, "'s"));
		if (i == 1) TEST_EQUAL(wids[1], ngram_wid(model, "going"));
		if (i == 1) TEST_EQUAL(wids[2], ngram_wid(model, "so"));
	}

	{
		ngram_iter_t *itor2, *itor3;
		int32 score, bowt;
		int32 const *wids;

		/* Test the boundary condition - successors of last 1-gram. */
		itor = ngram_ng_iter(model, ngram_model_get_counts(model)[0] - 1,
				     NULL, 0);
		wids = ngram_iter_get(itor, &score, &bowt);
		printf("%.4f %s %.4f\n",
		       logmath_log_to_log10(lmath, score),
		       ngram_word(model, wids[0]),
		       logmath_log_to_log10(lmath, bowt));
		TEST_EQUAL(wids[0], ngram_wid(model, "~"));

		for (itor2 = ngram_iter_successors(itor);
		     itor2; itor2 = ngram_iter_next(itor2)) {
			wids = ngram_iter_get(itor2, &score, &bowt);
			printf("%.4f %s %s %.4f\n",
			       logmath_log_to_log10(lmath, score),
			       ngram_word(model, wids[0]),
			       ngram_word(model, wids[1]),
			       logmath_log_to_log10(lmath, bowt));
			TEST_EQUAL(wids[0], ngram_wid(model, "~"));
			TEST_EQUAL(wids[1], ngram_wid(model, "eleven"));
		}
		itor2 = ngram_iter_successors(itor);
		for (itor3 = ngram_iter_successors(itor2);
		     itor3; itor3 = ngram_iter_next(itor3)) {
			wids = ngram_iter_get(itor3, &score, &bowt);
			printf("%.4f %s %s %s\n",
			       logmath_log_to_log10(lmath, score),
			       ngram_word(model, wids[0]),
			       ngram_word(model, wids[1]),
			       ngram_word(model, wids[2]));
			TEST_EQUAL(wids[0], ngram_wid(model, "~"));
			TEST_EQUAL(wids[1], ngram_wid(model, "eleven"));
			TEST_EQUAL(wids[2], ngram_wid(model, "per"));
		}
		ngram_iter_free(itor2);
		ngram_iter_free(itor);
	}
	
	TEST_EQUAL(0, ngram_model_free(model));
	logmath_free(lmath);

	return 0;
}
