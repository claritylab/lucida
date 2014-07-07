#!gmake

TOPDIR		= ../..

include $(TOPDIR)/Makefile.cfg

CXXFLAGS += -fexceptions

# -----------------------------------------------------------------------------

SUBDIRS 	=
TARGETS		= libSprintTest.$(a) unit-test$(exe)

LIBSPRINTTEST_O	= $(OBJDIR)/Registry.o \
                  $(OBJDIR)/Lexicon.o \
                  $(OBJDIR)/File.o


TEST_O = $(OBJDIR)/Bliss_SegmentOrdering.o
TEST_O += $(OBJDIR)/Core_StringUtilities.o
TEST_O += $(OBJDIR)/Core_Thread.o
TEST_O += $(OBJDIR)/Core_ThreadPool.o
TEST_O += $(OBJDIR)/Fsa_Sssp4SpecialSymbols.o
TEST_O += $(OBJDIR)/Math_Utilities.o
TEST_O += $(OBJDIR)/Math_Blas.o
TEST_O += $(OBJDIR)/Math_FastVectorOperations.o
TEST_O += $(OBJDIR)/Math_CudaVector.o
TEST_O += $(OBJDIR)/Math_CudaMatrix.o
TEST_O += $(OBJDIR)/Math_FastMatrix.o
#TEST_O += $(OBJDIR)/Math_LinearConjugateGradient.o
TEST_O += $(OBJDIR)/Test_File.o
TEST_O += $(OBJDIR)/Test_Lexicon.o



ifdef MODULE_NN
TEST_O += $(OBJDIR)/Nn_NetworkTopology.o
TEST_O += $(OBJDIR)/Nn_BufferedFeatureExtractor.o
TEST_O += $(OBJDIR)/Nn_BufferedAlignedFeatureProcessor.o
TEST_O += $(OBJDIR)/Nn_ClassLabelWrapper.o
TEST_O += $(OBJDIR)/Nn_FeedForwardCrossEntropyTrainer.o
TEST_O += $(OBJDIR)/Nn_LinearAndActivationLayer.o
TEST_O += $(OBJDIR)/Nn_LinearLayer.o
TEST_O += $(OBJDIR)/Nn_NeuralNetwork.o
TEST_O += $(OBJDIR)/Nn_NeuralNetworkLayer.o
TEST_O += $(OBJDIR)/Nn_NeuralNetworkTrainer.o
TEST_O += $(OBJDIR)/Nn_PreprocessingLayer.o
TEST_O += $(OBJDIR)/Nn_Statistics.o
endif

ifdef MODULE_OPENMP
TEST_O += $(OBJDIR)/Math_MultithreadingHelper.o
endif


UNIT_TEST_O	= $(OBJDIR)/UnitTester.o $(TEST_O) \
		  libSprintTest.$(a) \
		  ../Bliss/libSprintBliss.$(a) \
		  ../Fsa/libSprintFsa.$(a) \
		  ../Core/libSprintCore.$(a)\
		  ../Speech/libSprintSpeech.$(a) \
		  ../Search/libSprintSearch.$(a) \
		  ../Lattice/libSprintLattice.$(a) \
		  ../Am/libSprintAm.$(a) \
		  ../Mm/libSprintMm.$(a) \
		  ../Lm/libSprintLm.$(a) \
		  ../Mc/libSprintMc.$(a) \
		  ../Legacy/libSprintLegacy.$(a) \
		  ../Audio/libSprintAudio.$(a) \
		  ../Signal/libSprintSignal.$(a) \
		  ../Flow/libSprintFlow.$(a) \
		  ../Math/libSprintMath.$(a) \
		  ../Math/Lapack/libSprintMathLapack.$(a) \

ifdef MODULE_FLF
UNIT_TEST_O += ../Flf/libSprintFlf.$(a)
endif
ifdef MODULE_FLF_CORE
UNIT_TEST_O += ../Flf/FlfCore/libSprintFlfCore.$(a)
endif
ifdef MODULE_NN
UNIT_TEST_O += ../Nn/libSprintNn.$(a)
endif
ifdef MODULE_CART
UNIT_TEST_O += ../Cart/libSprintCart.$(a)
endif
ifdef MODULE_MATH_NR
UNIT_TEST_O += ../Math/Nr/libSprintMathNr.$(a)
endif

# -----------------------------------------------------------------------------

all: $(TARGETS)

libSprintTest.$(a): $(LIBSPRINTTEST_O)
	$(MAKELIB) $@ $^

unit-test$(exe): $(UNIT_TEST_O)
	$(LD) $(LD_START_GROUP) $^ $(LD_END_GROUP) -o $@ $(LDFLAGS) -lcppunit

test: unit-test$(exe)
	./unit-test$(exe)

include $(TOPDIR)/Rules.make

sinclude $(LIBSPRINTTEST_O:.o=.d)
sinclude $(patsubst %.o,%.d,$(filter %.o,$(UNIT_TEST_O)))


