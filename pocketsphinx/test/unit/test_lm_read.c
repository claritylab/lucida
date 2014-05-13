#include <stdio.h>
#include <string.h>

#include <pocketsphinx.h>

#include "test_macros.h"

int
main(int argc, char *argv[])
{
	char const *hyp;
	char const *uttid;
	ps_decoder_t *ps;
	cmd_ln_t *config;
	ngram_model_t *lmset, *lm;
	FILE *rawfh;
	int32 score;

	/* First decode it with the crappy WSJ language model. */
	TEST_ASSERT(config =
		    cmd_ln_init(NULL, ps_args(), TRUE,
				"-hmm", MODELDIR "/hmm/en_US/hub4wsj_sc_8k",
				"-lm", MODELDIR "/lm/en_US/wsj0vp.5000.DMP",
				"-dict", DATADIR "/defective.dic",
				"-dictcase", "yes",
				/* FIXME: fwdflat causes some problems. */
				"-bestpath", "no", "-fwdflat", "no",
				"-input_endian", "little",
				"-samprate", "16000", NULL));
	TEST_ASSERT(ps = ps_init(config));
	TEST_ASSERT(rawfh = fopen(DATADIR "/goforward.raw", "rb"));

	ps_decode_raw(ps, rawfh, "goforward", -1);
	hyp = ps_get_hyp(ps, &score, &uttid);
	printf("%s: %s (%d)\n", uttid, hyp, score);
	TEST_EQUAL(0, strcmp(hyp, "go forward ten years"));

	/* Now load the turtle language model. */
	lm = ngram_model_read(config, MODELDIR "/lm/en/turtle.DMP",
			                  NGRAM_AUTO, ps_get_logmath(ps));
  ps_set_lm(ps, "turtle", lm);
	ps_set_search(ps, "turtle");
	clearerr(rawfh);
	fseek(rawfh, 0, SEEK_SET);
	TEST_ASSERT(ps_decode_raw(ps, rawfh, "goforward", -1));
	hyp = ps_get_hyp(ps, &score, &uttid);
	printf("%s: %s (%d)\n", uttid, hyp, score);

	/* Oops!  It's still not correct, because METERS isn't in the
	 * dictionary that we originally loaded. */
	TEST_EQUAL(0, strcmp(hyp, "go forward ten degrees"));
	/* So let's add it to the dictionary. */
	ps_add_word(ps, "foobie", "F UW B IY", FALSE);
	ps_add_word(ps, "meters", "M IY T ER Z", TRUE);
	/* And try again. */
	clearerr(rawfh);
	fseek(rawfh, 0, SEEK_SET);
	TEST_ASSERT(ps_decode_raw(ps, rawfh, "goforward", -1));
	hyp = ps_get_hyp(ps, &score, &uttid);
	TEST_ASSERT(hyp);
	printf("%s: %s (%d)\n", uttid, hyp, score);
	ps_lattice_write(ps_get_lattice(ps), "meters.lat");
	/* Bingo! */
	TEST_EQUAL(0, strcmp(hyp, "go forward ten meters"));

	/* Now let's test dictionary switching. */
	TEST_EQUAL(-1, ps_load_dict(ps, MODELDIR "/lm/en/turtle_missing_file.dic",
				   NULL, NULL));

	/* Now let's test dictionary switching. */
	TEST_EQUAL(0, ps_load_dict(ps, MODELDIR "/lm/en/turtle.dic",
				   NULL, NULL));

	/* And try again. */
	clearerr(rawfh);
	fseek(rawfh, 0, SEEK_SET);
	TEST_ASSERT(ps_decode_raw(ps, rawfh, "goforward", -1));
	hyp = ps_get_hyp(ps, &score, &uttid);
	printf("%s: %s (%d)\n", uttid, hyp, score);
	TEST_EQUAL(0, strcmp(hyp, "go forward ten meters"));

	/* Try switching back again just to make sure. */
	TEST_EQUAL(0, ps_load_dict(ps, DATADIR "/defective.dic",
				   NULL, NULL));
	clearerr(rawfh);
	fseek(rawfh, 0, SEEK_SET);
	TEST_ASSERT(ps_decode_raw(ps, rawfh, "goforward", -1));
	hyp = ps_get_hyp(ps, &score, &uttid);
	printf("%s: %s (%d)\n", uttid, hyp, score);
	TEST_EQUAL(0, strcmp(hyp, "go forward ten degrees"));

	fclose(rawfh);
	ps_free(ps);
	cmd_ln_free_r(config);

	return 0;
}

