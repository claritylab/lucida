#include <ngram_model.h>
#include <logmath.h>
#include <strfuncs.h>

#include "test_macros.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

int
main(int argc, char *argv[])
{
	logmath_t *lmath;
	ngram_model_t *model;

	/* Initialize a logmath object to pass to ngram_read */
	lmath = logmath_init(1.0001, 0, 0);

	/* Read a language model */
	model = ngram_model_read(NULL, LMDIR "/100.arpa.DMP", NGRAM_DMP, lmath);
	TEST_ASSERT(model);

	ngram_model_recode(model, "iso8859-1", "utf-8");
	TEST_EQUAL(strcmp(ngram_word(model, 0), "<UNK>"), 0);
	ngram_model_free(model);
	logmath_free(lmath);

	return 0;
}
