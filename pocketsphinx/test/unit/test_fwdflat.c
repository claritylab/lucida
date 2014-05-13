#include <pocketsphinx.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "pocketsphinx_internal.h"
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

    TEST_ASSERT(config =
            cmd_ln_init(NULL, ps_args(), TRUE,
                "-hmm", MODELDIR "/hmm/en_US/hub4wsj_sc_8k",
                "-lm", MODELDIR "/lm/en_US/wsj0vp.5000.DMP",
                "-dict", MODELDIR "/lm/en_US/cmu07a.dic",
                "-fwdtree", "no",
                "-fwdflat", "yes",
                "-bestpath", "no",
                "-input_endian", "little",
                "-samprate", "16000", NULL));
    TEST_ASSERT(ps = ps_init(config));

    ngs = (ngram_search_t *)ps->search;
    acmod = ps->acmod;

    setbuf(stdout, NULL);
    c = clock();
    {
        FILE *rawfh;
        int16 buf[2048];
        size_t nread;
        int16 const *bptr;
        int nfr;

        TEST_ASSERT(rawfh = fopen(DATADIR "/goforward.raw", "rb"));
        TEST_EQUAL(0, acmod_start_utt(acmod));
        ngram_fwdflat_start(ngs);
        while (!feof(rawfh)) {
            nread = fread(buf, sizeof(*buf), 2048, rawfh);
            bptr = buf;
            while ((nfr = acmod_process_raw(acmod, &bptr, &nread, FALSE)) > 0) {
                while (acmod->n_feat_frame > 0) {
                    ngram_fwdflat_search(ngs,acmod->output_frame);
                    acmod_advance(acmod);
                }
            }
        }
        ngram_fwdflat_finish(ngs);
        printf("%s\n",
               ngram_search_bp_hyp(ngs, ngram_search_find_exit(ngs, -1, NULL, NULL)));

        TEST_ASSERT(acmod_end_utt(acmod) >= 0);
        fclose(rawfh);
    }
    TEST_EQUAL(0, strcmp("a former ten leaders",
                 ngram_search_bp_hyp(ngs, ngram_search_find_exit(ngs, -1, NULL, NULL))));
    c = clock() - c;
    printf("2 * fwdflat search in %.2f sec\n",
           (double)c / CLOCKS_PER_SEC);
    ps_free(ps);
    cmd_ln_free_r(config);

    return 0;
}
