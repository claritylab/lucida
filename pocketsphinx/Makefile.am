SUBDIRS = src doc model include test swig

EXTRA_DIST = \
	autogen.sh \
	group/ABLD.BAT \
	group/bld.inf \
	group/pocketsphinx.mmp \
	m4/pkg.m4 \
	pocketsphinx.pc.in \
	pocketsphinx.sln \
	win32/pocketsphinx/pocketsphinx.vcxproj \
	win32/pocketsphinx/pocketsphinx.vcxproj.filters \
	win32/pocketsphinx_batch/pocketsphinx_batch.vcxproj \
	win32/pocketsphinx_continuous/pocketsphinx_continuous.vcxproj \
	win32/pocketsphinx_mdef_convert/pocketsphinx_mdef_convert.vcxproj

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = pocketsphinx.pc
CLEANFILES = pocketsphinx.pc

ACLOCAL_AMFLAGS = -I m4
