#include <pocketsphinx.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "pocketsphinx_internal.h"
#include "ngram_search_fwdtree.h"
#include "ps_lattice_internal.h"
#include "test_macros.h"

int
main(int argc, char *argv[])
{
    ps_decoder_t *ps;
    cmd_ln_t *config;
    acmod_t *acmod;
    ngram_search_t *ngs;
    ps_lattice_t *dag;
    clock_t c;
    int i;

    TEST_ASSERT(config =
            cmd_ln_init(NULL, ps_args(), TRUE,
                "-hmm", MODELDIR "/hmm/en_US/hub4wsj_sc_8k",
                "-lm", MODELDIR "/lm/en_US/wsj0vp.5000.DMP",
                "-dict", MODELDIR "/lm/en_US/cmu07a.dic",
                "-fwdtree", "yes",
                "-fwdflat", "no",
                "-bestpath", "yes",
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
        ps_astar_t *nbest;
        ps_latpath_t *path;
        char *besthyp;
        char *astar_besthyp;
        int32 astar_hyp_score;
        int i;

        /* PocketSphinx API would do this for us but we have to do it manually here. */
        ps_lattice_free(ps->search->dag);
        ps->search->dag = NULL;

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
        ngram_fwdtree_finish(ngs);
        printf("FWDTREE: %s\n",
               ngram_search_bp_hyp(ngs, ngram_search_find_exit(ngs, -1, NULL, NULL)));

        TEST_ASSERT(acmod_end_utt(acmod) >= 0);
        fclose(rawfh);

        dag = ngram_search_lattice(ps->search);
        if (dag == NULL) {
            E_ERROR("Failed to build DAG!\n");
            return 1;
        }
        besthyp = ckd_salloc
            (ps_lattice_hyp(dag, ps_lattice_bestpath
                    (dag, ngs->lmset, 6.5, 1.0)));
        printf("BESTPATH: %s\n", besthyp);

        TEST_ASSERT(nbest = ps_astar_start(dag, ngs->lmset, 6.5, 0, -1, -1, -1));
        i = 0;
        astar_hyp_score = WORST_SCORE;
        while ((path = ps_astar_next(nbest))) {
            if (i < 10)
                printf("NBEST %d: %s\n", i, ps_astar_hyp(nbest, path));
            if (path->score > astar_hyp_score) {
                astar_hyp_score = path->score;
                astar_besthyp = ps_astar_hyp(nbest, path);
            }
            i++;
        }
        TEST_EQUAL(0, strcmp(besthyp, astar_besthyp));
        ps_astar_finish(nbest);
        ckd_free(besthyp);
    }
    printf("%s\n", ngram_search_bp_hyp(ngs, ngram_search_find_exit(ngs, -1, NULL, NULL)));
    TEST_EQUAL(0, strcmp("a former ten new shares",
                 ngram_search_bp_hyp(ngs, ngram_search_find_exit(ngs, -1, NULL, NULL))));
    c = clock() - c;
    printf("5 * fwdtree + bestpath + N-best search in %.2f sec\n",
           (double)c / CLOCKS_PER_SEC);
    ps_free(ps);
    cmd_ln_free_r(config);

    return 0;
}
