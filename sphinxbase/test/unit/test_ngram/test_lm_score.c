#include <ngram_model.h>
#include <logmath.h>
#include <strfuncs.h>

#include "test_macros.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

void
run_tests(ngram_model_t *model)
{
	int32 n_used;

	ngram_tg_score(model,
		       ngram_wid(model, "daines"),
		       ngram_wid(model, "huggins"),
		       ngram_wid(model, "huggins"), &n_used);
	TEST_EQUAL(n_used, 2);
	ngram_tg_score(model,
		       ngram_wid(model, "david"),
		       ngram_wid(model, "david"),
		       ngram_wid(model, "david"), &n_used);
	TEST_EQUAL(n_used, 1);

	/* Apply weights. */
	ngram_model_apply_weights(model, 7.5, 0.5, 1.0);
	/* -9452 * 7.5 + log(0.5) = -77821 */
	TEST_EQUAL_LOG(ngram_score(model, "daines", "huggins", "david", NULL),
		   -77821);
	/* Recover original score. */
	TEST_EQUAL_LOG(ngram_probv(model, "daines", "huggins", "david", NULL),
		   -9452);
	TEST_EQUAL_LOG(ngram_probv(model, "huggins", "david", NULL), -831);

	/* Un-apply weights. */
	ngram_model_apply_weights(model, 1.0, 1.0, 1.0);
	TEST_EQUAL_LOG(ngram_score(model, "daines", "huggins", "david", NULL),
		       -9452);
	TEST_EQUAL_LOG(ngram_score(model, "huggins", "david", NULL), -831);
	/* Recover original score. */
	TEST_EQUAL_LOG(ngram_probv(model, "daines", "huggins", "david", NULL),
		       -9452);

	/* Pre-weighting, this should give the "raw" score. */
	TEST_EQUAL_LOG(ngram_score(model, "daines", "huggins", "david", NULL),
		       -9452);
	TEST_EQUAL_LOG(ngram_score(model, "huggins", "david", NULL), -831);
	/* Verify that backoff mode calculations work. */
	ngram_bg_score(model,
		       ngram_wid(model, "huggins"),
		       ngram_wid(model, "david"), &n_used);
	TEST_EQUAL(n_used, 2);
	ngram_bg_score(model,
		       ngram_wid(model, "blorglehurfle"),
		       ngram_wid(model, "david"), &n_used);
	TEST_EQUAL(n_used, 1);
	ngram_bg_score(model,
		       ngram_wid(model, "david"),
		       ngram_wid(model, "david"), &n_used);
	TEST_EQUAL(n_used, 1);
	ngram_tg_score(model,
		       ngram_wid(model, "daines"),
		       ngram_wid(model, "huggins"),
		       ngram_wid(model, "david"), &n_used);
	TEST_EQUAL(n_used, 3);
}

int
main(int argc, char *argv[])
{
	logmath_t *lmath;
	ngram_model_t *model;

	lmath = logmath_init(1.0001, 0, 0);

	model = ngram_model_read(NULL, LMDIR "/100.arpa.DMP", NGRAM_DMP, lmath);
	run_tests(model);
	ngram_model_free(model);

	model = ngram_model_read(NULL, LMDIR "/100.arpa.gz", NGRAM_ARPA, lmath);
	run_tests(model);
	ngram_model_free(model);

	logmath_free(lmath);
	return 0;
}
