#!gmake

TOPDIR		= ../..

include $(TOPDIR)/Makefile.cfg
ifneq ($(COMPILER),sun)
CXXFLAGS	+= -fexceptions
endif

# -----------------------------------------------------------------------------

SUBDIRS 	=
TARGETS		= libSprintCore.$(a) check$(exe) check-xml$(exe)

LIBSPRINTCORE_O	= $(OBJDIR)/Application.o \
		  $(OBJDIR)/Archive.o \
		  $(OBJDIR)/ArithmeticExpressionParser.o \
		  $(OBJDIR)/Assertions.o \
		  $(OBJDIR)/BinaryStream.o \
		  $(OBJDIR)/BinaryTree.o \
		  $(OBJDIR)/BundleArchive.o \
		  $(OBJDIR)/Channel.o \
		  $(OBJDIR)/Choice.o \
		  $(OBJDIR)/CodeGenerator.o \
		  $(OBJDIR)/Component.o \
		  $(OBJDIR)/CompressedStream.o \
		  $(OBJDIR)/Configuration.o \
		  $(OBJDIR)/Debug.o \
		  $(OBJDIR)/Dependency.o \
		  $(OBJDIR)/Description.o \
		  $(OBJDIR)/Directory.o \
		  $(OBJDIR)/DirectoryArchive.o \
		  $(OBJDIR)/FileArchive.o \
		  $(OBJDIR)/FormatSet.o \
		  $(OBJDIR)/IoUtilities.o \
		  $(OBJDIR)/MappedArchive.o \
		  $(OBJDIR)/MemoryInfo.o \
		  $(OBJDIR)/MD5.o \
		  $(OBJDIR)/Parameter.o \
		  $(OBJDIR)/ProgressIndicator.o \
		  $(OBJDIR)/ReferenceCounting.o \
		  $(OBJDIR)/ResourceUsageInfo.o \
		  $(OBJDIR)/Statistics.o \
		  $(OBJDIR)/StringExpression.o \
		  $(OBJDIR)/StringUtilities.o \
		  $(OBJDIR)/TextStream.o \
		  $(OBJDIR)/Tokenizer.o \
		  $(OBJDIR)/Types.o \
		  $(OBJDIR)/Unicode.o \
		  $(OBJDIR)/Utility.o \
		  $(OBJDIR)/Version.o \
		  $(OBJDIR)/XmlBuilder.o \
		  $(OBJDIR)/XmlBuilder2.o \
		  $(OBJDIR)/XmlParser.o \
		  $(OBJDIR)/XmlStream.o \


CHECK_O			= $(OBJDIR)/check.o \
			  libSprintCore.$(a)

CHECK_XML_O		= $(OBJDIR)/check-xml.o \
			  libSprintCore.$(a)

# non-deleted intermediate files
.SECONDARY : ArithmeticExpressionParser.cc ArithmeticExpressionParser.hh ArithmeticExpressionParser.yy

# -----------------------------------------------------------------------------

all: $(TARGETS)

libSprintCore.$(a): $(LIBSPRINTCORE_O)
	$(MAKELIB) $@ $^

check$(exe): $(CHECK_O)
	$(LD) $^ -o $@ $(LDFLAGS)

check-xml$(exe): $(CHECK_XML_O)
	$(LD) $^ -o $@ $(LDFLAGS)

# patch bison files for incompatible changes between Bison 2.3 and 2.4
# in bison >= 2.4 we can use '%code requires' to include declarations
# in the header file. In prior versions, the first prologue was always
# included in the header file.
ArithmeticExpressionParser.yy: ArithmeticExpressionParser.in
	@echo generating $@
	@if test $(BISON_MAJOR) -lt 2 -o $(BISON_MINOR) -lt 4; then \
	  sed -e '5s/%code requires {/%{/' -e '70s/^}$$/%}/' $< > $@ ; \
	else \
	  cp $< $@ ; \
	fi ;

# make sure ArithmeticExpressionParser.hh is generated before Configuration.cc is compiled.
# (in case the dependency files haven't been generated yet)
$(OBJDIR)/Configuration.o : ArithmeticExpressionParser.hh


include $(TOPDIR)/Rules.make

sinclude $(LIBSPRINTCORE_O:.o=.d)
sinclude $(patsubst %.o,%.d,$(filter %.o,$(CHECK_O)))
sinclude $(patsubst %.o,%.d,$(filter %.o,$(CHECK_XML_O)))


