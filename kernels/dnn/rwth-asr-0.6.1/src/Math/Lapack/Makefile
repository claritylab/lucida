#!gmake

TOPDIR		= ../../..

include $(TOPDIR)/Makefile.cfg

# -----------------------------------------------------------------------------

SUBDIRS 	=
TARGETS		= libSprintMathLapack.$(a) check$(exe)

LIBLAPACK_O		= $(OBJDIR)/Lapack.o \
			  $(OBJDIR)/EigenvalueProblem.o\
			  $(OBJDIR)/Svd.o


CHECK_O		= $(OBJDIR)/check.o \
		  libSprintMathLapack.$(a) \
		  ../../Core/libSprintCore.$(a)


# -----------------------------------------------------------------------------

all: $(TARGETS)

build: libSprintMathLapack.$(a)

libSprintMathLapack.$(a): $(LIBLAPACK_O)
	$(MAKELIB) $@ $^

check$(exe): $(CHECK_O)
	$(LD) $^ -o check$(exe) $(LDFLAGS)

include $(TOPDIR)/Rules.make

sinclude $(LIBLAPACK_O:.o=.d)
sinclude $(patsubst %.o,%.d,$(filter %.o,$(CHECK_O)))



