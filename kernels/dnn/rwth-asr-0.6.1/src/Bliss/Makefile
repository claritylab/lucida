#!gmake

TOPDIR		= ../..

include $(TOPDIR)/Makefile.cfg

# -----------------------------------------------------------------------------

SUBDIRS 	=
TARGETS		= libSprintBliss.$(a) check$(exe)

LIBBLISS_O    = \
		$(OBJDIR)/CorpusDescription.o		\
		$(OBJDIR)/CorpusKey.o 				\
		$(OBJDIR)/CorpusParser.o  		\
		$(OBJDIR)/CorpusStatistics.o		\
		$(OBJDIR)/EditDistance.o		\
		$(OBJDIR)/Evaluation.o			\
		$(OBJDIR)/Fsa.o				\
		$(OBJDIR)/Lexicon.o			\
		$(OBJDIR)/LexiconParser.o		\
		$(OBJDIR)/Orthography.o			\
		$(OBJDIR)/Phoneme.o			\
		$(OBJDIR)/Phonology.o			\
		$(OBJDIR)/PrefixTree.o			\
		$(OBJDIR)/SegmentOrdering.o			\
		$(OBJDIR)/Symbol.o			\
		$(OBJDIR)/Unknown.o			\


CHECK_O		= $(OBJDIR)/check.o 			\
		  libSprintBliss.$(a)			\
		  ../Fsa/libSprintFsa.$(a)		\
		  ../Core/libSprintCore.$(a)

# -----------------------------------------------------------------------------

all: $(TARGETS)

libSprintBliss.$(a): $(LIBBLISS_O)
	$(MAKELIB) $@ $^

check$(exe): $(CHECK_O)
	$(LD) $^ -o check$(exe) $(LDFLAGS)

include $(TOPDIR)/Rules.make

sinclude $(LIBBLISS_O:.o=.d)
sinclude $(patsubst %.o,%.d,$(filter %.o,$(CHECK_O)))
