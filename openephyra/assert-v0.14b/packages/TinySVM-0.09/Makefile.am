# Process this file with automake to produce Makefile.in
#
# $Id: Makefile.am,v 1.26 2001/12/07 10:54:21 taku-ku Exp $;
#
@SET_MAKE@
AUTOMAKE_OPTIONS = 1.4 no-dependencies

always_built_SUBDIRS = src tests man doc
EXTRA_DIRS = doc perl ruby python java
EXTRA_DIST = tinysvm.spec THANKS

SUBDIRS = $(always_built_SUBDIRS)

pkgdatadir = @datadir@/@PACKAGE@

clean-swig:
	(cd perl; rm -f Makefile.PL Makefile.old model* *.so *.o Makefile)
	(cd ruby; rm -f extconf.rb model* Makefile *.o *.so)
	(cd python; rm -fr Makefile Makefile.pre Setup *.o model* config.c sedscript *.pyc *.so)
	(cd java; rm -fr *.class *.jar model* Makefile *.so *.o)

dist-hook: clean-swig
	for subdir in $(EXTRA_DIRS); do \
		cp -rp $$subdir $(distdir)/$$subdir; \
		rm -f $(distdir)/$$subdir/*~; \
		rm -f $(distdir)/$$subdir/*.{bak,orig,old}; \
		rm -f $(distdir)/$$subdir/Makefile.PL; \
		rm -rf $(distdir)/$$subdir/CVS; \
		rm -rf $(distdir)/$$subdir/*/CVS; \
	done

update-man:
	cd man; make update-man
	cd doc; make update-html

rpm: dist
	rpm -ta @PACKAGE@-@VERSION@.tar.gz
	
win:
	make distclean
	env AR=i386-mingw32msvc-ar RANLIB=i386-mingw32msvc-ranlib CXX=i386-mingw32msvc-g++ CC=i386-mingw32msvc-gcc \
	./configure --host=i386-pc-mingw32  --target=i386-pc-mingw32 --disable-shared
	make

win-dist:
	rm -f @PACKAGE@-@VERSION@.zip
	mkdir bin;
	cp -f src/svm_learn.exe    bin/svm_learn.exe
	cp -f src/svm_classify.exe bin/svm_classify.exe
	cp -f src/svm_model.exe    bin/svm_model.exe

	zip @PACKAGE@-@VERSION@.zip README AUTHORS NEWS COPYING THANKS ChangeLog \
	bin/*.exe doc/*.html doc/*.css tests/*.svmdata
	rm -fr bin/
	
dist-all-package:
	(test -f Makefile) && make distclean
	./configure
	make dist
	make rpm
	make clean
	make win
	make win-dist
