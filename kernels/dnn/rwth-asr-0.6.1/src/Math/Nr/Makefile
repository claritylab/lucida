#!gmake

TOPDIR		= ../../..

include $(TOPDIR)/Makefile.cfg

# -----------------------------------------------------------------------------

SUBDIRS 	=
TARGETS		= libSprintMathNr.$(a) check$(exe)

LIBNR_O		= \
		$(OBJDIR)/BesselFunctions.o \
		$(OBJDIR)/ConjugateGradient.o \
		$(OBJDIR)/FastFourierTransform.o \
		$(OBJDIR)/integration.o \
		$(OBJDIR)/ludcmp.o \
		$(OBJDIR)/pythag.o \
		$(OBJDIR)/PowerSpectrum.o \
		$(OBJDIR)/Random.o \
		$(OBJDIR)/svdcmp.o

CHECK_O		= $(OBJDIR)/check.o \
		libSprintMathNr.$(a) \
		  ../../Core/libSprintCore.$(a)

# -----------------------------------------------------------------------------

all: $(TARGETS)

build: libSprintMathNr.$(a)

libSprintMathNr.$(a): $(LIBNR_O)
	$(MAKELIB) $@ $^

check$(exe): $(CHECK_O)
	$(LD) $^ -o check$(exe) $(LDFLAGS)

include $(TOPDIR)/Rules.make

sinclude $(LIBNR_O:.o=.d)
sinclude $(patsubst %.o,%.d,$(filter %.o,$(CHECK_O)))



