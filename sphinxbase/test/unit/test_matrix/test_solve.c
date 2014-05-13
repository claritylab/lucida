#include <stdio.h>
#include <string.h>

#include "matrix.h"
#include "ckd_alloc.h"

const float32 foo[3][3] = {
	{2, 1, 1},
	{1, 2, 1},
	{1, 1, 2}
};
float32 bar[3] = {1, 3, 1};

int
main(int argc, char *argv[])
{
	float32 **a, *x;
	int i;

	a = (float32 **)ckd_calloc_2d(3, 3, sizeof(float32));
	memcpy(a[0], foo, sizeof(float32) * 3 * 3);
	x = ckd_calloc(3, sizeof(float32));

	/* Should see:
	   -0.25 1.75 -0.25
	*/
	solve(a, bar, x, 3);
	for (i = 0; i < 3; ++i)
		printf("%.2f ", x[i]);
	printf("\n");

	ckd_free_2d((void **)a);
	ckd_free(x);

	return 0;
}
