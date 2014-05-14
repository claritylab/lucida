# Process this file with automake to produce Makefile.in
#
# $Id: Makefile.am,v 1.25 2003/01/30 15:04:35 taku-ku Exp $;
#
@SET_MAKE@
AUTOMAKE_OPTIONS = 1.4 no-dependencies

SUBDIRS = src libexec training tests man doc
EXTRA_DIRS = example
EXTRA_DIST = training/Makefile.in yamcha.spec
bin_SCRIPTS = yamcha-config

dist-hook:
	for subdir in $(EXTRA_DIRS); do \
	  cp -rp $$subdir $(distdir)/$$subdir; \
	  rm -f $(distdir)/$$subdir/*~; \
	  rm -f $(distdir)/$$subdir/*.{bak,orig}; \
	  rm -rf $(distdir)/$$subdir/CVS; \
	  rm -rf $(distdir)/$$subdir/*/CVS; \
	done

rpm: dist
	env PATH="/usr/bin:$$PATH" rpm -ta @PACKAGE@-@VERSION@.tar.gz

win:
	ssh ivy "(cd /cygdrive/z/yamcha; ./make-win.bat)"

win-dist:
	rm -f @PACKAGE@-@VERSION@.zip  @PACKAGE@-@VERSION@
	mkdir -p @PACKAGE@-@VERSION@/bin
	mkdir -p @PACKAGE@-@VERSION@/doc 
	mkdir -p @PACKAGE@-@VERSION@/lib
	mkdir -p @PACKAGE@-@VERSION@/include
	mkdir -p @PACKAGE@-@VERSION@/example
	mkdir -p @PACKAGE@-@VERSION@/libexec/yamcha
	cp -f example/example.cpp @PACKAGE@-@VERSION@/example
	cp -f libexec/mkdarts.exe @PACKAGE@-@VERSION@/libexec/yamcha
	cp -f libexec/mksvmdata   @PACKAGE@-@VERSION@/libexec/yamcha
	cp -f libexec/svm_learn_wrapper @PACKAGE@-@VERSION@/libexec/yamcha
	cp -f libexec/mkparam   @PACKAGE@-@VERSION@/libexec/yamcha
	cp -f libexec/showse    @PACKAGE@-@VERSION@/libexec/yamcha
	cp -f libexec/zipmodel  @PACKAGE@-@VERSION@/libexec/yamcha
	cp -f libexec/selecttag @PACKAGE@-@VERSION@/libexec/yamch
	cp -f libexec/Makefile  @PACKAGE@-@VERSION@/libexec/yamcha
	cp -f src/libyamcha_msvc.lib @PACKAGE@-@VERSION@/lib
	cp -f src/yamcha.h @PACKAGE@-@VERSION@/include
	cp -f src/*.exe @PACKAGE@-@VERSION@/bin
	cp -f doc/*.html doc/*.css @PACKAGE@-@VERSION@/doc
	cp -f README COPYING AUTHORS @PACKAGE@-@VERSION@
	zip -r @PACKAGE@-@VERSION@.zip @PACKAGE@-@VERSION@
	rm -fr @PACKAGE@-@VERSION@

dist-all-package:
	(test -f Makefile) && $(MAKE) distclean
	./configure
	$(MAKE) dist
	$(MAKE) rpm
	$(MAKE) clean
	$(MAKE) win
	$(MAKE) win-dist
