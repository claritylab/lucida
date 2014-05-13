/**
 * @file test_hash_iter.c Test hash table iterators
 * @author David Huggins-Daines <dhuggins@cs.cmu.edu>
 */

#include "hash_table.h"
#include "ckd_alloc.h"
#include "test_macros.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
main(int argc, char *argv[])
{
	hash_table_t *h;
	hash_iter_t *itor;
	char *foo2 = ckd_salloc("foo");
	char *foo3 = ckd_salloc("foo");

	/* Test insertion and replacement. */
	TEST_ASSERT(h = hash_table_new(42, FALSE));
	TEST_EQUAL((void*)0xdeadbeef, hash_table_enter(h, "foo", (void*)0xdeadbeef));
	TEST_EQUAL((void*)0xdeadbeef, hash_table_replace(h, foo2, (void*)0xd0d0feed));
	TEST_EQUAL((void*)0xd0d0feed, hash_table_replace(h, foo3, (void*)0xdeadbeaf));
	TEST_EQUAL((void*)0xcafec0de, hash_table_enter(h, "bar", (void*)0xcafec0de));
	TEST_EQUAL((void*)0xeeefeeef, hash_table_enter(h, "baz", (void*)0xeeefeeef));
	TEST_EQUAL((void*)0xbabababa, hash_table_enter(h, "quux", (void*)0xbabababa));

	hash_table_display(h, TRUE);
	/* Now test iterators. */
	for (itor = hash_table_iter(h); itor; itor = hash_table_iter_next(itor)) {
		printf("%s %p\n", itor->ent->key, itor->ent->val);
		if (0 == strcmp(itor->ent->key, "foo")) {
			TEST_EQUAL(itor->ent->val, (void*)0xdeadbeaf);
		}
		else if (0 == strcmp(itor->ent->key, "bar")) {
			TEST_EQUAL(itor->ent->val, (void*)0xcafec0de);
		}
		else if (0 == strcmp(itor->ent->key, "baz")) {
			TEST_EQUAL(itor->ent->val, (void*)0xeeefeeef);
		}
		else if (0 == strcmp(itor->ent->key, "quux")) {
			TEST_EQUAL(itor->ent->val, (void*)0xbabababa);
		}
	}
	ckd_free(foo2);
	ckd_free(foo3);

	return 0;
}
