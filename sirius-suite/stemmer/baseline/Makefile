CXX = g++
LINK_FLAGS =
CXX_FLAGS  = -O3 -fpermissive -Wno-write-strings

INPUT = ../input/voc.txt

# File names
EXEC = stem_porter
SOURCES = main.cpp porter.cpp ../../utils/timer.cpp
OBJECTS = $(SOURCES:.cpp=.o)

.PHONY: all test

all : $(EXEC)
$(EXEC): $(OBJECTS) Makefile
	$(CXX) $(CXX_FLAGS) $(OBJECTS) -o $(EXEC) $(LINK_FLAGS)

# To obtain object files
%.o: %.cpp Makefile
	$(CXX) -c $(CXX_FLAGS) $(EXTRA_FLAGS) $< -o $@

# test
test: CXX_FLAGS += -DTESTING
test: all
	@./$(EXEC) $(INPUT)

# To remove generated files
clean:
	$(RM) -f $(EXEC) $(OBJECTS) *.o ../input/$(EXEC).baseline
