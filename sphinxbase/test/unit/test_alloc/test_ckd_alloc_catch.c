#include <stdio.h>

#include <ckd_alloc.h>

#include "test_macros.h"

int
main(int argc, char *argv[])
{
	int *alloc1;
	jmp_buf env;

	ckd_set_jump(&env, FALSE);
	if (setjmp(env)) {
		printf("Successfully caught bad allocation!\n");
	}
	else {
		int failed_to_catch_bad_alloc = FALSE;

		/* Guaranteed to fail, we hope!. */
		alloc1 = ckd_calloc(-1,-1);
		TEST_ASSERT(failed_to_catch_bad_alloc);
	}

	return 0;
}
