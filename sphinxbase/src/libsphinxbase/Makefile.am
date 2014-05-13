SUBDIRS = util fe feat lm

lib_LTLIBRARIES = libsphinxbase.la

libsphinxbase_la_SOURCES =
# Confusing libtool version info means:
#
# This is API version 2, software release 1, which also supports API version 1
libsphinxbase_la_LDFLAGS = -version-info 2:1:1
libsphinxbase_la_LIBADD = $(LTLIBICONV) \
	util/libsphinxutil.la \
	fe/libsphinxfe.la \
	feat/libsphinxfeat.la \
	lm/libsphinxlm.la
