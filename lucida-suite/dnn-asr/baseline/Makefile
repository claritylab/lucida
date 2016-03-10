CXX=g++
CXX_FLAGS = -O2 -DCPU_ONLY -I$(CAFFE)/include
LINK_FLAGS = $(CAFFE)/lib/libcaffe.so \
						 -lprotobuf \
						 -lgflags -lleveldb -lsnappy \
						 -llmdb -lhdf5 -lhdf5_hl -lboost_thread -lpthread -lopencv_core -lopencv_imgproc \
						 -lglog
LINK_FLAGS += -lpthread -ldl

MODEL   = ../model/asr.prototxt
WEIGHTS = ../model/asr.caffemodel
INPUT   = ../input/features.in

EXEC = dnn_asr
SOURCES=dnn-asr.cpp ../../utils/timer.cpp
OBJECTS=$(SOURCES:.cpp=.o)

.PHONY: all test clean

all : $(EXEC)
$(EXEC): $(OBJECTS) Makefile
	$(CXX) $(OBJECTS) -o $(EXEC) $(LINK_FLAGS)

# To obtain object files
%.o: %.cpp Makefile
	$(CXX) -c $(CXX_FLAGS) $(EXTRA_FLAGS) $< -o $@

# test
test: CXX_FLAGS += -DTESTING
test: all
	@./$(EXEC) $(MODEL) $(WEIGHTS) $(PASSES) $(INPUT)

# To remove generated files
clean:
	rm -f $(EXEC) $(OBJECTS) *.o
