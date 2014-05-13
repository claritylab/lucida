bin_PROGRAMS = \
	pocketsphinx_batch \
	pocketsphinx_continuous \
	pocketsphinx_mdef_convert

pocketsphinx_mdef_convert_SOURCES = mdef_convert.c
pocketsphinx_mdef_convert_LDADD = \
	$(top_builddir)/src/libpocketsphinx/libpocketsphinx.la

pocketsphinx_batch_SOURCES = batch.c
pocketsphinx_batch_LDADD = \
	$(top_builddir)/src/libpocketsphinx/libpocketsphinx.la

pocketsphinx_kws_SOURCES = kws.c
pocketsphinx_kws_LDADD = \
	$(top_builddir)/src/libpocketsphinx/libpocketsphinx.la

pocketsphinx_continuous_SOURCES = continuous.c
pocketsphinx_continuous_LDADD = \
	$(top_builddir)/src/libpocketsphinx/libpocketsphinx.la -lsphinxad

INCLUDES = -I$(top_srcdir)/include \
	-I$(top_srcdir)/src/libpocketsphinx \
	-I$(top_builddir)/include
