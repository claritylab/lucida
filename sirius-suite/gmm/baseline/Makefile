CXX = g++
CXX_FLAGS += -O3
LINK_FLAGS = -lrt -lm

# File names
EXEC = gmm_scoring
SOURCES = gmm_scoring.cpp ../../timer/timer.cpp
OBJECTS = $(SOURCES:.cpp=.o)

# No need to change after this line
$(EXEC): $(OBJECTS) Makefile
	$(CXX) $(OBJECTS) -o $(EXEC) $(LINK_FLAGS)

# To obtain object files
%.o: %.cpp Makefile
	$(CXX) -c $(CXX_FLAGS) $(EXTRA_FLAGS) $< -o $@

# To remove generated files
clean:
	rm -f $(EXEC) *.o
