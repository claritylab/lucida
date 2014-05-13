/**
 * @file test_heap.c Test heaps
 * @author David Huggins-Daines <dhuggins@cs.cmu.edu>
 */

#include "heap.h"
#include "test_macros.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
main(int argc, char *argv[])
{
	heap_t *heap;
	int i;

	heap = heap_new();
	for (i = 0; i < 25; ++i)
		heap_insert(heap, (void *)(long)i, i);
	for (i = 0; i < 25; ++i) {
		int32 val;
		void *data;
		TEST_EQUAL(1, heap_pop(heap, &data, &val));
		TEST_EQUAL(val, i);
		TEST_EQUAL((int)(long)data, i);
		TEST_EQUAL(heap_size(heap), 25 - i - 1);
	}
	for (i = 0; i < 25; ++i)
		heap_insert(heap, (void *)(long)i, i);
	TEST_EQUAL(0, heap_remove(heap, (void *)(long)10));
	TEST_EQUAL(-1, heap_remove(heap, (void *)(long)10));
	TEST_EQUAL(heap_size(heap), 24);
	TEST_EQUAL(0, heap_remove(heap, (void *)(long)15));
	TEST_EQUAL(0, heap_remove(heap, (void *)(long)9));
	TEST_EQUAL(0, heap_remove(heap, (void *)(long)0));
	TEST_EQUAL(heap_size(heap), 21);
	return 0;
}
