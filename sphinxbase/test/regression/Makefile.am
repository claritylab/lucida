EXTRA_DIST =					\
	testfuncs_cygwin.sh			\
	chan3.cepview				\
	chan3-dither.cepview			\
	chan3.logspec				\
	chan3-logspec.cepview			\
	chan3-smoothspec.cepview		\
	chan3.ctl				\
	chan3.mfc				\
	chan3.raw				\
	chan3.2chan.wav				\
	chan3.wav				\
	chan3.sph				\
	chan3.f0				\
	tutorial-check.sh			\
	crontab					\
	polite.gram				\
	test.gram				\
	test.command.fsg			\
	test.kleene.fsg				\
	test.nestedRightRecursion.fsg		\
	test.nulltest.fsg			\
	test.rightRecursion.fsg

CLEANFILES = test*.err test*.out		\
	test-sphinx_fe.mfc			\
	test-sphinx_fe.cepview			\
	test-sphinx_pitch.f0			\
	chan3.sph.mfc				\
	chan3.2chan.wav.mfc			\
	chan3.wav.mfc				\
	chan3.raw.mfc

# Disable sphinx_fe tests for now if fixed-point due to imprecision
if FIXED_POINT
TESTS = test-cepview.sh test-sphinx_pitch.sh
else
TESTS = \
	test-cepview.sh \
	test-sphinx_fe-ctl.sh \
	test-sphinx_fe-dct.sh \
	test-sphinx_fe-dither-seed.sh \
	test-sphinx_fe-logspec2cep.sh \
	test-sphinx_fe-logspec.sh \
	test-sphinx_fe.sh \
	test-sphinx_fe-smoothspec.sh \
	test-sphinx_jsgf2fsg.sh \
	test-sphinx_pitch.sh
endif

EXTRA_DIST += $(TESTS)
