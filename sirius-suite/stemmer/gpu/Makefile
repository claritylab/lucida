CXX = nvcc
CXX_FLAGS = -arch=sm_35

INPUT = ../input/voc.txt

# File names
EXEC = stem_porter
SOURCES = porter.cu
OBJECTS = $(SOURCES:.cpp=.o)

.PHONY: all

all : $(EXEC)
$(EXEC): $(OBJECTS) Makefile
	$(CXX) $(CXX_FLAGS) $(OBJECTS) -o $(EXEC) $(LINK_FLAGS)

# To obtain object files
%.o: %.cpp Makefile
	$(CXX) -c $(CXX_FLAGS) $(EXTRA_FLAGS) $< -o $@

# test
test: all
	@./$(EXEC) $(INPUT) > /dev/null

# To remove generated files
clean:
	rm -f $(EXEC) *.o
