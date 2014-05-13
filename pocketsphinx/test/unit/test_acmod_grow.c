#include <stdio.h>
#include <string.h>
#include <pocketsphinx.h>

#include <sphinxbase/logmath.h>

#include "acmod.h"
#include "test_macros.h"

static const mfcc_t prior[13] = {
    FLOAT2MFCC(37.03),
    FLOAT2MFCC(-1.01),
    FLOAT2MFCC(0.53),
    FLOAT2MFCC(0.49),
    FLOAT2MFCC(-0.60),
    FLOAT2MFCC(0.14),
    FLOAT2MFCC(-0.05),
    FLOAT2MFCC(0.25),
    FLOAT2MFCC(0.37),
    FLOAT2MFCC(0.58),
    FLOAT2MFCC(0.13),
    FLOAT2MFCC(-0.16),
    FLOAT2MFCC(0.17)
};

int
main(int argc, char *argv[])
{
    acmod_t *acmod;
    logmath_t *lmath;
    cmd_ln_t *config;
    FILE *rawfh;
    int16 *buf;
    int16 const *bptr;
    size_t nread, nsamps;
    int nfr;
    int frame_counter;
    int bestsen1[270];

    lmath = logmath_init(1.0001, 0, 0);
    config = cmd_ln_init(NULL, ps_args(), TRUE,
                 "-featparams", MODELDIR "/hmm/en_US/hub4wsj_sc_8k/feat.params",
                 "-mdef", MODELDIR "/hmm/en_US/hub4wsj_sc_8k/mdef",
                 "-mean", MODELDIR "/hmm/en_US/hub4wsj_sc_8k/means",
                 "-var", MODELDIR "/hmm/en_US/hub4wsj_sc_8k/variances",
                 "-tmat", MODELDIR "/hmm/en_US/hub4wsj_sc_8k/transition_matrices",
                 "-sendump", MODELDIR "/hmm/en_US/hub4wsj_sc_8k/sendump",
                 "-compallsen", "true",
                 "-cmn", "prior",
                 "-tmatfloor", "0.0001",
                 "-mixwfloor", "0.001",
                 "-varfloor", "0.0001",
                 "-mmap", "no",
                 "-topn", "4",
                 "-ds", "1",
                 "-input_endian", "little",
                 "-samprate", "16000", NULL);
    TEST_ASSERT(config);
    TEST_ASSERT(acmod = acmod_init(config, lmath, NULL, NULL));
    cmn_prior_set(acmod->fcb->cmn_struct, prior);

    nsamps = 2048;
    frame_counter = 0;
    buf = ckd_calloc(nsamps, sizeof(*buf));
    TEST_ASSERT(rawfh = fopen(DATADIR "/goforward.raw", "rb"));
    TEST_EQUAL(FALSE, acmod_set_grow(acmod, TRUE));
    TEST_EQUAL(0, acmod_start_utt(acmod));
    printf("Incremental(2048):\n");
    while (!feof(rawfh)) {
        nread = fread(buf, sizeof(*buf), nsamps, rawfh);
        bptr = buf;
        while ((nfr = acmod_process_raw(acmod, &bptr, &nread, FALSE)) > 0 || nread > 0) {
            int16 const *senscr;
            int16 best_score;
            int frame_idx = -1, best_senid;
            while (acmod->n_feat_frame > 0) {
                senscr = acmod_score(acmod, &frame_idx);
                acmod_advance(acmod);
                best_score = acmod_best_score(acmod, &best_senid);
                printf("Frame %d best senone %d score %d\n",
                       frame_idx, best_senid, best_score);
                TEST_EQUAL(frame_counter, frame_idx);
                if (frame_counter < 190)
                    bestsen1[frame_counter] = best_senid;
                ++frame_counter;
                frame_idx = -1;
            }
        }
    }
    TEST_EQUAL(0, acmod_end_utt(acmod));
    nread = 0;
    acmod_process_raw(acmod, NULL, &nread, FALSE);
    {
        int16 const *senscr;
        int16 best_score;
        int frame_idx = -1, best_senid;
        while (acmod->n_feat_frame > 0) {
            senscr = acmod_score(acmod, &frame_idx);
            acmod_advance(acmod);
            best_score = acmod_best_score(acmod, &best_senid);
            printf("Frame %d best senone %d score %d\n",
                   frame_idx, best_senid, best_score);
            if (frame_counter < 190)
                bestsen1[frame_counter] = best_senid;
            TEST_EQUAL(frame_counter, frame_idx);
            ++frame_counter;
            frame_idx = -1;
        }
    }

    printf("Rewound (MFCC):\n");
    TEST_EQUAL(0, acmod_rewind(acmod));
    {
        int16 const *senscr;
        int16 best_score;
        int frame_idx = -1, best_senid;
        frame_counter = 0;
        while (acmod->n_feat_frame > 0) {
            senscr = acmod_score(acmod, &frame_idx);
            acmod_advance(acmod);
            best_score = acmod_best_score(acmod, &best_senid);
            printf("Frame %d best senone %d score %d\n",
                   frame_idx, best_senid, best_score);
            if (frame_counter < 190)
                TEST_EQUAL(best_senid, bestsen1[frame_counter]);
            TEST_EQUAL(frame_counter, frame_idx);
            ++frame_counter;
            frame_idx = -1;
        }
    }

    /* Clean up, go home. */
    fclose(rawfh);
    ckd_free(buf);
    acmod_free(acmod);
    logmath_free(lmath);
    cmd_ln_free_r(config);
    return 0;
}
