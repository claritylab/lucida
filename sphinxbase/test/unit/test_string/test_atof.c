#include <stdio.h>
#include <string.h>
#include <math.h>
#include <locale.h>
#include <stdlib.h>

#include "strfuncs.h"

int
main(int argc, char *argv[])
{
	double foo;

	/* Ensure that it's really locale-independent. */
	if (setlocale(LC_ALL, "fr_CA.UTF-8") == NULL)
		fprintf(stderr, "Note: setlocale(LC_ALL, fr_CA.UTF-8) failed\n");
	
	foo = atof("1.5324523524523423");
	printf("atof(): 1.5324523524523423 %f\n", foo);

	foo = atof_c("1.5324523524523423");
	printf("1.5324523524523423 %f\n", foo);
	if (fabs(foo - 1.532) > 0.01)
		return 1;

	foo = atof_c("5e-3");
	printf("5e-3 %f\n", foo);
	if (fabs(foo - 0.005) > 0.01)
		return 1;

	foo = atof_c("1.2e+2");
	printf("1.2e+2 %f\n", foo);
	if (fabs(foo - 120.0) > 0.01)
		return 1;

	foo = atof_c("1e-80");
	printf("1e-80 %g\n", foo);

	return 0;
}
