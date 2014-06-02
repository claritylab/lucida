EXTRA_DIST = README Makefile.msvc.in merge-models.pl
EXTRA_DIRS = doc example sdk perl python ruby java swig
bin_PROGRAMS = crf_learn crf_test
ACLOCAL_AMFLAGS = -I m4

AUTOMAKE_OPTIONS = no-dependencies
lib_LTLIBRARIES = libcrfpp.la
libcrfpp_la_SOURCES = crfpp.h thread.h libcrfpp.cpp lbfgs.cpp scoped_ptr.h param.cpp param.h encoder.cpp feature.cpp stream_wrapper.h \
                      feature_cache.cpp feature_index.cpp node.cpp path.cpp tagger.cpp \
		      common.h darts.h encoder.h feature_cache.h feature_index.h \
                      freelist.h lbfgs.h mmap.h node.h path.h tagger.h timer.h winmain.h
include_HEADERS = crfpp.h

dist-hook:
	for subdir in $(EXTRA_DIRS); do \
	  cp -rp $$subdir $(distdir); \
	  rm -f $(distdir)/$$subdir/*~; \
	  rm -f $(distdir)/$$subdir/*.{bak,orig}; \
	  rm -rf $(distdir)/$$subdir/CVS; \
	  rm -rf $(distdir)/$$subdir/.svn; \
	  rm -rf $(distdir)/.svn; \
	  rm -rf $(distdir)/*/.svn; \
	  rm -rf $(distdir)/*/*/.svn; \
	  rm -rf $(distdir)/$$subdir/*/CVS; \
	  rm -rf $(distdir)/$$subdir/*/.svn; \
	  find $(distdir) -name .svn  | xargs rm -fr; \
	done

win-dist:
	rm -f @PACKAGE@-@VERSION@.zip
	mkdir -p @PACKAGE@-@VERSION@/doc
	mkdir -p @PACKAGE@-@VERSION@/example
	mkdir -p @PACKAGE@-@VERSION@/sdk
	cp -f crf_learn.exe @PACKAGE@-@VERSION@
	cp -f crf_test.exe @PACKAGE@-@VERSION@
	cp -f libcrfpp.dll @PACKAGE@-@VERSION@
	cp -f libcrfpp.lib @PACKAGE@-@VERSION@/sdk
	cp -f crfpp.h @PACKAGE@-@VERSION@/sdk
#	cp -f sdk/model @PACKAGE@-@VERSION@/sdk
	cp -f sdk/example.cpp @PACKAGE@-@VERSION@/sdk	
	cp -fr doc/*.html doc/*.css doc/doxygen @PACKAGE@-@VERSION@/doc
	cp -f README COPYING AUTHORS LGPL BSD @PACKAGE@-@VERSION@
	cp -fr example/* @PACKAGE@-@VERSION@/example
	find @PACKAGE@-@VERSION@ -name CVS | xargs rm -rf
	find @PACKAGE@-@VERSION@ -name .svn | xargs rm -rf	
	zip -r @PACKAGE@-@VERSION@.zip @PACKAGE@-@VERSION@
	rm -fr @PACKAGE@-@VERSION@

crf_learn_SOURCES = crf_learn.cpp 
crf_learn_LDADD = libcrfpp.la

crf_test_SOURCES = crf_test.cpp 
crf_test_LDADD = libcrfpp.la 

dist-all-package:
	(test -f Makefile) && $(MAKE) distclean
	./configure
	$(MAKE) dist
	$(MAKE) clean
	$(MAKE) script-clean
	$(MAKE) script-dist
	$(MAKE) win-dist

script-dist:
	for subdir in perl ruby python java; do \
	  rm -fr CRF++-$${subdir}-@VERSION@; \
	  mkdir CRF++-$${subdir}-@VERSION@; \
	  cp -r $${subdir}/* CRF++-$${subdir}-@VERSION@; \
	  cp -r doc/bindings.html  CRF++-$${subdir}-@VERSION@; \
	  find ./CRF++-$${subdir}-@VERSION@ -type d -name CVS | xargs rm -fr; \
	  rm -f CRF++-$${subdir}-@VERSION@/*~; \
	  tar zcfv CRF++-$${subdir}-@VERSION@.tar.gz CRF++-$${subdir}-@VERSION@; \
	  rm -fr CRF++-$${subdir}-@VERSION@; \
	done

script-clean:
	(cd perl; $(MAKE) clean; rm -f Makefile.old Makefile)
	(cd ruby; $(MAKE) clean; rm -f Makefile *.log)
	(cd python; python setup.py clean --all)
	(cd java; $(MAKE) clean;)

export-package:
	python googlecode_upload.py -p crfpp -s crfpp      -u taku@chasen.org -w `cat ~/.googlecode_upload_password` CRF++-@VERSION@.tar.gz
	python googlecode_upload.py -p crfpp -s crfpp-win  -u taku@chasen.org -w `cat ~/.googlecode_upload_password` CRF++-@VERSION@.zip
