CXX = nvcc
CXX_FLAGS = -arch=sm_35

# File names
EXEC = gmm_scoring
SOURCES = gmm_scoring.cu
OBJECTS = $(SOURCES:.cpp=.o)

# No need to change after this line
$(EXEC): $(OBJECTS) Makefile
	$(CXX) $(CXX_FLAGS) $(OBJECTS) -o $(EXEC) $(LINK_FLAGS)

# To obtain object files
%.o: %.cpp Makefile
	$(CXX) -c $(CXX_FLAGS) $(EXTRA_FLAGS) $< -o $@

# To remove generated files
clean:
	rm -f $(EXEC) *.o
