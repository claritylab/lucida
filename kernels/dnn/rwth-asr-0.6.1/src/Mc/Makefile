#!gmake

TOPDIR		= ../..

include $(TOPDIR)/Makefile.cfg

# -----------------------------------------------------------------------------

SUBDIRS		=
TARGETS		= libSprintMc.$(a) #check$(exe)

LIBSPRINTMC_O	= $(OBJDIR)/Component.o



CHECK_O		= $(OBJDIR)/check.o \
		  libSprintMc.$(a)

# -----------------------------------------------------------------------------

all: $(TARGETS)

libSprintMc.$(a): $(LIBSPRINTMC_O)
		$(MAKELIB) $@ $^

check$(exe): $(CHECK_O)
		$(LD) $^ -o $@ $(LDFLAGS)

include $(TOPDIR)/Rules.make

sinclude $(LIBSPRINTMC_O:.o=.d)
sinclude $(patsubst %.o,%.d,$(filter %.o,$(CHECK_O)))
