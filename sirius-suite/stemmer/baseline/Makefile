CXX = g++
LINK_FLAGS = -lrt
CXX_FLAGS  = -O3 -fpermissive -Wno-write-strings

INPUT = ../input/voc.txt

# File names
EXEC = stem_porter
SOURCES = main.cpp porter.cpp ../../timer/timer.cpp
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
	@./$(EXEC) $(INPUT)

# To remove generated files
clean:
	rm -f $(EXEC) *.o
