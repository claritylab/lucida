CXX = g++
LINK_FLAGS = -lpthread -lrt -lm
CXX_FLAGS = -g

# File names
EXEC = regex
SOURCES = main.cpp slre.cpp
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
