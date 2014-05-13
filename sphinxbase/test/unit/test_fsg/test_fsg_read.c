#include <fsg_model.h>

#include "test_macros.h"

int
main(int argc, char *argv[])
{
	logmath_t *lmath;
	fsg_model_t *fsg;
	fsg_arciter_t *itor;

	/* Initialize a logmath object to pass to fsg_model_read */
	lmath = logmath_init(1.0001, 0, 0);
	/* Read a FSG. */
	fsg = fsg_model_readfile(LMDIR "/goforward.fsg", lmath, 7.5);
	TEST_ASSERT(fsg);

	TEST_ASSERT(fsg_model_add_silence(fsg, "<sil>", -1, 0.3));
	TEST_ASSERT(fsg_model_add_silence(fsg, "++NOISE++", -1, 0.3));
	TEST_ASSERT(fsg_model_add_alt(fsg, "FORWARD", "FORWARD(2)"));

	fsg_model_write(fsg, stdout);

	/* Test reference counting. */
	TEST_ASSERT(fsg = fsg_model_retain(fsg));
	TEST_EQUAL(1, fsg_model_free(fsg));
	fsg_model_write(fsg, stdout);

	/* Test iteration. */
	for (itor = fsg_model_arcs(fsg, 3);
	     itor; itor = fsg_arciter_next(itor)) {
		fsg_link_t *link = fsg_arciter_get(itor);

		TEST_EQUAL(fsg_link_from_state(link), 3);
		if (fsg_link_wid(link) == -1) {
			TEST_EQUAL(fsg_link_to_state(link), 4);
			TEST_EQUAL(fsg_link_logs2prob(link), 0);
		}
		else if (fsg_link_wid(link) == fsg_model_word_id(fsg, "++NOISE++")
			 || fsg_link_wid(link) == fsg_model_word_id(fsg, "<sil>")) {
			TEST_EQUAL(fsg_link_to_state(link), 3);
			TEST_EQUAL_LOG(fsg_link_logs2prob(link), -90300);
		}
		printf("%d => %d %s %d\n",
		       fsg_link_from_state(link),
		       fsg_link_to_state(link),
		       fsg_link_wid(link) == -1
		       ? "&epsilon;" : fsg_model_word_str(fsg, fsg_link_wid(link)),
		       fsg_link_logs2prob(link));
	}

	TEST_EQUAL(0, fsg_model_free(fsg));
	logmath_free(lmath);

	return 0;
}
