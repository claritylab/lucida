include src/kaldi.mk
include src/makefiles/default_rules.mk
include Makefile.config

TARGET  = ASRServer
SOURCES = $(wildcard *.cpp gen-cpp2/*.cpp)
OBJECTS = $(SOURCES:.cpp=.o)

ADDLIBS = src/online2/kaldi-online2.a src/ivector/kaldi-ivector.a \
					src/nnet2/kaldi-nnet2.a src/lat/kaldi-lat.a \
					src/decoder/kaldi-decoder.a src/cudamatrix/kaldi-cudamatrix.a \
					src/feat/kaldi-feat.a src/transform/kaldi-transform.a \
					src/gmm/kaldi-gmm.a src/thread/kaldi-thread.a src/hmm/kaldi-hmm.a \
					src/tree/kaldi-tree.a src/matrix/kaldi-matrix.a \
					src/fstext/kaldi-fstext.a src/util/kaldi-util.a \
					src/base/kaldi-base.a $(LDLIBS) $(CUDA_LDLIBS)

CXXFLAGS += -Isrc/

all: $(TARGET)
all: CXXFLAGS += -O3
#all: thrift $(TARGET)

debug: CXXFLAGS += -g3
debug: thrift $(TARGET)

thrift:
	@if [ ! -d "gen-cpp2" ]; then \
       python -mthrift_compiler.main --gen cpp2 ../lucidaservice.thrift; \
       python -mthrift_compiler.main --gen cpp2 ../lucidatypes.thrift; \
    fi

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(ADDLIBS) $(LDFLAGS)

%.o: %.cpp
	$(CXX) -Wall $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(TARGET) *.o

#rm -rf $(TARGET) *.o gen-cpp2

.PHONY: all debug thrift clean
