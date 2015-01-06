# $Id:$

bin_PROGRAMS = crfsuite
#man_MANS = crfsuite.1
#EXTRA_DIST = ${man_MANS}

EXTRA_DIST = \
	frontend.vcxproj

crfsuite_SOURCES = \
	iwa.h \
	iwa.c \
	option.h \
	option.c \
	readdata.h \
	reader.c \
	learn.c \
	tag.c \
	dump.c \
	main.c

#crfsuite_CPPFLAGS =

AM_CFLAGS = @CFLAGS@
INCLUDES = @INCLUDES@
AM_LDFLAGS = @LDFLAGS@

crfsuite_CFLAGS = -I$(top_builddir)/include
crfsuite_LDADD = $(top_builddir)/lib/crf/libcrfsuite.la
