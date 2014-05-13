#include <pocketsphinx.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "pocketsphinx_internal.h"
#include "test_macros.h"

int
main(int argc, char *argv[])
{
    ps_decoder_t *ps;
    cmd_ln_t *config;
    acmod_t *acmod;
    ps_search_t *ngs, *pls;
    clock_t c;
    int32 score;
    int i;

    TEST_ASSERT(config =
            cmd_ln_init(NULL, ps_args(), TRUE,
                "-hmm", MODELDIR "/hmm/en_US/hub4wsj_sc_8k",
                "-lm", MODELDIR "/lm/en_US/wsj0vp.5000.DMP",
                "-dict", MODELDIR "/lm/en_US/cmu07a.dic",
                "-fwdtree", "yes",
                "-fwdflat", "no",
                "-bestpath", "no",
                "-pl_window", "6",
                "-input_endian", "little",
                "-samprate", "16000", NULL));
    TEST_ASSERT(ps = ps_init(config));

    ngs = ps->search;
    pls = ps->phone_loop;
    acmod = ps->acmod;

    setbuf(stdout, NULL);
    c = clock();
    for (i = 0; i < 5; ++i) {
        FILE *rawfh;
        int16 buf[2048];
        size_t nread;
        int16 const *bptr;
        int nfr, n_searchfr;

        TEST_ASSERT(rawfh = fopen(DATADIR "/goforward.raw", "rb"));
        TEST_EQUAL(0, acmod_start_utt(acmod));
        ps_search_start(ngs);
        ps_search_start(pls);
        n_searchfr = 0;
        while (!feof(rawfh)) {
            nread = fread(buf, sizeof(*buf), 2048, rawfh);
            bptr = buf;
            while ((nfr = acmod_process_raw(acmod, &bptr, &nread, FALSE)) > 0) {
                while (acmod->n_feat_frame > 0) {
                    ps_search_step(pls, n_searchfr);
                    if (n_searchfr >= 6)
                        ps_search_step(ngs, n_searchfr - 6);
                    acmod_advance(acmod);
                    ++n_searchfr;
                }
            }
        }
        for (nfr = n_searchfr - 6; nfr < n_searchfr; ++nfr) {
            ps_search_step(ngs, nfr);
        }
        ps_search_finish(pls);
        ps_search_finish(ngs);
        printf("%s\n", ps_search_hyp(ngs, &score, NULL));

        TEST_ASSERT(acmod_end_utt(acmod) >= 0);
        fclose(rawfh);
    }
    printf("%s\n", ps_search_hyp(ngs, &score, NULL));
    TEST_EQUAL(0, strcmp("a former ten leaders", ps_search_hyp(ngs, &score, NULL)));
    c = clock() - c;
    printf("5 * fwdtree search in %.2f sec\n",
           (double)c / CLOCKS_PER_SEC);
    ps_free(ps);
    cmd_ln_free_r(config);

    return 0;
}
