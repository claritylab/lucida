#include <stdio.h>
#include <string.h>

#include <listelem_alloc.h>

#include "test_macros.h"

struct bogus {
	char const *str;
	long foobie;
};

int
main(int argc, char *argv[])
{
	listelem_alloc_t *le;
	struct bogus *bogus1, *bogus2;
	int i;

	TEST_ASSERT(le = listelem_alloc_init(sizeof(struct bogus)));
	bogus1 = listelem_malloc(le);
	bogus1->str = "hello";
	bogus1->foobie = 42;
	bogus2 = listelem_malloc(le);
	bogus2->str = "goodbye";
	bogus2->foobie = 69;
	TEST_EQUAL(bogus1->foobie, 42);
	TEST_EQUAL(0, strcmp(bogus1->str, "hello"));
	listelem_free(le, bogus1);
	listelem_free(le, bogus2);
	listelem_alloc_free(le);

	TEST_ASSERT(le = listelem_alloc_init(sizeof(struct bogus)));
	listelem_stats(le);
	for (i = 0; i < 60; ++i)
		bogus1 = listelem_malloc(le);
	listelem_stats(le);
	listelem_alloc_free(le);

	{
		struct bogus *bogus[600];
		int32 bogus_id[600];

		le = listelem_alloc_init(sizeof(struct bogus));
		for (i = 0; i < 600; ++i)
			bogus[i] = listelem_malloc_id(le, bogus_id + i);
		listelem_stats(le);
		for (i = 0; i < 600; ++i) {
			TEST_EQUAL(bogus[i], listelem_get_item(le, bogus_id[i]));
		}
		for (i = 0; i < 600; ++i)
			listelem_free(le, bogus[i]);
		listelem_stats(le);
		for (i = 0; i < 600; ++i)
			bogus[i] = listelem_malloc_id(le, bogus_id + i);
		listelem_stats(le);
		for (i = 0; i < 600; ++i)
			TEST_EQUAL(bogus[i], listelem_get_item(le, bogus_id[i]));
		listelem_alloc_free(le);
	}


	return 0;
}
