CXX = nvcc
CXX_FLAGS = -arch=compute_35 -code=sm_35

INPUT = ../input/gmm_data.txt

# File names
EXEC = gmm_scoring
SOURCES = gmm_scoring.cu ../../utils/correct.cpp
OBJECTS = $(SOURCES:.cpp=.o)

.PHONY: all

all : $(EXEC)
$(EXEC): $(OBJECTS) Makefile
	$(CXX) $(CXX_FLAGS) $(OBJECTS) -o $(EXEC) $(LINK_FLAGS)

# To obtain object files
%.o: %.cpp Makefile
	$(CXX) -c $(CXX_FLAGS) $< -o $@

# test
test: CXX_FLAGS += -DTESTING
test: all
	@./$(EXEC) $(INPUT)
	@diff ../input/gmm.gpu ../input/gmm.baseline > /dev/null ; \
		if [ $$? -eq 1 ] ; then echo "GMM-GPU test failed" ; fi

# To remove generated files
clean:
	rm -f $(EXEC) *.o
