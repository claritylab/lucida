CXX = g++
LINK_FLAGS = -lpthread
CXX_FLAGS = -O3 -fpermissive

THREADS   = 4
LIST      = ../input/list
QUESTIONS = ../input/questions

# File names
EXEC = regex_slre
SOURCES = main.cpp slre.cpp ../../utils/timer.cpp
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
	@./$(EXEC) $(THREADS) $(LIST) $(QUESTIONS)
	-@diff -q ../input/$(EXEC).baseline ../input/$(EXEC).pthread

# To remove generated files
clean:
	$(RM) -f $(EXEC) $(OBJECTS) *.o ../input/$(EXEC).pthread
