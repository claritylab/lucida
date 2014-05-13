#include <pocketsphinx.h>
#include <stdio.h>
#include <string.h>

#include "pocketsphinx_internal.h"

#include "test_macros.h"

static const mfcc_t prior[13] = {
    FLOAT2MFCC(33.89),
    FLOAT2MFCC(-1.13),
    FLOAT2MFCC(0.83),
    FLOAT2MFCC(0.49),
    FLOAT2MFCC(-0.65),
    FLOAT2MFCC(0.12),
    FLOAT2MFCC(-0.03),
    FLOAT2MFCC(0.28),
    FLOAT2MFCC(0.41),
    FLOAT2MFCC(0.59),
    FLOAT2MFCC(0.11),
    FLOAT2MFCC(-0.20),
    FLOAT2MFCC(0.17)
};

int
ps_decoder_test(cmd_ln_t *config, char const *sname, char const *expected)
{
    ps_decoder_t *ps;
    mfcc_t **cepbuf;
    FILE *rawfh;
    int16 *buf;
    int16 const *bptr;
    size_t nread;
    size_t nsamps;
    int32 nfr, i, score, prob;
    char const *hyp;
    char const *uttid;
    double n_speech, n_cpu, n_wall;
    ps_seg_t *seg;

    TEST_ASSERT(ps = ps_init(config));
    /* Test it first with pocketsphinx_decode_raw() */
    TEST_ASSERT(rawfh = fopen(DATADIR "/goforward.raw", "rb"));
    ps_decode_raw(ps, rawfh, "goforward", -1);
    hyp = ps_get_hyp(ps, &score, &uttid);
    prob = ps_get_prob(ps, &uttid);
    printf("%s (%s): %s (%d, %d)\n", sname, uttid, hyp, score, prob);
    TEST_EQUAL(0, strcmp(hyp, expected));
    TEST_ASSERT(prob <= 0);
    ps_get_utt_time(ps, &n_speech, &n_cpu, &n_wall);
    printf("%.2f seconds speech, %.2f seconds CPU, %.2f seconds wall\n",
           n_speech, n_cpu, n_wall);
    printf("%.2f xRT (CPU), %.2f xRT (elapsed)\n",
           n_cpu / n_speech, n_wall / n_speech);

    /* Test it with ps_process_raw() */
    clearerr(rawfh);
    fseek(rawfh, 0, SEEK_END);
    nsamps = ftell(rawfh) / sizeof(*buf);
    fseek(rawfh, 0, SEEK_SET);
    TEST_EQUAL(0, ps_start_utt(ps, NULL));
    nsamps = 2048;
    buf = ckd_calloc(nsamps, sizeof(*buf));
    while (!feof(rawfh)) {
        nread = fread(buf, sizeof(*buf), nsamps, rawfh);
        ps_process_raw(ps, buf, nread, FALSE, FALSE);
    }
    TEST_EQUAL(0, ps_end_utt(ps));
    hyp = ps_get_hyp(ps, &score, &uttid);
    prob = ps_get_prob(ps, &uttid);
    printf("%s (%s): %s (%d, %d)\n", sname, uttid, hyp, score, prob);
    TEST_EQUAL(0, strcmp(uttid, "000000000"));
    TEST_EQUAL(0, strcmp(hyp, expected));
    ps_get_utt_time(ps, &n_speech, &n_cpu, &n_wall);
    printf("%.2f seconds speech, %.2f seconds CPU, %.2f seconds wall\n",
           n_speech, n_cpu, n_wall);
    printf("%.2f xRT (CPU), %.2f xRT (elapsed)\n",
           n_cpu / n_speech, n_wall / n_speech);

    /* Now read the whole file and produce an MFCC buffer. */
    clearerr(rawfh);
    fseek(rawfh, 0, SEEK_END);
    nsamps = ftell(rawfh) / sizeof(*buf);
    fseek(rawfh, 0, SEEK_SET);
    bptr = buf = ckd_realloc(buf, nsamps * sizeof(*buf));
    TEST_EQUAL(nsamps, fread(buf, sizeof(*buf), nsamps, rawfh));
    fe_process_frames(ps->acmod->fe, &bptr, &nsamps, NULL, &nfr);
    cepbuf = ckd_calloc_2d(nfr + 1,
                   fe_get_output_size(ps->acmod->fe),
                   sizeof(**cepbuf));
    fe_start_utt(ps->acmod->fe);
    fe_process_frames(ps->acmod->fe, &bptr, &nsamps, cepbuf, &nfr);
    fe_end_utt(ps->acmod->fe, cepbuf[nfr], &i);

    /* Decode it with process_cep() */
    TEST_EQUAL(0, ps_start_utt(ps, NULL));
    for (i = 0; i < nfr; ++i) {
        ps_process_cep(ps, cepbuf + i, 1, FALSE, FALSE);
    }
    TEST_EQUAL(0, ps_end_utt(ps));
    hyp = ps_get_hyp(ps, &score, &uttid);
    prob = ps_get_prob(ps, &uttid);
    printf("%s (%s): %s (%d, %d)\n", sname, uttid, hyp, score, prob);
    TEST_EQUAL(0, strcmp(uttid, "000000001"));
    TEST_EQUAL(0, strcmp(hyp, expected));
    TEST_ASSERT(prob <= 0);
    for (seg = ps_seg_iter(ps, &score); seg;
         seg = ps_seg_next(seg)) {
        char const *word;
        int sf, ef;
        int32 post, lscr, ascr, lback;

        word = ps_seg_word(seg);
        ps_seg_frames(seg, &sf, &ef);
        post = ps_seg_prob(seg, &ascr, &lscr, &lback);
        printf("%s (%d:%d) P(w|o) = %f ascr = %d lscr = %d lback = %d\n", word, sf, ef,
               logmath_exp(ps_get_logmath(ps), post), ascr, lscr, lback);
        TEST_ASSERT(post <= 2); // Due to numerical errors with float it sometimes could go out of 0
    }

    ps_get_utt_time(ps, &n_speech, &n_cpu, &n_wall);
    printf("%.2f seconds speech, %.2f seconds CPU, %.2f seconds wall\n",
           n_speech, n_cpu, n_wall);
    printf("%.2f xRT (CPU), %.2f xRT (elapsed)\n",
           n_cpu / n_speech, n_wall / n_speech);
    ps_get_all_time(ps, &n_speech, &n_cpu, &n_wall);
    printf("TOTAL: %.2f seconds speech, %.2f seconds CPU, %.2f seconds wall\n",
           n_speech, n_cpu, n_wall);
    printf("TOTAL: %.2f xRT (CPU), %.2f xRT (elapsed)\n",
           n_cpu / n_speech, n_wall / n_speech);

    fclose(rawfh);
    ps_free(ps);
    cmd_ln_free_r(config);
    ckd_free_2d(cepbuf);
    ckd_free(buf);

    return 0;
}
