#include <pocketsphinx.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "pocketsphinx_internal.h"
#include "ngram_search_fwdtree.h"
#include "ngram_search_fwdflat.h"
#include "test_macros.h"

int
main(int argc, char *argv[])
{
    ps_decoder_t *ps;
    cmd_ln_t *config;
    acmod_t *acmod;
    ngram_search_t *ngs;
    clock_t c;
    int i;

    TEST_ASSERT(config =
            cmd_ln_init(NULL, ps_args(), TRUE,
                "-hmm", MODELDIR "/hmm/en_US/hub4wsj_sc_8k",
                "-lm", MODELDIR "/lm/en_US/wsj0vp.5000.DMP",
                "-dict", MODELDIR "/lm/en_US/cmu07a.dic",
                "-fwdtree", "yes",
                "-fwdflat", "yes",
                "-bestpath", "no",
                "-input_endian", "little",
                "-samprate", "16000", NULL));
    TEST_ASSERT(ps = ps_init(config));

    ngs = (ngram_search_t *)ps->search;
    acmod = ps->acmod;
        acmod_set_grow(ps->acmod, TRUE);

    setbuf(stdout, NULL);
    c = clock();
    for (i = 0; i < 5; ++i) {
        FILE *rawfh;
        int16 buf[2048];
        size_t nread;
        int16 const *bptr;
        int nfr;

        TEST_ASSERT(rawfh = fopen(DATADIR "/goforward.raw", "rb"));
        TEST_EQUAL(0, acmod_start_utt(acmod));
        ngram_fwdtree_start(ngs);
        while (!feof(rawfh)) {
            nread = fread(buf, sizeof(*buf), 2048, rawfh);
            bptr = buf;
            while ((nfr = acmod_process_raw(acmod, &bptr, &nread, FALSE)) > 0) {
                while (acmod->n_feat_frame > 0) {
                    ngram_fwdtree_search(ngs, acmod->output_frame);
                    acmod_advance(acmod);
                }
            }
        }

        TEST_ASSERT(acmod_end_utt(acmod) >= 0);
        while (acmod->n_feat_frame > 0) {
            ngram_fwdtree_search(ngs, acmod->output_frame);
            acmod_advance(acmod);
        }
        ngram_fwdtree_finish(ngs);
        printf("FWDTREE: %s\n",
               ngram_search_bp_hyp(ngs, ngram_search_find_exit(ngs, -1, NULL, NULL)));
        fclose(rawfh);

        E_INFO("grow_feat %d output_frame %d n_mfc_alloc %d n_mfc_frame %d\n",
               acmod->grow_feat, acmod->output_frame, acmod->n_mfc_alloc,
               acmod->n_mfc_frame);
        E_INFO("mfc_outidx %d n_feat_alloc %d n_feat_frame %d feat_outidx %d\n",
               acmod->mfc_outidx, acmod->n_feat_alloc, acmod->n_feat_frame,
               acmod->feat_outidx);
        TEST_EQUAL(0, acmod_rewind(acmod));
        ngram_fwdflat_start(ngs);
        while (acmod->n_feat_frame > 0) {
            ngram_fwdflat_search(ngs, acmod->output_frame);
            acmod_advance(acmod);
        }
        ngram_fwdflat_finish(ngs);
        printf("FWDFLAT: %s\n",
               ngram_search_bp_hyp(ngs, ngram_search_find_exit(ngs, -1, NULL, NULL)));
    }
    TEST_EQUAL(0, strcmp("a former ten new shares",
                 ngram_search_bp_hyp(ngs, ngram_search_find_exit(ngs, -1, NULL, NULL))));
    c = clock() - c;
    printf("5 * fwdtree + fwdflat search in %.2f sec\n",
           (double)c / CLOCKS_PER_SEC);
    ps_free(ps);
    cmd_ln_free_r(config);

    return 0;
}
