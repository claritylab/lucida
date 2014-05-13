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
	int32 wid, score;

	wid = ngram_model_add_word(model, "foobie", 1.0);
	score = ngram_score(model, "foobie", NULL);
	TEST_EQUAL_LOG(score, logmath_log(lmath, 1.0/400.0)); /* #unigrams */

	wid = ngram_model_add_word(model, "quux", 0.5);
	score = ngram_score(model, "quux", NULL);
	TEST_EQUAL_LOG(score, logmath_log(lmath, 0.5/400.0)); /* #unigrams */

	ngram_model_apply_weights(model, 1.0, 1.0, 0.9);
	score = ngram_score(model, "quux", NULL);
	TEST_EQUAL_LOG(score, logmath_log(lmath, 0.5/400.0*0.9 + 1.0/400.0*0.1));

	wid = ngram_model_add_word(model, "bazbar", 0.5);
	score = ngram_score(model, "bazbar", NULL);
	TEST_EQUAL_LOG(score, logmath_log(lmath, 0.5/400.0*0.9 + 1.0/400.0*0.1));
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
