#include <stdio.h>
#include <math.h>
#include <string.h>

#define TEST_ASSERT(x) if (!(x)) { fprintf(stderr, "FAIL: %s\n", #x); exit(1); }
#define EPSILON 0.01
#define TEST_EQUAL(a,b) TEST_ASSERT((a) == (b))
#define TEST_EQUAL_FLOAT(a,b) TEST_ASSERT(fabs((a) - (b)) < EPSILON)
#define TEST_EQUAL_LOG(a,b) TEST_ASSERT(abs((a) - (b)) < LOG_EPSILON)
#define TEST_EQUAL_STRING(a,b) TEST_ASSERT(strcmp((a), (b)) == 0)

