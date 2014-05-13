#include <stdio.h>
#include <string.h>

#include "matrix.h"
#include "ckd_alloc.h"

const float32 foo[3][3] = {
	{2, 1, 1},
	{1, 2, 1},
	{1, 1, 2}
};
const float32 bar[3][3] = {
	{2, 0.5, 1},
	{0.5, 2, 1},
	{1, 1, 2}
};

int
main(int argc, char *argv[])
{
	float32 **a, **ainv, **ii;
	int i, j;

	a = (float32 **)ckd_calloc_2d(3, 3, sizeof(float32));
	ainv = (float32 **)ckd_calloc_2d(3, 3, sizeof(float32));
	ii = (float32 **)ckd_calloc_2d(3, 3, sizeof(float32));

	memcpy(a[0], foo, sizeof(float32) * 3 * 3);
	printf("%d\n", invert(ainv, a, 3));
	/* Should see:
	   0.75 -0.25 -0.25 
	   -0.25 0.75 -0.25 
	   -0.25 -0.25 0.75 
	*/
	for (i = 0; i < 3; ++i) {
		for (j = 0; j < 3; ++j) {
			printf("%.2f ", ainv[i][j]);
		}
		printf("\n");
	}
	/* Should see:
	   1.00 0.00 0.00
	   0.00 1.00 0.00
	   0.00 0.00 1.00
	*/
	matrixmultiply(ii, ainv, a, 3);
	for (i = 0; i < 3; ++i) {
		for (j = 0; j < 3; ++j) {
			printf("%.2f ", ii[i][j]);
		}
		printf("\n");
	}

	memcpy(a[0], bar, sizeof(float32) * 3 * 3);
	printf("%d\n", invert(ainv, a, 3));
	/* Should see:
	*/
	for (i = 0; i < 3; ++i) {
		for (j = 0; j < 3; ++j) {
			printf("%.2f ", ainv[i][j]);
		}
		printf("\n");
	}
	/* Should see:
	   1.00 0.00 0.00 
	   0.00 1.00 0.00 
	   0.00 0.00 1.00 
	*/
	memset(ii[0], 0, sizeof(float32) * 3 * 3);
	matrixmultiply(ii, ainv, a, 3);
	for (i = 0; i < 3; ++i) {
		for (j = 0; j < 3; ++j) {
			printf("%.2f ", ii[i][j]);
		}
		printf("\n");
	}

	/* Should see:
	   -1
	*/
	a[0][0] = 1.0;
	printf("%d\n", invert(ainv, a, 3));

	ckd_free_2d((void **)a);
	ckd_free_2d((void **)ainv);
	ckd_free_2d((void **)ii);

	return 0;
}
