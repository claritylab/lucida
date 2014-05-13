#include <stdio.h>
#include <string.h>

#include "matrix.h"
#include "ckd_alloc.h"

const float32 foo[3][3] = {
	{2, 0.42, 1},
	{0.42, 2, -0.3},
	{1, -0.3, 2}
};
const float32 bar[3][3] = {
	{1, 0, 1},
	{0, 1, 0},
	{0, 0, 1}
};

int
main(int argc, char *argv[])
{
	float32 **a;

	a = (float32 **)ckd_calloc_2d(3, 3, sizeof(float32));

	memcpy(a[0], foo, sizeof(float32) * 3 * 3);
	/* Should see 5.22 */
	printf("%.2f\n", determinant(a, 3));

	/* Should see -1.0 */
	memcpy(a[0], bar, sizeof(float32) * 3 * 3);
	printf("%.2f\n", determinant(a, 3));

	ckd_free_2d((void **)a);

	return 0;
}
