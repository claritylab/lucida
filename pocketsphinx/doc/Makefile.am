man_MANS = \
	pocketsphinx_batch.1 \
	pocketsphinx_continuous.1 \
	pocketsphinx_mdef_convert.1

EXTRA_DIST = \
	args2man.pl \
	doxy2swig.py \
	pocketsphinx_batch.1 \
	pocketsphinx_continuous.1 \
	pocketsphinx_mdef_convert.1

# pocketsphinx_batch.1: pocketsphinx_batch.1.in
# 	$(srcdir)/args2man.pl $(top_builddir)/src/programs/pocketsphinx_batch \
# 		< $< > $@

# pocketsphinx_continuous.1: pocketsphinx_continuous.1.in
# 	$(srcdir)/args2man.pl $(top_builddir)/src/programs/pocketsphinx_continuous \
# 		< $< > $@

if BUILD_SWIG
SWIG_DOC = pydoc.i

pydoc.i: html/index.html
	$(PYTHON) $(srcdir)/doxy2swig.py -n xml/index.xml pydoc.i
endif

if BUILD_DOXYGEN
all-local: html/index.html $(SWIG_DOC)
endif

headers = $(top_srcdir)/include/pocketsphinx.h \
	$(top_srcdir)/include/ps_lattice.h

latex/refman.pdf: doxyfile $(headers)
	doxygen
	$(MAKE) -C latex refman.pdf

html/index.html: doxyfile $(headers)
	doxygen

clean-local:
	-rm -rf html xml latex doxytags $(SWIG_DOC)

# Totally CMU-specific rule for uploading documentation
upload: html/index.html
	rsync -av html/ file:/usr12/apache2/htdocs/sphinx/doc/doxygen/pocketsphinx/
