/**
 * @file test_bit_encode.c Test bitstream encoding
 * @author David Huggins-Daines <dhuggins@cs.cmu.edu>
 */

#include "pio.h"
#include "test_macros.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
main(int argc, char *argv[])
{
	FILE *fh;
	bit_encode_t *be;
	int i;
	static unsigned char const bits[] = "\xde\xad\xbe\xef";
	uint32 cw = 0xdeadbeef;
	unsigned char inbits[16];

	fh = fopen("bittest.out", "wb");
	be = bit_encode_attach(fh);
	bit_encode_write(be, bits, 8);
	bit_encode_write(be, bits + 1, 16);
	bit_encode_write(be, bits + 3, 8);
	bit_encode_write_cw(be, cw >> 24, 8);
	bit_encode_write_cw(be, cw >> 16, 8);
	bit_encode_write_cw(be, cw >> 8, 8);
	bit_encode_write_cw(be, cw, 8);
	bit_encode_write_cw(be, cw >> 26, 6);
	bit_encode_write_cw(be, cw >> 14, 12);
	bit_encode_write_cw(be, cw >> 8, 6);
	bit_encode_write_cw(be, cw, 8);
	for (i = 0; i < 32; ++i) {
		bit_encode_write_cw(be, cw >> (31-i), 1);
	}
	bit_encode_flush(be);
	bit_encode_free(be);
	fclose(fh);
	fh = fopen("bittest.out", "rb");
	i = fread(inbits, 1, 16, fh);
	TEST_ASSERT(0 == memcmp(inbits, bits, 4));
	TEST_ASSERT(0 == memcmp(inbits + 4, bits, 4));
	TEST_ASSERT(0 == memcmp(inbits + 8, bits, 4));
	TEST_ASSERT(0 == memcmp(inbits + 12, bits, 4));
	fclose(fh);

	return 0;
}
