CXX = g++
CXX_FLAGS = -O3
LINK_FLAGS = ../libcrfpp.a

MODEL   = ../input/model
INPUT   = ../input/test.data

# File names
EXEC = crf_tag
SOURCES = main.cpp ../../utils/timer.cpp
OBJECTS = $(SOURCES:.cpp=.o)

.PHONY: all

all : $(EXEC)
$(EXEC): $(OBJECTS) Makefile
	$(CXX) $(OBJECTS) -o $(EXEC) $(LINK_FLAGS)

# To obtain object files
%.o: %.cpp Makefile
	$(CXX) -c $(CXX_FLAGS) $(EXTRA_FLAGS) $< -o $@

# test
test: CXX_FLAGS += -DTESTING
test: all
	@./$(EXEC) $(MODEL) $(INPUT)

# To remove generated files
clean:
	rm -f $(EXEC) *.o
