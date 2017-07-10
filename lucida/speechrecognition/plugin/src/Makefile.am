# Note: plugindir is set in configure
plugin_LTLIBRARIES = libgstasrplugin.la

# sources used to compile this plug-in
libgstasrplugin_la_SOURCES = gstasrplugin.c gstasrplugin.h

# compiler and linker flags used to compile this plugin, set in configure.ac
libgstasrplugin_la_CFLAGS = $(GST_CFLAGS)
libgstasrplugin_la_LIBADD = $(GST_LIBS)
libgstasrplugin_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstasrplugin_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = gstasrplugin.h
