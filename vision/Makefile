CXX = g++
CXX_FLAGS = `pkg-config opencv --cflags` 
LINK_FLAGS = `pkg-config opencv --libs` -ltesseract
CXX_FLAGS +=
# LINK_FLAGS +=
# EXTRA_FLAGS = -Wno-missing-field-initializers -Wno-missing-braces -Wno-format

# Directories
SRCDIR = src/
BUILDDIR = bin/

# File names
EXEC = detect
SOURCES = $(wildcard *.cpp) $(wildcard *.cc)
OBJECTS = $(SOURCES:.cpp=.o)

# Main target
$(EXEC): $(OBJECTS) Makefile
	$(CXX) $(OBJECTS) -o $(EXEC) $(LINK_FLAGS)

# To obtain object files
%.o: %.cpp Makefile
	$(CXX) -c $(CXX_FLAGS) $(EXTRA_FLAGS) $< -o $@

# To remove generated files
clean:
	rm -f $(EXEC) *.o
