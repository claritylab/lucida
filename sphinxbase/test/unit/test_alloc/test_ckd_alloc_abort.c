#include <stdio.h>

#include <ckd_alloc.h>

#include "test_macros.h"

int
main(int argc, char *argv[])
{
	int *alloc1;
	int bad_alloc_did_not_fail = FALSE;

	ckd_set_jump(NULL, TRUE);
	/* Guaranteed to fail, we hope!. */
	alloc1 = ckd_calloc(-1,-1);
	TEST_ASSERT(bad_alloc_did_not_fail);

	return 0;
}
