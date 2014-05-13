/* -*- c-basic-offset: 4 -*- */
#include "config.h"
#include "ad.h"
#include "cont_ad.h"
#include "byteorder.h"
#include "test_macros.h"

#include <stdio.h>

#ifndef WORDS_BIGENDIAN
#define WORDS_BIGENDIAN 0
#endif

FILE *infp;

static int32
file_ad_read(ad_rec_t * r, int16 * buf, int32 max)
{
    int32 i, k;

    k = fread(buf, sizeof(int16), max, infp);
    if (WORDS_BIGENDIAN) {
        for (i = 0; i < k; i++) {
	    SWAP_INT16(&buf[i]);
        }
    }

    return ((k > 0) ? k : -1);
}

int
main(int argc, char *argv[])
{
    cont_ad_t *cont;
    ad_rec_t ad;
    int16 buf[512];
    int listening;

    ad.sps = 16000;
    ad.bps = 2;

    TEST_ASSERT(infp = fopen(TESTDATADIR "/chan3.raw", "rb"));
    TEST_ASSERT(cont = cont_ad_init(&ad, file_ad_read));

    printf("Calibrating ...");
    fflush(stdout);
    if (cont_ad_calib(cont) < 0)
	    printf(" failed; file too short?\n");
    else
	    printf(" done after %ld samples\n", ftell(infp) / 2);
    rewind(infp);

    listening = FALSE;
    while (1) {
	int k = cont_ad_read(cont, buf, 512);
	/* End of file. */
	if (k < 0) {
	    if (listening) {
		printf("End of file at %.3f seconds\n",
		       (double)(cont->read_ts - k) / 16000);
	    }
	    break;
	}

	if (cont->state == CONT_AD_STATE_SIL) {
	    /* Has there been enough silence to cut the utterance? */
	    if (listening && cont->seglen > 8000) {
		printf("End of utterance at %.3f seconds\n",
		       (double)(cont->read_ts - k - cont->seglen) / 16000);
		listening = FALSE;
	    }
	}
	else {
	    if (!listening) {
		printf("Start of utterance at %.3f seconds\n",
		       (double)(cont->read_ts - k) / 16000);
		listening = TRUE;
	    }
	}
    }

    cont_ad_close(cont);
    fclose(infp);
    return 0;
}
