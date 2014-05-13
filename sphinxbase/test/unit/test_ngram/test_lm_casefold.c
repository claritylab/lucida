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

	ngram_model_casefold(model, NGRAM_UPPER);

	TEST_EQUAL(0, strcmp("</s>", ngram_word(model, 5)));
	TEST_EQUAL(0, strcmp("BE", ngram_word(model, 42)));
	TEST_EQUAL(0, strcmp("FLOORED", ngram_word(model, 130)));
	TEST_EQUAL(0, strcmp("ZERO", ngram_word(model, 398)));
	TEST_EQUAL(0, strcmp("~", ngram_word(model, 399)));

	ngram_model_casefold(model, NGRAM_LOWER);

	TEST_EQUAL(0, strcmp("</s>", ngram_word(model, 5)));
	TEST_EQUAL(0, strcmp("be", ngram_word(model, 42)));
	TEST_EQUAL(0, strcmp("floored", ngram_word(model, 130)));
	TEST_EQUAL(0, strcmp("zero", ngram_word(model, 398)));
	TEST_EQUAL(0, strcmp("~", ngram_word(model, 399)));

	ngram_model_free(model);
	logmath_free(lmath);

	return 0;
}
