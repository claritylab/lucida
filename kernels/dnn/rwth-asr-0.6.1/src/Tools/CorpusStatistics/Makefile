#!make

TOPDIR		= ../../..

include $(TOPDIR)/Makefile.cfg

# -----------------------------------------------------------------------------

SUBDIRS		=
TARGETS		= costa$(exe)

CORPUS_STATISTICS_O = $(OBJDIR)/Costa.o \
		  ../../Lm/libSprintLm.$(a) \
		  ../../Mc/libSprintMc.$(a) \
		  ../../Bliss/libSprintBliss.$(a) \
		  ../../Fsa/libSprintFsa.$(a) \
		  ../../Audio/libSprintAudio.$(a) \
		  ../../Signal/libSprintSignal.$(a) \
		  ../../Speech/libSprintSpeech.$(a) \
		  ../../Flow/libSprintFlow.$(a) \
		  ../../Core/libSprintCore.$(a) \
		  ../../Lattice/libSprintLattice.$(a)


# -----------------------------------------------------------------------------

build: $(TARGETS)

install:
	$(INSTALL) $(TARGETS) $(INSTALL_TARGET)

costa$(exe): $(CORPUS_STATISTICS_O)
	$(LD) $(CORPUS_STATISTICS_O) -o $@ $(LDFLAGS)

include $(TOPDIR)/Rules.make

sinclude $(patsubst %.o,%.d,$(filter %.o,$(CORPUS_STATISTICS_O)))
