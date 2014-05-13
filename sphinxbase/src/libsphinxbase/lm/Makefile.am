noinst_LTLIBRARIES = libsphinxlm.la

AM_YFLAGS = -d
BUILT_SOURCES = jsgf_parser.h jsgf_parser.c

libsphinxlm_la_SOURCES =			\
	lm3g_model.c				\
	ngram_model.c				\
	ngram_model_arpa.c			\
	ngram_model_dmp.c			\
	ngram_model_set.c			\
	fsg_model.c				\
	jsgf.c					\
	jsgf_scanner.c				\
	jsgf_parser.y

libsphinxlm_la_LIBADD = $(LTLIBICONV)

noinst_HEADERS = ngram_model_internal.h		\
	ngram_model_dmp.h			\
	ngram_model_set.h			\
	ngram_model_arpa.h			\
	lm3g_model.h				\
	jsgf_internal.h				\
	jsgf_scanner.h				\
	jsgf_parser.h

INCLUDES = -I$(top_srcdir)/include/sphinxbase \
	   -I$(top_srcdir)/include \
	   -I$(top_builddir)/include

# Nobody has the version of Flex needed to regenerate the scanner, so
# we can't give it a name that would induce Make to try to build it.
# Hence the underscore.  Sorry folks.
EXTRA_DIST = _jsgf_scanner.l \
	lm3g_templates.c
