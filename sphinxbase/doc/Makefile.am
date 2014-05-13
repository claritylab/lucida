EXTRA_DIST = \
	args2man.pl \
	doxy2swig.py \
	sphinx_cepview.1.in \
	sphinx_cont_adseg.1 \
	sphinx_cont_fileseg.1 \
	sphinx_fe.1.in \
	sphinx_lm_sort.1 \
	sphinx_pitch.1.in

if BUILD_SWIG
SWIG_DOC = pydoc.i

pydoc.i: html/index.html
	$(PYTHON) $(srcdir)/doxy2swig.py -n xml/index.xml pydoc.i
endif

if BUILD_DOXYGEN
all-local: html/index.html $(SWIG_DOC)
endif

headers = \
	$(top_srcdir)/include/sphinxbase/ad.h \
	$(top_srcdir)/include/sphinxbase/agc.h \
	$(top_srcdir)/include/sphinxbase/bio.h \
	$(top_srcdir)/include/sphinxbase/bitvec.h \
	$(top_srcdir)/include/sphinxbase/byteorder.h \
	$(top_srcdir)/include/sphinxbase/case.h \
	$(top_srcdir)/include/sphinxbase/ckd_alloc.h \
	$(top_srcdir)/include/sphinxbase/clapack_lite.h \
	$(top_srcdir)/include/sphinxbase/cmd_ln.h \
	$(top_srcdir)/include/sphinxbase/cmn.h \
	$(top_srcdir)/include/sphinxbase/cont_ad.h \
	$(top_srcdir)/include/sphinxbase/err.h \
	$(top_srcdir)/include/sphinxbase/f2c.h \
	$(top_srcdir)/include/sphinxbase/fe.h \
	$(top_srcdir)/include/sphinxbase/feat.h \
	$(top_srcdir)/include/sphinxbase/filename.h \
	$(top_srcdir)/include/sphinxbase/fixpoint.h \
	$(top_srcdir)/include/sphinxbase/fsg_model.h \
	$(top_srcdir)/include/sphinxbase/genrand.h \
	$(top_srcdir)/include/sphinxbase/glist.h \
	$(top_srcdir)/include/sphinxbase/hash_table.h \
	$(top_srcdir)/include/sphinxbase/heap.h \
	$(top_srcdir)/include/sphinxbase/jsgf.h \
	$(top_srcdir)/include/sphinxbase/listelem_alloc.h \
	$(top_srcdir)/include/sphinxbase/logmath.h \
	$(top_srcdir)/include/sphinxbase/matrix.h \
	$(top_srcdir)/include/sphinxbase/mmio.h \
	$(top_srcdir)/include/sphinxbase/mulaw.h \
	$(top_srcdir)/include/sphinxbase/ngram_model.h \
	$(top_srcdir)/include/sphinxbase/pio.h \
	$(top_srcdir)/include/sphinxbase/prim_type.h \
	$(top_srcdir)/include/sphinxbase/profile.h \
	$(top_srcdir)/include/sphinxbase/sphinxbase_export.h \
	$(top_srcdir)/include/sphinxbase/strfuncs.h \
	$(top_srcdir)/include/sphinxbase/yin.h

latex/refman.pdf: doxyfile $(headers)
	doxygen
	$(MAKE) -C latex refman.pdf

html/index.html: doxyfile $(headers)
	doxygen

clean-local:
	-rm -rf html latex xml doxytags $(SWIG_DOC)

