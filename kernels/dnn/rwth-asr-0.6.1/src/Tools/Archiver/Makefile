#!gmake

TOPDIR		= ../../..

include $(TOPDIR)/Makefile.cfg

# -----------------------------------------------------------------------------

SUBDIRS 	=
TARGETS		= archiver$(exe)

ARCHIVER_O	= $(OBJDIR)/Archiver.o \
                 ../../Speech/libSprintSpeech.$(a) \
                 ../../Am/libSprintAm.$(a) \
                 ../../Bliss/libSprintBliss.$(a) \
                 ../../Math/libSprintMath.$(a) \
                 ../../Math/Lapack/libSprintMathLapack.$(a) \
                 ../../Core/libSprintCore.$(a) \
                 ../../Fsa/libSprintFsa.$(a)

# -----------------------------------------------------------------------------

build: $(TARGETS)

install:
	$(INSTALL) $(TARGETS) $(INSTALL_TARGET)

archiver$(exe): $(ARCHIVER_O)
	$(LD) $(ARCHIVER_O) -o $@ $(LDFLAGS)

include $(TOPDIR)/Rules.make

sinclude $(patsubst %.o,%.d,$(filter %.o,$(ARCHIVER_O)))
