CXX = g++
LINK_FLAGS =
CXX_FLAGS  = -O3

LIST = ../input/list
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
	@./$(EXEC) $(LIST) $(QUESTIONS)

# To remove generated files
clean:
	$(RM) -f $(EXEC) $(OBJECTS) *.o ../input/$(EXEC).baseline
