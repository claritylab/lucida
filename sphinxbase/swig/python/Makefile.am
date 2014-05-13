if BUILD_SWIG

SWIG_FLAGS = -I$(top_srcdir)/include

SWIG_DIR = $(top_srcdir)/swig

SWIG_FILES = \
	$(SWIG_DIR)/sphinxbase.i \
	$(SWIG_DIR)/cmd_ln.i \
	$(SWIG_DIR)/fe.i \
	$(SWIG_DIR)/feat.i \
	$(SWIG_DIR)/jsgf.i \
	$(SWIG_DIR)/fsg_model.i \
	$(SWIG_DIR)/ngram_model.i \
	$(SWIG_DIR)/python.i \
	$(SWIG_DIR)/typemaps.i

if BUILD_DOXYGEN
SWIG_FLAGS += -DHAS_DOC -I$(top_builddir)/doc
SWIG_FILES += $(top_builddir)/doc/pydoc.i
endif

CLEANFILES = sphinxbase_wrap.c sphinxbase.py sphinxbase.pyc

pkgpyexec_PYTHON = __init__.py 
nodist_pkgpyexec_PYTHON = sphinxbase.py
pkgpyexec_LTLIBRARIES = _sphinxbase.la
nodist__sphinxbase_la_SOURCES = sphinxbase_wrap.c

_sphinxbase_la_CPPFLAGS = \
	$(PYTHON_CPPFLAGS) \
	-I$(top_srcdir)/include \
	-D_DATADIR=\"$(datadir)/@PACKAGE@\" \
	-Wno-unused-value

_sphinxbase_la_LDFLAGS = -module
_sphinxbase_la_LIBADD = $(top_builddir)/src/libsphinxbase/libsphinxbase.la

sphinxbase_wrap.c: $(SWIG_FILES)
	$(SWIG) $(SWIG_FLAGS) -outdir . -o $@ -python $<

endif
