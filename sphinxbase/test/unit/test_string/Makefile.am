check_PROGRAMS = strtest test_atof

INCLUDES = \
	-I$(top_srcdir)/include/sphinxbase \
	-I$(top_srcdir)/include \
	-I$(top_builddir)/include \
	-DTESTDATADIR=\"$(srcdir)\"

LDADD = ${top_builddir}/src/libsphinxbase/libsphinxbase.la

TESTS = _fread_line.test			\
	_nextword.test				\
	_str2words.test				\
	_string_join.test			\
	_string_trim.test			\
	test_atof

EXTRA_DIST = _fread_line.txt			\
	 _fread_line.test			\
	_nextword.test				\
	_str2words.test				\
	_string_join.test			\
	_string_trim.test
