CXX = g++
LINK_FLAGS = -lrt
CXX_FLAGS += -O3 -fpermissive -Wno-write-strings

# File names
EXEC = porter
SOURCES = porter.cpp
OBJECTS = $(SOURCES:.cpp=.o)

# No need to change after this line
# Main target
$(EXEC): $(OBJECTS) Makefile
	$(CXX) $(OBJECTS) -o $(EXEC) $(LINK_FLAGS)

# To obtain object files
%.o: %.cpp Makefile
	$(CXX) -c $(CXX_FLAGS) $(EXTRA_FLAGS) $< -o $@

# To remove generated files
clean:
	rm -f $(EXEC) *.o
