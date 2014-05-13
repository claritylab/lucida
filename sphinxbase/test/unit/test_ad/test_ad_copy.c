/* -*- c-basic-offset: 4 -*- */

#include "config.h"
#include "ad.h"
#include "cont_ad.h"
#include "ckd_alloc.h"
#include "byteorder.h"
#include "test_macros.h"

#include <stdio.h>
#include <string.h>

#ifndef WORDS_BIGENDIAN
#define WORDS_BIGENDIAN 0
#endif

int
main(int argc, char *argv[])
{
    cont_ad_t *cont;
    FILE *infp;
    int16 buf[512];
    int listening;
    int k, n_calib_samp;
    int16 *calib, *cptr;

    TEST_ASSERT(infp = fopen(TESTDATADIR "/chan3.raw", "rb"));
    TEST_ASSERT(cont = cont_ad_init(NULL, NULL));

    n_calib_samp = cont_ad_calib_size(cont);
    calib = ckd_malloc(n_calib_samp * 2);
    printf("Reading %d calibration samples\n", n_calib_samp);
    TEST_ASSERT(fread(calib, 2, n_calib_samp, infp) == n_calib_samp);
    printf("Calibrating...\n");
    TEST_EQUAL(0, cont_ad_calib_loop(cont, calib, n_calib_samp));
    printf("Calibrated!\n");

    listening = FALSE;
    cptr = calib;
    while (1) {
	/* Use up the calibration samples first. */
	if (n_calib_samp) {
	    k = n_calib_samp;
	    if (k > 512)
		k = 512;
	    memcpy(buf, cptr, 512 * 2);
	    cptr += k;
	    n_calib_samp -= k;
	    if (k < 512)
		k = fread(buf + k, 2, 512-k, infp);
	}
	else {
	    k = fread(buf, 2, 512, infp);
	}

	/* End of file. */
	if (k < 256) { /* FIXME: It should do something useful with fewer samples. */
	    if (listening) {
		printf("End of file at %.3f seconds\n",
		       (double)(cont->read_ts - k) / 16000);
	    }
	    break;
	}

	k = cont_ad_read(cont, buf, k);

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

    ckd_free(calib);
    cont_ad_close(cont);
    fclose(infp);
    return 0;
}
