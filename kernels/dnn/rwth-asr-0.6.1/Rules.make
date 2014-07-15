# Project:      SPRINT
# File:		Rules.make - generic parts of build environment
# Revision:     $Id: Rules.make 8305 2011-06-29 16:02:58Z rybach $

# -----------------------------------------------------------------------------
subdirs_%:
	@sh -c 'set -e; for i in $(SUBDIRS); do $(MAKE) -C $$i $*; done'

recursive_%:	%
	@sh -c 'set -e; for i in $(SUBDIRS); do $(MAKE) -C $$i $@; done'

# -----------------------------------------------------------------------------
# cleaning
clean:
	find . -name "*~" 			| xargs rm -f
	if [ "$(OBJBUILDDIR)" != "" ]; then find -L . -path "*/$(OBJDIR)/*" -exec rm \{\} \;; fi
	find . \( -type d -or -type l \) -path "*/$(OBJDIR)"	| xargs rm -fr
	find . -type f -name "core" 		| xargs rm -f
	find . -regex '.*\.\(c\|hh\|cc\).bprof' | xargs rm -f
	find . -name "[bg]mon.out" 		| xargs rm -f
	#find . -name "*.bak" 			| xargs rm -f
	find . -name ".gdb_history" 		| xargs rm -f

distclean:	clean
	find . -type d -name '.build'		| xargs rm -fr
	rm -f $(TARGETS)

clean-depend:
	find -L . -type f -name "*.d" -print | xargs rm -f

clean-libs:
	find . -type f -name "*.$(a)" -print | xargs rm -f

clean-all: distclean clean-depend clean-libs


# -----------------------------------------------------------------------------
# C/C++ compilation
define createObjDir
        if [ "$(OBJBUILDDIR)" = "" ]; then \
                mkdir -p $(OBJDIR); \
        else \
                mkdir -p $(OBJBUILDDIR)/$(shell basename $(shell pwd))/$(OBJDIR); \
		if [ ! -L $(OBJDIR) ]; then \
			if [ -d $(OBJDIR) ]; then \
				echo "WARNING: $(OBJDIR) already exists. Cannot use OBJBUILDDIR $(OBJBUILDDIR)"; \
			else \
				if [ ! -d $(shell dirname $(OBJDIR)) ]; then \
					mkdir -p $(shell dirname $(OBJDIR)); \
				fi; \
				ln -s $(OBJBUILDDIR)/$(shell basename $(shell pwd))/$(OBJDIR) $(OBJDIR); \
			fi; \
                fi; \
        fi
endef


$(OBJDIR)/%.o: %.asm
	@$(call createObjDir)
	$(ASM) $(ASMFLAGS) -o $@ $<

$(OBJDIR)/%.o: %.c
	@$(call createObjDir)
	@echo compiling $<
	@if $(CC) $(CPPFLAGS) $(CFLAGS) \
	-MMD -MP -MF "$(OBJDIR)/$*.dd" \
	-c -o $@ $<; \
	then mv -f "$(OBJDIR)/$*.dd" "$(OBJDIR)/$*.d"; \
	else rm -f "$(OBJDIR)/$*.dd"; exit 1; fi

$(OBJDIR)/%.o: %.cc
	@$(call createObjDir)
	@echo compiling $<
	@if $(CXX) $(CPPFLAGS) $(CXXFLAGS) \
	-MMD -MP -MF "$(OBJDIR)/$*.dd" \
	-c -o $@ $<; \
	then mv -f "$(OBJDIR)/$*.dd" "$(OBJDIR)/$*.d"; \
	else rm -f "$(OBJDIR)/$*.dd"; exit 1; fi

$(OBJDIR)/%.pic: %.c
	@$(call createObjDir)
	@echo compiling $<
	@$(CC) $(CPPFLAGS) $(CFLAGS) -fPIC -c -o $@ $<

$(OBJDIR)/%.pic: %.cc
	@$(call createObjDir)
	@echo compiling $<
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) -fPIC -c -o $@ $<

$(OBJDIR)/%.i: %.c
	@$(call createObjDir)
	@echo preprocessing $<
	@$(CC) $(CPPFLAGS) $(CFLAGS) -E -o $@ $<

$(OBJDIR)/%.ii: %.cc
	@$(call createObjDir)
	@echo preprocessing $<
	@$(CC) $(CPPFLAGS) $(CXXFLAGS) -E -o $@ $<

$(OBJDIR)/%.ss: %.cc
	@$(call createObjDir)
	@echo preprocessing $<
	@$(CC) $(CPPFLAGS) $(CXXFLAGS) -S -o $@ $<

%.cc %.hh: %.yy
	@echo processing $<
	@$(BISON) -o $(patsubst %.hh,%.cc,$@) $<

moc_%.cc: %.hh
	$(MOC) $< -o $@

# -----------------------------------------------------------------------------
# document processing
%.dvi: %.tex
	$(LATEX) $<

# -----------------------------------------------------------------------------
INSTALL	= install -s -p
