#include <logmath.h>

#include "test_macros.h"

#define LOG_EPSILON 1500

int
main(int argc, char *argv[])
{
	logmath_t *lmath;
	int32 rv;

	lmath = logmath_init(1.0001, 8, 1);
	TEST_ASSERT(lmath);
	printf("log(1e-150) = %d\n", logmath_log(lmath, 1e-150));
	TEST_EQUAL_LOG(logmath_log(lmath, 1e-150), -13493);
	printf("exp(log(1e-150)) = %e\n",logmath_exp(lmath, logmath_log(lmath, 1e-150)));
	TEST_EQUAL_FLOAT(logmath_exp(lmath, logmath_log(lmath, 1e-150)), 1e-150);
	printf("log(1e-48) = %d\n", logmath_log(lmath, 1e-48));
	printf("exp(log(1e-48)) = %e\n",logmath_exp(lmath, logmath_log(lmath, 1e-48)));
	TEST_EQUAL_FLOAT(logmath_exp(lmath, logmath_log(lmath, 1e-48)), 1e-48);
	printf("log(42) = %d\n", logmath_log(lmath, 42));
	TEST_EQUAL_LOG(logmath_log(lmath, 42), 146);
	printf("exp(log(42)) = %f\n",logmath_exp(lmath, logmath_log(lmath, 42)));
	TEST_EQUAL_FLOAT(logmath_exp(lmath, logmath_log(lmath, 42)), 41.99);
	printf("log(1e-3 + 5e-3) = %d l+ %d = %d\n",
	       logmath_log(lmath, 1e-3),
	       logmath_log(lmath, 5e-3),
	       logmath_add(lmath, logmath_log(lmath, 1e-3),
			   logmath_log(lmath, 5e-3)));
	printf("log(1e-3 + 5e-3) = %e + %e = %e\n",
	       logmath_exp(lmath, logmath_log(lmath, 1e-3)),
	       logmath_exp(lmath, logmath_log(lmath, 5e-3)),
	       logmath_exp(lmath, logmath_add(lmath, logmath_log(lmath, 1e-3),
					      logmath_log(lmath, 5e-3))));
	TEST_EQUAL_LOG(logmath_add(lmath, logmath_log(lmath, 1e-48),
				   logmath_log(lmath, 5e-48)),
		       logmath_log(lmath, 6e-48));
	TEST_EQUAL_LOG(logmath_add(lmath, logmath_log(lmath, 1e-48),
				   logmath_log(lmath, 42)),
		       logmath_log(lmath, 42));

	rv = logmath_write(lmath, "tmp.logadd");
	TEST_EQUAL(rv, 0);
	logmath_free(lmath);
	lmath = logmath_read("tmp.logadd");
	TEST_ASSERT(lmath);
	printf("log(1e-150) = %d\n", logmath_log(lmath, 1e-150));
	TEST_EQUAL_LOG(logmath_log(lmath, 1e-150), -13493);
	printf("exp(log(1e-150)) = %e\n",logmath_exp(lmath, logmath_log(lmath, 1e-150)));
	TEST_EQUAL_FLOAT(logmath_exp(lmath, logmath_log(lmath, 1e-150)), 1e-150);
	printf("log(1e-48) = %d\n", logmath_log(lmath, 1e-48));
	printf("exp(log(1e-48)) = %e\n",logmath_exp(lmath, logmath_log(lmath, 1e-48)));
	TEST_EQUAL_FLOAT(logmath_exp(lmath, logmath_log(lmath, 1e-48)), 1e-48);
	printf("log(42) = %d\n", logmath_log(lmath, 42));
	TEST_EQUAL_LOG(logmath_log(lmath, 42), 146);
	printf("exp(log(42)) = %f\n",logmath_exp(lmath, logmath_log(lmath, 42)));
	TEST_EQUAL_FLOAT(logmath_exp(lmath, logmath_log(lmath, 42)), 41.99);
	printf("log(1e-3 + 5e-3) = %d l+ %d = %d\n",
	       logmath_log(lmath, 1e-3),
	       logmath_log(lmath, 5e-3),
	       logmath_add(lmath, logmath_log(lmath, 1e-3),
			   logmath_log(lmath, 5e-3)));
	printf("log(1e-3 + 5e-3) = %e + %e = %e\n",
	       logmath_exp(lmath, logmath_log(lmath, 1e-3)),
	       logmath_exp(lmath, logmath_log(lmath, 5e-3)),
	       logmath_exp(lmath, logmath_add(lmath, logmath_log(lmath, 1e-3),
					      logmath_log(lmath, 5e-3))));
	TEST_EQUAL_LOG(logmath_add(lmath, logmath_log(lmath, 1e-48),
				   logmath_log(lmath, 5e-48)),
		       logmath_log(lmath, 6e-48));
	TEST_EQUAL_LOG(logmath_add(lmath, logmath_log(lmath, 1e-48),
				   logmath_log(lmath, 42)),
		       logmath_log(lmath, 42));

	return 0;
}
