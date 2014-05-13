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
	ngram_model_t *lms[3];
	ngram_model_t *lmset;
	const char *names[] = { "100", "100_2" };
	const char *words[] = {
		"<UNK>",
		"ROBOMAN",
		"libio",
		"sphinxtrain",
		"bigbird",
		"quuxfuzz"
	};
	const int32 n_words = sizeof(words) / sizeof(words[0]);
	float32 weights[] = { 0.6, 0.4 };

	lmath = logmath_init(1.0001, 0, 0);

	lms[0] = ngram_model_read(NULL, LMDIR "/100.arpa.DMP", NGRAM_DMP, lmath);
	lms[1] = ngram_model_read(NULL, LMDIR "/100_2.arpa.DMP", NGRAM_DMP, lmath);

	lmset = ngram_model_set_init(NULL, lms, (char **)names, NULL, 2);
	TEST_ASSERT(lmset);
	TEST_EQUAL(ngram_model_set_select(lmset, "100_2"), lms[1]);
	TEST_EQUAL(ngram_model_set_select(lmset, "100"), lms[0]);
	TEST_EQUAL(ngram_score(lmset, "sphinxtrain", NULL),
		   logmath_log10_to_log(lmath, -2.7884));
	TEST_EQUAL(ngram_score(lmset, "huggins", "david", NULL),
		   logmath_log10_to_log(lmath, -0.0361));
	TEST_EQUAL_LOG(ngram_score(lmset, "daines", "huggins", "david", NULL),
		       logmath_log10_to_log(lmath, -0.4105));

	TEST_EQUAL(ngram_model_set_select(lmset, "100_2"), lms[1]);
	TEST_EQUAL(ngram_score(lmset, "sphinxtrain", NULL),
		   logmath_log10_to_log(lmath, -2.8192));
	TEST_EQUAL(ngram_score(lmset, "huggins", "david", NULL),
		   logmath_log10_to_log(lmath, -0.1597));
	TEST_EQUAL_LOG(ngram_score(lmset, "daines", "huggins", "david", NULL),
		       logmath_log10_to_log(lmath, -0.0512));

	/* Test interpolation with default weights. */
	TEST_ASSERT(ngram_model_set_interp(lmset, NULL, NULL));
	TEST_EQUAL_LOG(ngram_score(lmset, "sphinxtrain", NULL),
		       logmath_log(lmath,
				   0.5 * pow(10, -2.7884)
				   + 0.5 * pow(10, -2.8192)));

	/* Test interpolation with set weights. */
	TEST_ASSERT(ngram_model_set_interp(lmset, names, weights));
	TEST_EQUAL_LOG(ngram_score(lmset, "sphinxtrain", NULL),
		       logmath_log(lmath,
				   0.6 * pow(10, -2.7884)
				   + 0.4 * pow(10, -2.8192)));

	/* Test switching back to selected mode. */
	TEST_EQUAL(ngram_model_set_select(lmset, "100_2"), lms[1]);
	TEST_EQUAL(ngram_score(lmset, "sphinxtrain", NULL),
		   logmath_log10_to_log(lmath, -2.8192));
	TEST_EQUAL(ngram_score(lmset, "huggins", "david", NULL),
		   logmath_log10_to_log(lmath, -0.1597));
	TEST_EQUAL_LOG(ngram_score(lmset, "daines", "huggins", "david", NULL),
		       logmath_log10_to_log(lmath, -0.0512));

	/* Test interpolation with previously set weights. */
	TEST_ASSERT(ngram_model_set_interp(lmset, NULL, NULL));
	TEST_EQUAL_LOG(ngram_score(lmset, "sphinxtrain", NULL),
		       logmath_log(lmath,
				   0.6 * pow(10, -2.7884)
				   + 0.4 * pow(10, -2.8192)));

	/* Test interpolation with closed-vocabulary models and OOVs. */
	lms[2] = ngram_model_read(NULL, LMDIR "/turtle.lm", NGRAM_ARPA, lmath);
	TEST_ASSERT(ngram_model_set_add(lmset, lms[2], "turtle", 1.0, FALSE));
	TEST_EQUAL_LOG(ngram_score(lmset, "sphinxtrain", NULL),
		       logmath_log(lmath,
				   0.6 * (2.0 / 3.0) * pow(10, -2.7884)
				   + 0.4 * (2.0 / 3.0) * pow(10, -2.8192)));
	ngram_model_free(lmset);

	/* Test adding and removing language models with preserved
	 * word ID mappings. */
	lms[0] = ngram_model_read(NULL, LMDIR "/100.arpa.DMP", NGRAM_DMP, lmath);
	lms[1] = ngram_model_read(NULL, LMDIR "/100_2.arpa.DMP", NGRAM_DMP, lmath);
	lms[2] = ngram_model_read(NULL, LMDIR "/turtle.lm", NGRAM_ARPA, lmath);
	lmset = ngram_model_set_init(NULL, lms, (char **)names, NULL, 1);
	{
		int32 wid;
		wid = ngram_wid(lmset, "sphinxtrain");
		TEST_ASSERT(ngram_model_set_add(lmset, lms[1], "100_2", 1.0, TRUE));
		/* Verify that it is the same. */
		TEST_EQUAL(wid, ngram_wid(lmset, "sphinxtrain"));
		/* Now add another model and verify that its words
		 * don't actually get added. */
		TEST_ASSERT(ngram_model_set_add(lmset, lms[2], "turtle", 1.0, TRUE));
		TEST_EQUAL(wid, ngram_wid(lmset, "sphinxtrain"));
		TEST_EQUAL(ngram_unknown_wid(lmset), ngram_wid(lmset, "FORWARD"));
		/* Remove language model, make sure this doesn't break horribly. */
		TEST_EQUAL(lms[1], ngram_model_set_remove(lmset, "100_2", TRUE));
		ngram_model_free(lms[1]);
		TEST_EQUAL(wid, ngram_wid(lmset, "sphinxtrain"));
		/* Now enable remapping of word IDs and verify that it works. */
		TEST_EQUAL(lms[2], ngram_model_set_remove(lmset, "turtle", TRUE));
		TEST_ASSERT(ngram_model_set_add(lmset, lms[2], "turtle", 1.0, FALSE));
		printf("FORWARD = %d\n", ngram_wid(lmset, "FORWARD"));
	}

	ngram_model_free(lmset);

	/* Now test lmctl files. */
	lmset = ngram_model_set_read(NULL, LMDIR "/100.lmctl", lmath);
	TEST_ASSERT(lmset);
	/* Test iterators. */
	{
		ngram_model_set_iter_t *itor;
		ngram_model_t *lm;
		char const *lmname;

		itor = ngram_model_set_iter(lmset);
		TEST_ASSERT(itor);
		lm = ngram_model_set_iter_model(itor, &lmname);
		printf("1: %s\n", lmname);
		itor = ngram_model_set_iter_next(itor);
		lm = ngram_model_set_iter_model(itor, &lmname);
		printf("2: %s\n", lmname);
		itor = ngram_model_set_iter_next(itor);
		lm = ngram_model_set_iter_model(itor, &lmname);
		printf("3: %s\n", lmname);
		itor = ngram_model_set_iter_next(itor);
		TEST_EQUAL(itor, NULL);
	}

	TEST_EQUAL(ngram_score(lmset, "sphinxtrain", NULL),
		   logmath_log10_to_log(lmath, -2.7884));

	TEST_ASSERT(ngram_model_set_interp(lmset, NULL, NULL));
	TEST_EQUAL_LOG(ngram_score(lmset, "sphinxtrain", NULL),
		       logmath_log(lmath,
				   (1.0 / 3.0) * pow(10, -2.7884)
				   + (1.0 / 3.0) * pow(10, -2.8192)));

	ngram_model_set_select(lmset, "100_2");
	TEST_EQUAL(ngram_score(lmset, "sphinxtrain", NULL),
		   logmath_log10_to_log(lmath, -2.8192));
	TEST_EQUAL(ngram_score(lmset, "huggins", "david", NULL),
		   logmath_log10_to_log(lmath, -0.1597));
	TEST_EQUAL_LOG(ngram_score(lmset, "daines", "huggins", "david", NULL),
		       logmath_log10_to_log(lmath, -0.0512));

	ngram_model_set_select(lmset, "100");
	TEST_EQUAL(ngram_score(lmset, "sphinxtrain", NULL),
		   logmath_log10_to_log(lmath, -2.7884));
	TEST_EQUAL(ngram_score(lmset, "huggins", "david", NULL),
		   logmath_log10_to_log(lmath, -0.0361));
	TEST_EQUAL_LOG(ngram_score(lmset, "daines", "huggins", "david", NULL),
		       logmath_log10_to_log(lmath, -0.4105));

	/* Test class probabilities. */
	ngram_model_set_select(lmset, "100");
	TEST_EQUAL_LOG(ngram_score(lmset, "scylla:scylla", NULL),
		       logmath_log10_to_log(lmath, -2.7884) + logmath_log(lmath, 0.4));
	TEST_EQUAL_LOG(ngram_score(lmset, "scooby:scylla", NULL),
		       logmath_log10_to_log(lmath, -2.7884) + logmath_log(lmath, 0.1));
	TEST_EQUAL_LOG(ngram_score(lmset, "apparently", "karybdis:scylla", NULL),
		       logmath_log10_to_log(lmath, -0.5172));

	/* Test word ID mapping. */
	ngram_model_set_select(lmset, "turtle");
	TEST_EQUAL(ngram_wid(lmset, "ROBOMAN"),
		   ngram_wid(lmset, ngram_word(lmset, ngram_wid(lmset, "ROBOMAN"))));
	TEST_EQUAL(ngram_wid(lmset, "bigbird"),
		   ngram_wid(lmset, ngram_word(lmset, ngram_wid(lmset, "bigbird"))));
	TEST_EQUAL(ngram_wid(lmset, "quuxfuzz"), ngram_unknown_wid(lmset));
	TEST_EQUAL(ngram_score(lmset, "quuxfuzz", NULL), ngram_zero(lmset));
	ngram_model_set_map_words(lmset, words, n_words);
	TEST_EQUAL(ngram_wid(lmset, "ROBOMAN"),
		   ngram_wid(lmset, ngram_word(lmset, ngram_wid(lmset, "ROBOMAN"))));
	TEST_EQUAL(ngram_wid(lmset, "bigbird"),
		   ngram_wid(lmset, ngram_word(lmset, ngram_wid(lmset, "bigbird"))));
	TEST_EQUAL(ngram_wid(lmset, "quuxfuzz"), 5);
	TEST_EQUAL(ngram_score(lmset, "quuxfuzz", NULL), ngram_zero(lmset));

	ngram_model_free(lmset);
	logmath_free(lmath);
	return 0;
}
