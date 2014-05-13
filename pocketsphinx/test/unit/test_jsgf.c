#include <pocketsphinx.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <sphinxbase/jsgf.h>
#include <sphinxbase/fsg_model.h>

#include "pocketsphinx_internal.h"
#include "fsg_search_internal.h"
#include "test_macros.h"

int
main(int argc, char *argv[])
{
    ps_decoder_t *ps;
    cmd_ln_t *config;
    acmod_t *acmod;
    fsg_search_t *fsgs;
    jsgf_t *jsgf;
    jsgf_rule_t *rule;
    fsg_model_t *fsg;
    ps_seg_t *seg;
    ps_lattice_t *dag;
    FILE *rawfh;
    char const *hyp, *uttid;
    int32 score, prob;
    clock_t c;
    int i;

    TEST_ASSERT(config =
            cmd_ln_init(NULL, ps_args(), TRUE,
                "-hmm", MODELDIR "/hmm/en_US/hub4wsj_sc_8k",
                "-dict", MODELDIR "/lm/en/turtle.dic",
                "-input_endian", "little",
                "-samprate", "16000", NULL));
    TEST_ASSERT(ps = ps_init(config));

    jsgf = jsgf_parse_file(DATADIR "/goforward.gram", NULL);
    TEST_ASSERT(jsgf);
    rule = jsgf_get_rule(jsgf, "<goforward.move2>");
    TEST_ASSERT(rule);
    fsg = jsgf_build_fsg(jsgf, rule, ps->lmath, 7.5);
    TEST_ASSERT(fsg);
    fsg_model_write(fsg, stdout);
  ps_set_fsg(ps, "<goforward.move2>", fsg);
  ps_set_search(ps, "<goforward.move2>"); 

    acmod = ps->acmod;
  fsgs = (fsg_search_t *) fsg_search_init(fsg, config, acmod, ps->dict, ps->d2p);

    setbuf(stdout, NULL);
    c = clock();
    for (i = 0; i < 5; ++i) {
        int16 buf[2048];
        size_t nread;
        int16 const *bptr;
        int nfr;
        int is_final;

        TEST_ASSERT(rawfh = fopen(DATADIR "/goforward.raw", "rb"));
        TEST_EQUAL(0, acmod_start_utt(acmod));
        fsg_search_start(ps_search_base(fsgs));
        is_final = FALSE;
        while (!feof(rawfh)) {
            nread = fread(buf, sizeof(*buf), 2048, rawfh);
            bptr = buf;
            while ((nfr = acmod_process_raw(acmod, &bptr, &nread, FALSE)) > 0) {
                while (acmod->n_feat_frame > 0) {
                    fsg_search_step(ps_search_base(fsgs),
                            acmod->output_frame);
                    acmod_advance(acmod);
                }
            }
            hyp = fsg_search_hyp(ps_search_base(fsgs), &score, &is_final);
            printf("FSG: %s (%d) frame %d final %s\n", hyp, score, acmod->output_frame, is_final ? "FINAL" : "");
            TEST_EQUAL (is_final, (acmod->output_frame > 170));
        }
        fsg_search_finish(ps_search_base(fsgs));
        hyp = fsg_search_hyp(ps_search_base(fsgs), &score, NULL);
        printf("FSG: %s (%d)\n", hyp, score);

        TEST_ASSERT(acmod_end_utt(acmod) >= 0);
        fclose(rawfh);
    }
    TEST_EQUAL(0, strcmp("go forward ten meters",
                 fsg_search_hyp(ps_search_base(fsgs), &score, NULL)));
    ps->search = (ps_search_t *)fsgs;
    for (seg = ps_seg_iter(ps, &score); seg;
         seg = ps_seg_next(seg)) {
        char const *word;
        int sf, ef;

        word = ps_seg_word(seg);
        ps_seg_frames(seg, &sf, &ef);
        printf("%s %d %d\n", word, sf, ef);
    }
    c = clock() - c;
    printf("5 * fsg search in %.2f sec\n", (double)c / CLOCKS_PER_SEC);

    dag = ps_get_lattice(ps);
    ps_lattice_write(dag, "test_jsgf.lat");
    jsgf_grammar_free(jsgf);
    fsg_search_free(ps_search_base(fsgs));
    ps_free(ps);
    cmd_ln_free_r(config);

    TEST_ASSERT(config =
            cmd_ln_init(NULL, ps_args(), TRUE,
                "-hmm", MODELDIR "/hmm/en_US/hub4wsj_sc_8k",
                "-dict", MODELDIR "/lm/en/turtle.dic",
                "-jsgf", DATADIR "/goforward.gram",
                "-input_endian", "little",
                "-samprate", "16000", NULL));
    TEST_ASSERT(ps = ps_init(config));
    TEST_ASSERT(rawfh = fopen(DATADIR "/goforward.raw", "rb"));
    ps_decode_raw(ps, rawfh, "goforward", -1);
    hyp = ps_get_hyp(ps, &score, &uttid);
    prob = ps_get_prob(ps, &uttid);
    printf("%s: %s (%d, %d)\n", uttid, hyp, score, prob);
    TEST_EQUAL(0, strcmp("go forward ten meters", hyp));
    ps_free(ps);
    fclose(rawfh);
    cmd_ln_free_r(config);

    TEST_ASSERT(config =
            cmd_ln_init(NULL, ps_args(), TRUE,
                "-hmm", MODELDIR "/hmm/en_US/hub4wsj_sc_8k",
                "-dict", MODELDIR "/lm/en/turtle.dic",
                "-jsgf", DATADIR "/goforward.gram",
                "-toprule", "goforward.move2",
                "-input_endian", "little",
                "-samprate", "16000", NULL));
    TEST_ASSERT(ps = ps_init(config));
    TEST_ASSERT(rawfh = fopen(DATADIR "/goforward.raw", "rb"));
    ps_decode_raw(ps, rawfh, "goforward", -1);
    hyp = ps_get_hyp(ps, &score, &uttid);
    prob = ps_get_prob(ps, &uttid);
    printf("%s: %s (%d, %d)\n", uttid, hyp, score, prob);
    TEST_EQUAL(0, strcmp("go forward ten meters", hyp));
    ps_free(ps);
    cmd_ln_free_r(config);
    fclose(rawfh);

    TEST_ASSERT(config =
            cmd_ln_init(NULL, ps_args(), TRUE,
                "-hmm", MODELDIR "/hmm/en_US/hub4wsj_sc_8k",
                "-dict", MODELDIR "/lm/en/turtle.dic",
                "-jsgf", DATADIR "/defective.gram",
                NULL));
    TEST_ASSERT(NULL == ps_init(config));
    cmd_ln_free_r(config);

    return 0;
}
