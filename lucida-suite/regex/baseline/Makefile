CXX = g++
CXX_FLAGS  = -O2
LINK_FLAGS += -lpthread -ldl

NUM_PATTERNS  = 100
PATTERNS      = ../input/patterns.txt
NUM_QUESTIONS = 100
QUESTIONS     = ../input/questions.txt

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
	@./$(EXEC) $(NUM_PATTERNS) $(PATTERNS) $(NUM_QUESTIONS) $(QUESTIONS)

# To remove generated files
clean:
	$(RM) -f $(EXEC) $(OBJECTS) *.o ../input/$(EXEC).baseline
