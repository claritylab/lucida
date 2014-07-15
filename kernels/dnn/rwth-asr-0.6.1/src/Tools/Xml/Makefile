#!gmake

TOPDIR		= ../../..

include $(TOPDIR)/Makefile.cfg

# -----------------------------------------------------------------------------

SUBDIRS		=
TARGETS		= xml2line$(exe) line2xml$(exe)

XML2LINE_O	= $(OBJDIR)/xml2line.o

LINE2XML_O	= $(OBJDIR)/line2xml.o

# -----------------------------------------------------------------------------

build: $(TARGETS)

install:
	$(INSTALL) $(TARGETS) $(INSTALL_TARGET)

xml2line$(exe): $(XML2LINE_O)
	$(LD) $^ -o $@ $(LDFLAGS)

line2xml$(exe): $(LINE2XML_O)
	$(LD) $^ -o $@ $(LDFLAGS)

include $(TOPDIR)/Rules.make

sinclude $(patsubst %.o,%.d,$(filter %.o,$(XML2LINE_O)))
sinclude $(patsubst %.o,%.d,$(filter %.o,$(LINE2XML_O)))
