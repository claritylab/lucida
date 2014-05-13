#include <stdio.h>
#include <string.h>

#include "fe.h"
#include "cmd_ln.h"
#include "ckd_alloc.h"

#include "test_macros.h"

int
main(int argc, char *argv[])
{
    static const arg_t fe_args[] = {
        waveform_to_cepstral_command_line_macro(),
        { NULL, 0, NULL, NULL }
    };
    FILE *raw;
    cmd_ln_t *config;
    fe_t *fe;
    int16 buf[1024];
    int16 const *inptr;
    int32 frame_shift, frame_size;
    mfcc_t **cepbuf1, **cepbuf2, **cptr;
    int32 nfr, i;
    size_t nsamp;

    TEST_ASSERT(config = cmd_ln_parse_r(NULL, fe_args, argc, argv, FALSE));
    TEST_ASSERT(fe = fe_init_auto_r(config));

    TEST_EQUAL(fe_get_output_size(fe), DEFAULT_NUM_CEPSTRA);

    fe_get_input_size(fe, &frame_shift, &frame_size);
    TEST_EQUAL(frame_shift, DEFAULT_FRAME_SHIFT);
    TEST_EQUAL(frame_size, (int)(DEFAULT_WINDOW_LENGTH*DEFAULT_SAMPLING_RATE));

    TEST_ASSERT(raw = fopen(TESTDATADIR "/chan3.raw", "rb"));

    TEST_EQUAL(0, fe_start_utt(fe));
    TEST_EQUAL(1024, fread(buf, sizeof(int16), 1024, raw));

    nsamp = 1024;
    TEST_ASSERT(fe_process_frames(fe, NULL, &nsamp, NULL, &nfr) >= 0);
    TEST_EQUAL(1024, nsamp);
    TEST_EQUAL(4, nfr);

    cepbuf1 = ckd_calloc_2d(5, DEFAULT_NUM_CEPSTRA, sizeof(**cepbuf1));
    inptr = &buf[0];
    nfr = 1;

    printf("frame_size %d frame_shift %d\n", frame_size, frame_shift);
    /* Process the first frame. */
    TEST_ASSERT(fe_process_frames(fe, &inptr, &nsamp, &cepbuf1[0], &nfr) >= 0);
    printf("inptr %d nsamp %d nfr %d\n", inptr - buf, nsamp, nfr);
    /* First frame assumed to be unvoiced to init noise reduction */
    TEST_EQUAL(nfr, 0);

    /* Note that this next one won't actually consume any frames
     * of input, because it already got sufficient overflow
     * samples last time around.  This is implementation-dependent
     * so we shouldn't actually test for it. 
     * First 1024 samples of chan3.raw is silence, nfr is expected to stay 0 */
    nfr = 1;
    TEST_ASSERT(fe_process_frames(fe, &inptr, &nsamp, &cepbuf1[1], &nfr) >= 0);
    printf("inptr %d nsamp %d nfr %d\n", inptr - buf, nsamp, nfr);
    TEST_EQUAL(nfr, 0);
    
    nfr = 1;
    TEST_ASSERT(fe_process_frames(fe, &inptr, &nsamp, &cepbuf1[2], &nfr) >= 0);
    printf("inptr %d nsamp %d nfr %d\n", inptr - buf, nsamp, nfr);
    TEST_EQUAL(nfr, 0);

    nfr = 1;
    TEST_ASSERT(fe_process_frames(fe, &inptr, &nsamp, &cepbuf1[3], &nfr) >= 0);
    printf("inptr %d nsamp %d nfr %d\n", inptr - buf, nsamp, nfr);
    TEST_EQUAL(nfr, 0);

    nfr = 1;
    TEST_ASSERT(fe_end_utt(fe, cepbuf1[4], &nfr) >= 0);
    printf("nfr %d\n", nfr);
    TEST_EQUAL(nfr, 0);

    /* What we *should* test is that the output we get by
     * processing one frame at a time is exactly the same as what
     * we get from doing them all at once.  So let's do that */
    cepbuf2 = ckd_calloc_2d(5, DEFAULT_NUM_CEPSTRA, sizeof(**cepbuf2));
    inptr = &buf[0];
    nfr = 5;
    nsamp = 1024;
    TEST_EQUAL(0, fe_start_utt(fe));
    TEST_ASSERT(fe_process_frames(fe, &inptr, &nsamp, cepbuf2, &nfr) >= 0);
    /* First 1024 samples of chan3.raw is silence, nfr is expected to stay 0 */
    printf("nfr %d\n", nfr);
    TEST_EQUAL(nfr, 0);
    nfr = 1;
    TEST_ASSERT(fe_end_utt(fe, cepbuf2[4], &nfr) >= 0);
    printf("nfr %d\n", nfr);
    TEST_EQUAL(nfr, 0);
    /* fe_process_frames overwrites features if frame is unvoiced, 
     * so for cepbuf2 last frame is at 0 and previous are lost */
    printf("%d: ", 3);
    for (i = 0; i < DEFAULT_NUM_CEPSTRA; ++i) {
        printf("%.2f,%.2f ",
               MFCC2FLOAT(cepbuf1[3][i]),
               MFCC2FLOAT(cepbuf2[0][i]));
        TEST_EQUAL_FLOAT(cepbuf1[3][i], cepbuf2[0][i]);
    }
    printf("\n");
    /* output features stored in cepbuf[4] by fe_end_utt 
     * should be the same */
    printf("%d: ", 4);
    for (i = 0; i < DEFAULT_NUM_CEPSTRA; ++i) {
        printf("%.2f,%.2f ",
               MFCC2FLOAT(cepbuf1[4][i]),
               MFCC2FLOAT(cepbuf2[4][i]));
        TEST_EQUAL_FLOAT(cepbuf1[4][i], cepbuf2[4][i]);
    }
    printf("\n");

    /* Now, also test to make sure that even if we feed data in
     * little tiny bits we can still make things work. */
    memset(cepbuf2[0], 0, 5 * DEFAULT_NUM_CEPSTRA * sizeof(**cepbuf2));
    inptr = &buf[0];
    cptr = &cepbuf2[0];
    nfr = 5;
    i = 5;
    nsamp = 256;
    TEST_EQUAL(0, fe_start_utt(fe));
    TEST_ASSERT(fe_process_frames(fe, &inptr, &nsamp, cptr, &i) >= 0);
    printf("inptr %d nsamp %d nfr %d\n", inptr - buf, nsamp, i);
    cptr += i;
    nfr -= i;
    i = nfr;
    nsamp = 256;
    TEST_ASSERT(fe_process_frames(fe, &inptr, &nsamp, cptr, &i) >= 0);
    printf("inptr %d nsamp %d nfr %d\n", inptr - buf, nsamp, i);
    cptr += i;
    nfr -= i;
    i = nfr;
    nsamp = 256;
    TEST_ASSERT(fe_process_frames(fe, &inptr, &nsamp, cptr, &i) >= 0);
    printf("inptr %d nsamp %d nfr %d\n", inptr - buf, nsamp, i);
    cptr += i;
    nfr -= i;
    i = nfr;
    nsamp = 256;
    TEST_ASSERT(fe_process_frames(fe, &inptr, &nsamp, cptr, &i) >= 0);
    printf("inptr %d nsamp %d nfr %d\n", inptr - buf, nsamp, i);
    cptr += i;
    nfr -= i;
    printf("nfr %d\n", nfr);
    TEST_EQUAL(nfr, 5);
    /* inptr contains unvoiced audio, 
     * no out feature frames will be produced */
    TEST_ASSERT(fe_end_utt(fe, *cptr, &nfr) >= 0);
    printf("nfr %d\n", nfr);
    TEST_EQUAL(nfr, 0);

    /* fe_process_frames overwrites features if frame is unvoiced, 
     * so for cepbuf2 last frame is at 0 and previous are lost */
    printf("%d: ", 4);
    for (i = 0; i < DEFAULT_NUM_CEPSTRA; ++i) {
        printf("%.2f,%.2f ",
               MFCC2FLOAT(cepbuf1[4][i]),
               MFCC2FLOAT(cepbuf2[0][i]));
        TEST_EQUAL_FLOAT(cepbuf1[4][i], cepbuf2[0][i]);
    }
    printf("\n");

    /* And now, finally, test fe_process_utt() */
    inptr = &buf[0];
    i = 0;
    TEST_EQUAL(0, fe_start_utt(fe));
    TEST_ASSERT(fe_process_utt(fe, inptr, 256, &cptr, &nfr) >= 0);
    printf("i %d nfr %d\n", i, nfr);
    if (nfr)
        memcpy(cepbuf2[i], cptr[0], nfr * DEFAULT_NUM_CEPSTRA * sizeof(**cptr));
    ckd_free_2d(cptr);
    i += nfr;
    inptr += 256;
    TEST_ASSERT(fe_process_utt(fe, inptr, 256, &cptr, &nfr) >= 0);
    printf("i %d nfr %d\n", i, nfr);
    if (nfr)
        memcpy(cepbuf2[i], cptr[0], nfr * DEFAULT_NUM_CEPSTRA * sizeof(**cptr));
    ckd_free_2d(cptr);
    i += nfr;
    inptr += 256;
    TEST_ASSERT(fe_process_utt(fe, inptr, 256, &cptr, &nfr) >= 0);
    printf("i %d nfr %d\n", i, nfr);
    if (nfr)
        memcpy(cepbuf2[i], cptr[0], nfr * DEFAULT_NUM_CEPSTRA * sizeof(**cptr));
    ckd_free_2d(cptr);
    i += nfr;
    inptr += 256;
    TEST_ASSERT(fe_process_utt(fe, inptr, 256, &cptr, &nfr) >= 0);
    printf("i %d nfr %d\n", i, nfr);
    if (nfr)
        memcpy(cepbuf2[i], cptr[0], nfr * DEFAULT_NUM_CEPSTRA * sizeof(**cptr));
    ckd_free_2d(cptr);
    i += nfr;
    inptr += 256;
    TEST_ASSERT(fe_end_utt(fe, cepbuf2[i], &nfr) >= 0);
    printf("i %d nfr %d\n", i, nfr);
    TEST_EQUAL(nfr, 0);

    /* fe_process_utt overwrites features if frame is unvoiced, 
     * so for cepbuf2 last frame is at 0 and previous are lost */
    printf("%d: ", 4);
    for (i = 0; i < DEFAULT_NUM_CEPSTRA; ++i) {
        printf("%.2f,%.2f ",
               MFCC2FLOAT(cepbuf1[4][i]),
               MFCC2FLOAT(cepbuf2[0][i]));
        TEST_EQUAL_FLOAT(cepbuf1[4][i], cepbuf2[0][i]);
    }
    printf("\n");

    ckd_free_2d(cepbuf1);
    ckd_free_2d(cepbuf2);
    fclose(raw);
    fe_free(fe);
    cmd_ln_free_r(config);

    return 0;
}
