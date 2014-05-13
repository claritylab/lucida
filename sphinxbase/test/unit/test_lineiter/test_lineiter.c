#include <pio.h>
#include <stdlib.h>

#include "test_macros.h"

int
main(int argc, char *argv[])
{
	FILE *fp;
	fp = fopen(FILEDIR "/test.txt", "rb");
	lineiter_t *li;
	int i;
	
	for (i = 0, li = lineiter_start(fp); i < 3 && li; li = lineiter_next(li), i++) {
	    printf ("Line is %s\n", li->buf);
	}
	
	printf("Buf is %s\n", li->buf);
	TEST_EQUAL_STRING(li->buf, "This is too\n");
	TEST_EQUAL(lineiter_lineno(li), 4);

	lineiter_free(li);
	
	rewind(fp);
	
	for (i = 0, li = lineiter_start_clean(fp); i < 3 && li; li = lineiter_next(li), i++) {
	    printf ("Clean line is %s\n", li->buf);
	}

	TEST_EQUAL_STRING(li->buf, "Bar");
	TEST_EQUAL(lineiter_lineno(li), 7);
	
	lineiter_free(li);
	fclose(fp);

	return 0;
}
