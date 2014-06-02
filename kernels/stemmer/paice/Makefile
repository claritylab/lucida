# This is a basic Makefile. Change flags and EXEC
CXX = gcc
CXX_FLAGS =
LINK_FLAGS =
CXX_FLAGS += -O3
# LINK_FLAGS +=
# EXTRA_FLAGS = -Wno-missing-field-initializers -Wno-missing-braces -Wno-format

# Directories
SRCDIR = src/
BUILDDIR = bin/

# File names
EXEC = paice
SOURCES = $(wildcard *.c)
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
