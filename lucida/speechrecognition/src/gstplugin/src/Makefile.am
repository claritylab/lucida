# Note: plugindir is set in configure
plugin_LTLIBRARIES = libgstasrplugin.la

# sources used to compile this plug-in
libgstasrplugin_la_SOURCES = gstasrplugin.c gstasrplugin.h ../../../include/gen-c_glib/a_s_r_thrift_service.h ../../../include/gen-c_glib/asrthriftservice_types.h ../../../include/gen-c_glib/a_s_r_thrift_service.c ../../../include/gen-c_glib/asrthriftservice_types.c

# compiler and linker flags used to compile this plugin, set in configure.ac
libgstasrplugin_la_CFLAGS = $(GST_CFLAGS) -I ../../../include -I ../../../include/gen-c_glib -I/usr/local/include/thrift
libgstasrplugin_la_LIBADD = $(GST_LIBS)
libgstasrplugin_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS) -L/usr/local/lib -lthrift -lthrift_c_glib -lthriftprotocol
libgstasrplugin_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = gstasrplugin.h
