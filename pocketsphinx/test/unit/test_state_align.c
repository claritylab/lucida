#include <pocketsphinx.h>

#include "ps_alignment.h"
#include "state_align_search.h"
#include "pocketsphinx_internal.h"

#include "test_macros.h"

static void
do_search(ps_search_t *search, acmod_t *acmod)
{
    FILE *rawfh;
    int16 buf[2048];
    size_t nread;
    int16 const *bptr;
    int nfr;

    TEST_ASSERT(rawfh = fopen(DATADIR "/goforward.raw", "rb"));
    TEST_EQUAL(0, acmod_start_utt(acmod));
    ps_search_start(search);
    while (!feof(rawfh)) {
        nread = fread(buf, sizeof(*buf), 2048, rawfh);
        bptr = buf;
        while ((nfr = acmod_process_raw(acmod, &bptr, &nread, FALSE)) > 0) {
            while (acmod->n_feat_frame > 0) {
                ps_search_step(search, acmod->output_frame);
                acmod_advance(acmod);
            }
        }
    }
    ps_search_finish(search);
    TEST_ASSERT(acmod_end_utt(acmod) >= 0);
    fclose(rawfh);
}


int
main(int argc, char *argv[])
{
    ps_decoder_t *ps;
    bin_mdef_t *mdef;
    dict_t *dict;
    dict2pid_t *d2p;
    acmod_t *acmod;
    ps_alignment_t *al;
    ps_alignment_iter_t *itor;
    ps_search_t *search;
    state_align_search_t *sas;
    cmd_ln_t *config;
    int i;

    config = cmd_ln_init(NULL, ps_args(), FALSE,
                 "-hmm", MODELDIR "/hmm/en_US/hub4wsj_sc_8k",
                 "-dict", MODELDIR "/lm/en_US/cmu07a.dic",
                 "-input_endian", "little",
                 "-samprate", "16000", NULL);
    TEST_ASSERT(ps = ps_init(config));
    dict = ps->dict;
    d2p = ps->d2p;
    acmod = ps->acmod;
    mdef = d2p->mdef;

    al = ps_alignment_init(d2p);
    TEST_EQUAL(1, ps_alignment_add_word(al, dict_wordid(dict, "<s>"), 0));
    TEST_EQUAL(2, ps_alignment_add_word(al, dict_wordid(dict, "go"), 0));
    TEST_EQUAL(3, ps_alignment_add_word(al, dict_wordid(dict, "forward"), 0));
    TEST_EQUAL(4, ps_alignment_add_word(al, dict_wordid(dict, "ten"), 0));
    TEST_EQUAL(5, ps_alignment_add_word(al, dict_wordid(dict, "meters"), 0));
    TEST_EQUAL(6, ps_alignment_add_word(al, dict_wordid(dict, "</s>"), 0));
    TEST_EQUAL(0, ps_alignment_populate(al));

    TEST_ASSERT(search = state_align_search_init(config, acmod, al));
    sas = (state_align_search_t *)search;

    for (i = 0; i < 5; ++i)
        do_search(search, acmod);

    itor = ps_alignment_words(al);
    TEST_EQUAL(ps_alignment_iter_get(itor)->start, 0);
    TEST_EQUAL(ps_alignment_iter_get(itor)->duration, 3);
    itor = ps_alignment_iter_next(itor);
    TEST_EQUAL(ps_alignment_iter_get(itor)->start, 3);
    TEST_EQUAL(ps_alignment_iter_get(itor)->duration, 6);
    itor = ps_alignment_iter_next(itor);
    TEST_EQUAL(ps_alignment_iter_get(itor)->start, 9);
    TEST_EQUAL(ps_alignment_iter_get(itor)->duration, 41);
    itor = ps_alignment_iter_next(itor);
    TEST_EQUAL(ps_alignment_iter_get(itor)->start, 50);
    TEST_EQUAL(ps_alignment_iter_get(itor)->duration, 37);
    itor = ps_alignment_iter_next(itor);
    TEST_EQUAL(ps_alignment_iter_get(itor)->start, 87);
    TEST_EQUAL(ps_alignment_iter_get(itor)->duration, 57);
    itor = ps_alignment_iter_next(itor);
    TEST_EQUAL(ps_alignment_iter_get(itor)->start, 144);
    TEST_EQUAL(ps_alignment_iter_get(itor)->duration, 31);
    itor = ps_alignment_iter_next(itor);
    TEST_EQUAL(itor, NULL);

    ps_search_free(search);
    ps_alignment_free(al);
    ps_free(ps);
    cmd_ln_free_r(config);
    return 0;
}
