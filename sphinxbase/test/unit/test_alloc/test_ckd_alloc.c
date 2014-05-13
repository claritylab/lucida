#include <stdio.h>

#include <ckd_alloc.h>

#include "test_macros.h"

int
main(int argc, char *argv[])
{
	int *alloc1;
	int **alloc2;
	int ***alloc3;
	int i;

	TEST_ASSERT(alloc1 = ckd_calloc(3*3*3, sizeof(*alloc1)));
	TEST_ASSERT(alloc2 = ckd_calloc_2d(3, 3, sizeof(**alloc2)));
	TEST_ASSERT(alloc3 = ckd_calloc_3d(3, 3, 3, sizeof(***alloc3)));

	for (i = 0; i < 27; ++i) {
		TEST_EQUAL(alloc1[i], 0);
		alloc1[i] = i + 1;
	}
	for (i = 0; i < 27; ++i)
		TEST_EQUAL(alloc1[i], i+1);

	for (i = 0; i < 3; ++i) {
		int j;
		for (j = 0; j < 3; ++j) {
			TEST_EQUAL(alloc2[i][j], 0);
			alloc2[i][j] = i * 3 + j + 1;
		}
	}
	/* Verify that row-major ordering is in use. */
	for (i = 0; i < 9; ++i) {
		TEST_EQUAL(alloc2[0][i], i+1);
		TEST_EQUAL(alloc2[0][i], alloc1[i]);
	}
	for (i = 0; i < 3; ++i) {
		int j;
		for (j = 0; j < 3; ++j) {
			TEST_EQUAL(alloc2[i][j], i * 3 + j + 1);
		}
	}
	/* Now test alloc_ptr. */
	ckd_free_2d(alloc2);
	alloc2 = ckd_alloc_2d_ptr(3, 3, alloc1, sizeof(*alloc1));
	for (i = 0; i < 9; ++i) {
		TEST_EQUAL(alloc2[0][i], i+1);
		TEST_EQUAL(alloc2[0][i], alloc1[i]);
	}
	for (i = 0; i < 3; ++i) {
		int j;
		for (j = 0; j < 3; ++j) {
			TEST_EQUAL(alloc2[i][j], i * 3 + j + 1);
		}
	}
	ckd_free_2d_ptr(alloc2);

	for (i = 0; i < 3; ++i) {
		int j;
		for (j = 0; j < 3; ++j) {
			int k;
			for (k = 0; k < 3; ++k) {
				TEST_EQUAL(alloc3[i][j][k], 0);
				alloc3[i][j][k] = i * 3 * 3 + j * 3 + k + 1;
			}
		}
	}
	/* Verify that row-major ordering is in use. */
	for (i = 0; i < 27; ++i) {
		TEST_EQUAL(alloc3[0][0][i], i+1);
		TEST_EQUAL(alloc3[0][0][i], alloc1[i]);
	}
	for (i = 0; i < 3; ++i) {
		int j;
		for (j = 0; j < 3; ++j) {
			int k;
			for (k = 0; k < 3; ++k) {
				TEST_EQUAL(alloc3[i][j][k], i * 3 * 3 + j * 3 + k + 1);
			}
		}
	}
	/* Now test alloc_ptr. */
	ckd_free_3d(alloc3);
	alloc3 = ckd_alloc_3d_ptr(3, 3, 3, alloc1, sizeof(*alloc1));
	for (i = 0; i < 27; ++i) {
		TEST_EQUAL(alloc3[0][0][i], i+1);
		TEST_EQUAL(alloc3[0][0][i], alloc1[i]);
	}
	for (i = 0; i < 3; ++i) {
		int j;
		for (j = 0; j < 3; ++j) {
			int k;
			for (k = 0; k < 3; ++k) {
				TEST_EQUAL(alloc3[i][j][k], i * 3 * 3 + j * 3 + k + 1);
			}
		}
	}
	ckd_free_3d_ptr(alloc3);
	ckd_free(alloc1);

	return 0;
}
