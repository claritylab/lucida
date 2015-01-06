# $Id:$

lib_LTLIBRARIES = libcqdb.la

EXTRA_DIST = \
	cqdb.vcxproj \
	COPYING

libcqdb_la_SOURCES = \
	src/lookup3.c \
	include/cqdb.h \
	src/cqdb.c

libcqdb_la_LDFLAGS = \
	-no-undefined \
	-release @VERSION@

libcqdb_la_CFLAGS = -I./include

AM_CFLAGS = @CFLAGS@
INCLUDES = @INCLUDES@
