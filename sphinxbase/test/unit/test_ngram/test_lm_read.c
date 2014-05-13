#include <ngram_model.h>
#include <logmath.h>
#include <strfuncs.h>

#include "test_macros.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static int
test_lm_vals(ngram_model_t *model)
{
	int32 n_used;
	TEST_ASSERT(model);
	TEST_EQUAL(ngram_wid(model, "<UNK>"), 0);
	TEST_EQUAL(strcmp(ngram_word(model, 0), "<UNK>"), 0);
	TEST_EQUAL(ngram_wid(model, "absolute"), 13);
	TEST_EQUAL(strcmp(ngram_word(model, 13), "absolute"), 0);
	/* Test unigrams. */
	TEST_EQUAL(ngram_score(model, "<UNK>", NULL), -75346);
	TEST_EQUAL(ngram_bg_score(model, ngram_wid(model, "<UNK>"),
				  NGRAM_INVALID_WID, &n_used), -75346);
	TEST_EQUAL(n_used, 1);
	TEST_EQUAL(ngram_score(model, "sphinxtrain", NULL), -64208);
	TEST_EQUAL(ngram_bg_score(model, ngram_wid(model, "sphinxtrain"),
				  NGRAM_INVALID_WID, &n_used), -64208);
	TEST_EQUAL(n_used, 1);
	/* Test bigrams. */
	TEST_EQUAL(ngram_score(model, "huggins", "david", NULL), -831);
	/* Test trigrams. */
	TEST_EQUAL_LOG(ngram_score(model, "daines", "huggins", "david", NULL), -9450);
	return 0;
}

static int
test_lm_ug_vals(ngram_model_t *model)
{
	TEST_ASSERT(model);
	TEST_EQUAL(ngram_score(model, "BACKWARD", NULL), -53008);
	return 0;
}

int
main(int argc, char *argv[])
{
	logmath_t *lmath;
	ngram_model_t *model;

	/* Initialize a logmath object to pass to ngram_read */
	lmath = logmath_init(1.0001, 0, 0);
	/* Read a language model */
	model = ngram_model_read(NULL, LMDIR "/100.arpa.bz2", NGRAM_ARPA, lmath);
	test_lm_vals(model);
	TEST_EQUAL(0, ngram_model_free(model));

	/* Read a language model */
	model = ngram_model_read(NULL, LMDIR "/100.arpa.DMP", NGRAM_DMP, lmath);
	test_lm_vals(model);

	/* Test refcounting. */
	model = ngram_model_retain(model);
	TEST_EQUAL(1, ngram_model_free(model));
	TEST_EQUAL(ngram_score(model, "daines", "huggins", "david", NULL), -9452);
	TEST_EQUAL(0, ngram_model_free(model));

	/* Read a language model */
	model = ngram_model_read(NULL, LMDIR "/turtle.ug.lm", NGRAM_ARPA, lmath);
	test_lm_ug_vals(model);

	/* Read a language model */
	model = ngram_model_read(NULL, LMDIR "/turtle.ug.lm.DMP", NGRAM_DMP, lmath);
	test_lm_ug_vals(model);

	logmath_free(lmath);

	return 0;
}
