#include <stdio.h>
#include <time.h>

#include "bitvec.h"
#include "test_macros.h"

int
main(int argc, char *argv[])
{
	bitvec_t *bv;
	int i, j;
	clock_t c;

	TEST_ASSERT(bv = bitvec_alloc(199));
	bitvec_set(bv,198);
	bitvec_set(bv,0);
	bitvec_set(bv,42);
	bitvec_set(bv,43);
	bitvec_set(bv,44);
	TEST_ASSERT(bitvec_is_set(bv,198));
	TEST_ASSERT(bitvec_is_set(bv,0));
	TEST_ASSERT(bitvec_is_set(bv,42));
	TEST_ASSERT(bitvec_is_set(bv,43));
	TEST_ASSERT(bitvec_is_set(bv,44));
	TEST_EQUAL(5, bitvec_count_set(bv, 199));
	bitvec_clear(bv, 43);
	TEST_EQUAL(0, bitvec_is_set(bv,43));

	c = clock();
	for (j = 0; j < 1000000; ++j)
		bitvec_count_set(bv, 199);
	c = clock() - c;
	printf("1000000 * 199 bitvec_count_set in %.2f sec\n",
	       (double)c / CLOCKS_PER_SEC);
	bitvec_free(bv);

	bv = bitvec_alloc(1314);
	c = clock();
	for (j = 0; j < 50000; ++j)
		for (i = 0; i < 1314; ++i)
			bitvec_set(bv, i);
	c = clock() - c;
	printf("50000 * 1314 bitvec_set in %.2f sec\n",
	       (double)c / CLOCKS_PER_SEC);
	bitvec_free(bv);

	/* Test realloc */
	bv = bitvec_alloc(13);
	for (i = 1; i < 13; i+=2)
	    bitvec_set(bv, i);
	printf("Bits set %d\n", bitvec_count_set(bv, 13));
        TEST_EQUAL(6, bitvec_count_set(bv, 13));
	bv = bitvec_realloc(bv, 13, 2000);
	for (i = 0; i < 2000; i++) {
          /* printf("%d %d\n", i, bitvec_is_set(bv, i) != 0); */
	}
	printf("Bits set after realloc %d\n", bitvec_count_set(bv, 2000));
        TEST_EQUAL(6, bitvec_count_set(bv, 2000));
	bitvec_free(bv);

	return 0;
}
