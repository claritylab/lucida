# Definition of used modules and tools
#
# The MODULE_xxx terms are available as variables in all makefiles
# and as preprocessor directives in src/Modules.hh
#
# Those parts of Sprint which are not required for a basic ASR system
# should be separated by a MODULE_ definition wherever applicable.
#
# If you implement a new fancy feature in Sprint:
#   * goal: everything can still be compiled (and run) without your source files
#     by simply deactivating the module in Modules.make
#   * try to implement your classes as loosely coupled to other classes as possible
#   * define a module name in Modules.make
#   * make the makefiles depend on that name by including 'ifdef MODULE_xxx' ... 'endif'
#     (remember the makefiles in Tools/*)
#   * frame the include files of your module by '#ifdef MODULE_xxx' ... '#endif'
#     do not forget to include Modules.hh
#   * Use '#MODF MyFile.hh' to mark files of a module if the file is not listed in the
#     Makefile (e.g. header files without corresponding .cc file). See Signal/Makefile
#     for an example
MODULES += MODULE_ADAPT_CMLLR
MODULES += MODULE_ADAPT_MLLR
MODULES += MODULE_AUDIO_RAW
# use included libsndfile (GPL)
# use system libsndfile (LGPL on current systems)
MODULES += MODULE_AUDIO_WAV_SYSTEM
MODULES += MODULE_CART
MODULES += MODULE_FLF_CORE
MODULES += MODULE_FLF
MODULES   += MODULE_LATTICE_BASIC
MODULES   += MODULE_LATTICE_HTK
MODULES   += MODULE_LATTICE_DT
MODULES += MODULE_LM_ARPA
MODULES += MODULE_LM_FSA
MODULES += MODULE_LM_ZEROGRAM
MODULES += MODULE_MATH_NR
MODULES += MODULE_MM_BATCH
MODULES += MODULE_MM_DT
MODULES += MODULE_NN
MODULES += MODULE_SIGNAL_GAMMATONE
MODULES += MODULE_SIGNAL_PLP
MODULES += MODULE_SIGNAL_VTLN
MODULES += MODULE_SIGNAL_VOICEDNESS
MODULES += MODULE_SPEECH_DT
MODULES += MODULE_SPEECH_ALIGNMENT_FLOW_NODES
MODULES += MODULE_SPEECH_LATTICE_FLOW_NODES
MODULES += MODULE_SPEECH_LATTICE_ALIGNMENT
MODULES += MODULE_TEST
MODULES += MODULE_OPENMP
MODULES += MODULE_CUDA
MODULES += MODULE_INTEL_MKL
# define variables for the makefiles
$(foreach module, $(MODULES), $(eval $(module) = 1))
TOOLS += AcousticModelTrainer
TOOLS += Archiver
TOOLS += CorpusStatistics
TOOLS += FeatureExtraction
TOOLS += FeatureStatistics
TOOLS += SpeechRecognizer
TOOLS += Xml
ifdef MODULE_CART
TOOLS += Cart
endif
ifdef MODULE_MM_DT
ifdef MODULE_LATTICE_DT
ifdef MODULE_SPEECH_DT
TOOLS += LatticeProcessor
endif
endif
endif
ifdef MODULE_FLF
TOOLS += Flf
endif
ifdef MODULE_NN
TOOLS += NnTrainer
endif
LIBS_SEARCH = src/Search/libSprintSearch.$(a)
