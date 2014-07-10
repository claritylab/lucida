CXX = gcc -std=c99 -g
CXX_FLAGS =
LINK_FLAGS =
CXX_FLAGS +=
# LINK_FLAGS +=
# EXTRA_FLAGS = -Wno-missing-field-initializers -Wno-missing-braces -Wno-format

# File names
EXEC = regex
SOURCES = $(wildcard *.c)
OBJECTS = $(SOURCES:.cpp=.o)

# Main target
$(EXEC): $(OBJECTS) Makefile
	$(CXX) $(OBJECTS) -o $(EXEC) $(LINK_FLAGS)

# To obtain object files
%.o: %.c Makefile
	$(CXX) -c $(CXX_FLAGS) $(EXTRA_FLAGS) $< -o $@

# To remove generated files
clean:
	rm -f $(EXEC) *.o
