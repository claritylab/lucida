#include <stdio.h>

#include "yin.h"
#include "ckd_alloc.h"

#include "test_macros.h"

int
main(int argc, char *argv[])
{
	/* This is 11025Hz data (yikes) */
	static const int frame_shift = 110, frame_size = 265;
	FILE *raw;
	yin_t *pe;
	int16 *buf;
	size_t nsamp, start;
	uint16 period, bestdiff;
	int nfr;

	/* To make life easier, read the whole thing. */
	TEST_ASSERT(raw = fopen(TESTDATADIR "/chan3.raw", "rb"));
	fseek(raw, 0, SEEK_END);
	nsamp = ftell(raw) / 2;
	buf = ckd_calloc(nsamp, 2);
	fseek(raw, 0, SEEK_SET);
	TEST_EQUAL(nsamp, fread(buf, 2, nsamp, raw));
	fclose(raw);

	TEST_ASSERT(pe = yin_init(frame_size, 0.1, 0.2, 2));
	yin_start(pe);
	nfr = 0;
	for (start = 0; start + frame_size < nsamp; start += frame_shift) {
		yin_write(pe, buf + start);
		if (yin_read(pe, &period, &bestdiff)) {
			if (bestdiff < 0.2 * 32768)
				printf("%d ", period ? 11025/period : 0);
			else
				printf("0 ");
			++nfr;
		}
	}
	yin_end(pe);
	while (yin_read(pe, &period, &bestdiff)) {
		if (bestdiff < 0.2 * 32768)
			printf("%d ", period ? 11025/period : 0);
		else
			printf("0 ");
		++nfr;
	}
	printf("\n");
	yin_free(pe);
	ckd_free(buf);

	return 0;
}
