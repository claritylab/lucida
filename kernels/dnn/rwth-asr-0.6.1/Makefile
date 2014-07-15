# Project:       SPRINT
# File:          Makefile - top-level makefile

# !!! Look into Options.make for configurable settings !!!

# -----------------------------------------------------------------------------

default:	build

TOPDIR		= .
SUBDIRS		= src

BASEDIR		= $(shell basename $(PWD))

include	Config.make

# -----------------------------------------------------------------------------
# targets

world:
	./scripts/build all

all:	build test

build:	config
	$(MAKE) -C src build

test:	build
	$(MAKE) -s -C test test

tar:	subdirs_recursive_distclean
	cd .. ; \
	tar cvf sprint.tar $(BASEDIR) ; \
	gzip -f sprint.tar

config: report-make-environment

install:
	install -d $(INSTALL_TARGET)
	$(MAKE) -C src $@

statistics:
	@$(TOPDIR)/scripts/statistics.sh

clean:	clean_
clean_: subdirs_clean

.PHONY:	world all build test tar config install statistics

report-make-environment:
	$(ECHO)
	$(ECHO) "********************************************************************************"
	$(ECHO) "Project:\t\tSPRINT"
	$(ECHO) "********************************************************************************"
	$(ECHO)
	$(ECHO) "Build host:\t\t" `hostname`
	$(ECHO) "Build date:\t\t" `date`
	$(ECHO) "Build directory:\t" `pwd`
	$(ECHO)
	$(ECHO) "Processor:\t\t$(PROC)"
	$(ECHO) "Operating system:\t$(OS)"
	$(ECHO) "Version:\t\t$(VERSION)  - $(COMPILE)"
	$(ECHO) "Profiling:\t\t$(PROFILE)"
	$(ECHO)
	$(ECHO) "Compiler:\t\t$(CC)  /  $(CXX)"
	$(ECHO) "Preprocessor flags:\t$(CPPFLAGS)"
	$(ECHO) "C compiler flags:\t$(CFLAGS)"
	$(ECHO) "C++ compiler flags:\t$(CXXFLAGS)"
	$(ECHO)
	$(ECHO) "Linker:\t\t\t$(LD)"
	$(ECHO) "Linker flags:\t\t$(LDFLAGS)"
	$(ECHO)
	$(ECHO) "Binary suffix:\t\t$(OBJEXT)"
	$(ECHO) "Build dir:\t\t$(OBJBUILDDIR)"
	$(ECHO)
	$(ECHO) "Installation directories:"
	$(ECHO) -n "  Base:\t\t\t"
	$(ECHO) $(INSTALL_BASE)
	$(ECHO) -n "  Binaries:\t\t"
	$(ECHO) $(INSTALL_BINDIR)
	$(ECHO) -n "  Includes:\t\t"
	$(ECHO) $(INSTALL_INCDIR)
	$(ECHO) -n "  Libraries:\t\t"
	$(ECHO) $(INSTALL_LIBDIR)
	$(ECHO) "********************************************************************************"
	$(ECHO)

include Rules.make
