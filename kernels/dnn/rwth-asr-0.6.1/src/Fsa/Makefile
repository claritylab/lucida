#!gmake

TOPDIR		= ../..

include $(TOPDIR)/Makefile.cfg

# -----------------------------------------------------------------------------

SUBDIRS 	=
TARGETS		= libSprintFsa.$(a)

# check$(exe)

LIBSPRINTFSA_O	= $(OBJDIR)/Accessible.o \
		  $(OBJDIR)/Alphabet.o \
		  $(OBJDIR)/AlphabetUtility.o \
		  $(OBJDIR)/AlphabetXml.o \
		  $(OBJDIR)/Archive.o \
		  $(OBJDIR)/Arithmetic.o \
		  $(OBJDIR)/Basic.o \
		  $(OBJDIR)/Best.o \
		  $(OBJDIR)/Cache.o \
		  $(OBJDIR)/Compose.o \
		  $(OBJDIR)/Determinize.o \
		  $(OBJDIR)/Input.o \
		  $(OBJDIR)/Levenshtein.o \
		  $(OBJDIR)/Linear.o \
		  $(OBJDIR)/Minimize.o \
		  $(OBJDIR)/Output.o \
		  $(OBJDIR)/Packed.o \
		  $(OBJDIR)/Permute.o \
		  $(OBJDIR)/Properties.o \
		  $(OBJDIR)/Project.o \
		  $(OBJDIR)/Prune.o \
		  $(OBJDIR)/Random.o \
		  $(OBJDIR)/Resources.o \
		  $(OBJDIR)/Rational.o \
		  $(OBJDIR)/RemoveEpsilons.o \
		  $(OBJDIR)/Semiring.o \
		  $(OBJDIR)/Semiring64.o \
		  $(OBJDIR)/Sort.o \
		  $(OBJDIR)/Sssp.o \
		  $(OBJDIR)/Sssp4SpecialSymbols.o \
		  $(OBJDIR)/Static.o \
		  $(OBJDIR)/Storage.o \
		  $(OBJDIR)/Types.o \
		  $(OBJDIR)/Utility.o



PYTHON_O	= $(OBJDIR)/Python.o \
		  ../Fsa/libSprintFsa.$(a) \
		  ../Core/libSprintCore.$(a)

CHECK_O		= $(OBJDIR)/check.o \
		  ../Fsa/libSprintFsa.$(a) \
		  ../Core/libSprintCore.$(a)

# -----------------------------------------------------------------------------

all: $(TARGETS)

libSprintFsa.$(a): $(LIBSPRINTFSA_O)
	$(MAKELIB) $@ $^

check$(exe): $(CHECK_O)
	$(LD) $^ -o $@ $(LDFLAGS)

fsa_wrap.cc: fsa.i Python.hh
	swig -c++ -python -o fsa_wrap.cc fsa.i

fsa_wrap.o: fsa_wrap.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -fpic -c fsa_wrap.cc -I/usr/include/python2.3

_fsa.so: fsa_wrap.o $(PYTHON_O)
	$(CXX) $(LDFLAGS) -shared fsa_wrap.o $(PYTHON_O) -o _fsa.so

include $(TOPDIR)/Rules.make

sinclude $(LIBSPRINTFSA_O:.o=.d)
