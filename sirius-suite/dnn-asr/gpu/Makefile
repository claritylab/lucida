CUDA=/usr/local/cuda
CXX=g++
CXX_FLAGS = -O3 -I$(CUDA)/include
LINK_FLAGS = -L$(CUDA)/lib64 \
			 -lcudart \
			 -lcaffe \
			 -lglog

MODEL   = ../model/asr.prototxt
WEIGHTS = ../model/asr.caffemodel
INPUT   = ../input/features.in

EXEC = dnn-asr
SOURCES=dnn-asr.cpp ../../timer/timer.cpp
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
	@./$(EXEC) $(MODEL) $(WEIGHTS) $(INPUT)

# To remove generated files
clean:
	rm -f $(EXEC) *.o
