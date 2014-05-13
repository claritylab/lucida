#include <ngram_model.h>
#include <logmath.h>
#include <strfuncs.h>

#include "test_macros.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

void
run_tests(logmath_t *lmath, ngram_model_t *model)
{
	int32 rv, i;

	TEST_ASSERT(model);

	TEST_EQUAL(ngram_wid(model, "scylla"), 285);
	TEST_EQUAL(strcmp(ngram_word(model, 285), "scylla"), 0);

	rv = ngram_model_read_classdef(model, LMDIR "/100.probdef");
	TEST_EQUAL(rv, 0);

	/* Verify that class word IDs remain the same. */
	TEST_EQUAL(ngram_wid(model, "scylla"), 285);
	TEST_EQUAL(strcmp(ngram_word(model, 285), "scylla"), 0);

	/* Verify in-class word IDs. */
	TEST_EQUAL(ngram_wid(model, "scylla:scylla"), 0x80000000 | 400);

	/* Verify in-class and out-class unigram scores. */
	TEST_EQUAL_LOG(ngram_score(model, "scylla:scylla", NULL),
		       logmath_log10_to_log(lmath, -2.7884) + logmath_log(lmath, 0.4));
	TEST_EQUAL_LOG(ngram_score(model, "scooby:scylla", NULL),
		       logmath_log10_to_log(lmath, -2.7884) + logmath_log(lmath, 0.1));
	TEST_EQUAL_LOG(ngram_score(model, "scylla", NULL),
		       logmath_log10_to_log(lmath, -2.7884));
	TEST_EQUAL_LOG(ngram_score(model, "oh:zero", NULL),
		       logmath_log10_to_log(lmath, -1.9038) + logmath_log(lmath, 0.7));
	TEST_EQUAL_LOG(ngram_score(model, "zero", NULL),
		       logmath_log10_to_log(lmath, -1.9038));

	/* Verify class bigram scores. */
	TEST_EQUAL_LOG(ngram_score(model, "scylla", "on", NULL),
		       logmath_log10_to_log(lmath, -1.2642));
	TEST_EQUAL_LOG(ngram_score(model, "scylla:scylla", "on", NULL),
		       logmath_log10_to_log(lmath, -1.2642) + logmath_log(lmath, 0.4));
	TEST_EQUAL_LOG(ngram_score(model, "apparently", "scylla", NULL),
		       logmath_log10_to_log(lmath, -0.5172));
	TEST_EQUAL_LOG(ngram_score(model, "apparently", "karybdis:scylla", NULL),
		       logmath_log10_to_log(lmath, -0.5172));
	TEST_EQUAL_LOG(ngram_score(model, "apparently", "scooby:scylla", NULL),
		       logmath_log10_to_log(lmath, -0.5172));

	/* Verify class trigram scores. */
	TEST_EQUAL_LOG(ngram_score(model, "zero", "be", "will", NULL),
		       logmath_log10_to_log(lmath, -0.5725));
	TEST_EQUAL_LOG(ngram_score(model, "oh:zero", "be", "will", NULL),
		       logmath_log10_to_log(lmath, -0.5725) + logmath_log(lmath, 0.7));
	TEST_EQUAL_LOG(ngram_score(model, "should", "variance", "zero", NULL),
		       logmath_log10_to_log(lmath, -0.9404));
	TEST_EQUAL_LOG(ngram_score(model, "should", "variance", "zero:zero", NULL),
		       logmath_log10_to_log(lmath, -0.9404));

	/* Add words to classes. */
	rv = ngram_model_add_class_word(model, "scylla", "scrappy:scylla", 1.0);
	TEST_ASSERT(rv >= 0);
	TEST_EQUAL(ngram_wid(model, "scrappy:scylla"), 0x80000196);
	TEST_EQUAL_LOG(ngram_score(model, "scrappy:scylla", NULL),
		       logmath_log10_to_log(lmath, -2.7884) + logmath_log(lmath, 0.2));
	printf("scrappy:scylla %08x %d %f\n", 
	       ngram_wid(model, "scrappy:scylla"),
	       ngram_score(model, "scrappy:scylla", NULL),
	       logmath_exp(lmath, ngram_score(model, "scrappy:scylla", NULL)));
	/* Add a lot of words to a class. */
	for (i = 0; i < 129; ++i) {
		char word[32];
		sprintf(word, "%d:scylla", i);
		rv = ngram_model_add_class_word(model, "scylla", word, 1.0);
		printf("%s %08x %d %f\n", word,
		       ngram_wid(model, word),
		       ngram_score(model, word, NULL),
		       logmath_exp(lmath, ngram_score(model, word, NULL)));
		TEST_ASSERT(rv >= 0);
		TEST_EQUAL(ngram_wid(model, word), 0x80000197 + i);
	}

	/* Add a new class. */
	{
		const char *words[] = { "blatz:foobie", "hurf:foobie" };
		float32 weights[] = { 0.6, 0.4 };
		int32 foobie_prob;
		rv = ngram_model_add_class(model, "[foobie]", 1.0,
					   words, weights, 2);
		TEST_ASSERT(rv >= 0);
		foobie_prob = ngram_score(model, "[foobie]", NULL);
		TEST_EQUAL_LOG(ngram_score(model, "blatz:foobie", NULL),
			       foobie_prob + logmath_log(lmath, 0.6));
		TEST_EQUAL_LOG(ngram_score(model, "hurf:foobie", NULL),
			       foobie_prob + logmath_log(lmath, 0.4));
	}
}

int
main(int argc, char *argv[])
{
	logmath_t *lmath;
	ngram_model_t *model;

	lmath = logmath_init(1.0001, 0, 0);

	model = ngram_model_read(NULL, LMDIR "/100.arpa.DMP", NGRAM_DMP, lmath);
	run_tests(lmath, model);
	ngram_model_free(model);

	model = ngram_model_read(NULL, LMDIR "/100.arpa.gz", NGRAM_ARPA, lmath);
	run_tests(lmath, model);
	ngram_model_free(model);

	logmath_free(lmath);

	return 0;
}
