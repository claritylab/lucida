my_plugins = 
my_headers =
my_files =
if BUILD_GST
my_plugins += libgstpocketsphinx.la
my_headers += psmarshal.h
my_files += psmarshal.h psmarshal.c
endif

plugin_LTLIBRARIES = $(my_plugins)

libgstpocketsphinx_la_SOURCES = gstpocketsphinx.c gstvader.c psmarshal.c
libgstpocketsphinx_la_CFLAGS = $(GST_CFLAGS) -DMODELDIR=\"$(pkgdatadir)/model\"
libgstpocketsphinx_la_LIBADD = $(GST_LIBS) -lgstaudio-$(GST_MAJORMINOR) \
	$(top_builddir)/src/libpocketsphinx/libpocketsphinx.la -lsphinxbase
libgstpocketsphinx_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)

noinst_HEADERS = gstpocketsphinx.h gstvader.h $(my_headers)

INCLUDES = -I$(top_srcdir)/include \
	-I$(top_builddir)/include

EXTRA_DIST = psmarshal.list livedemo.py fixsqrt.py
BUILT_SOURCES = $(my_files)
CLEANFILES = $(my_filse)

psmarshal.h: psmarshal.list
	glib-genmarshal --header --prefix=ps_marshal $(srcdir)/psmarshal.list > psmarshal.h.tmp
	mv psmarshal.h.tmp psmarshal.h

psmarshal.c: psmarshal.list
	echo "#include \"glib-object.h\"" >> psmarshal.c.tmp
	echo "#include \"psmarshal.h\"" >> psmarshal.c.tmp
	glib-genmarshal --body --prefix=ps_marshal $(srcdir)/psmarshal.list >> psmarshal.c.tmp
	mv psmarshal.c.tmp psmarshal.c
