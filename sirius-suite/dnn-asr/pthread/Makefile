CXX=g++
CXX_FLAGS = -O3 -DCPU_ONLY
LINK_FLAGS = -lcaffe -lglog

THREADS = 4
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
test: all
	@OPENBLAS_NUM_THREADS=$(THREADS) ./$(EXEC) $(MODEL) $(WEIGHTS) $(INPUT)

# To remove generated files
clean:
	rm -f $(EXEC) $(OBJECTS) *.o
