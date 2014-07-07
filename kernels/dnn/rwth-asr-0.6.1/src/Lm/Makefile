#!gmake

TOPDIR		= ../..

include $(TOPDIR)/Makefile.cfg

# -----------------------------------------------------------------------------

SUBDIRS 	=
TARGETS		= libSprintLm.$(a) check$(exe)

LIBSPRINTLM_O	= \
		$(OBJDIR)/BackingOff.o			\
		$(OBJDIR)/ClassLm.o			\
		$(OBJDIR)/Compose.o			\
		$(OBJDIR)/CorpusStatistics.o		\
		$(OBJDIR)/IndexMap.o			\
		$(OBJDIR)/LanguageModel.o		\
		$(OBJDIR)/Module.o 				\
		$(OBJDIR)/ReverseArpaLm.o \
		$(OBJDIR)/ScaledLanguageModel.o 	\
		$(OBJDIR)/WordlistInterface.o


ifdef MODULE_LM_ARPA
LIBSPRINTLM_O += $(OBJDIR)/ArpaLm.o
endif
ifdef MODULE_LM_FSA
LIBSPRINTLM_O += $(OBJDIR)/FsaLm.o
endif
ifdef MODULE_LM_ZEROGRAM
LIBSPRINTLM_O += $(OBJDIR)/Zerogram.o
endif

CHECK_O		= $(OBJDIR)/check.o			\
		  libSprintLm.$(a)			\
		  ../Bliss/libSprintBliss.$(a)		\
		  ../Mc/libSprintMc.$(a)		\
		  ../Fsa/libSprintFsa.$(a)		\
		  ../Core/libSprintCore.$(a)

# -----------------------------------------------------------------------------

all: $(TARGETS)

RwthLm:
	$(MAKE) -C $@

.PHONY:	$(SUBDIRS)

libSprintLm.$(a): $(SUBDIRS) $(LIBSPRINTLM_O)
	$(MAKELIB) $@ $(LIBSPRINTLM_O) $(ADD_FILES)

check$(exe): $(CHECK_O)
	$(LD) $^ -o $@ $(LDFLAGS)

include $(TOPDIR)/Rules.make

sinclude $(LIBSPRINTLM_O:.o=.d)
sinclude $(patsubst %.o,%.d,$(filter %.o,$(CHECK_O)))
# DO NOT DELETE

BackingOff.o: BackingOff.hh
# BackingOff.hh includes:
#	LanguageModel.hh
BackingOff.o: LanguageModel.hh
