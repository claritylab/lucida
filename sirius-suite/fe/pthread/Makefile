CXX = g++
CXX_FLAGS = -O3
LINK_FLAGS = -lopencv_core \
			 -lopencv_highgui \
			 -lopencv_imgproc \
			 -lopencv_nonfree \
			 -lopencv_features2d \
			 -lpthread

INPUT = ../input/2048x2048.jpg
THREADS = 4

# File names
EXEC = surf-fe
SOURCES = surf-fe.cpp ../../utils/timer.cpp
OBJECTS = $(SOURCES:.cpp=.o)

.PHONY: all

all : $(EXEC)
$(EXEC): $(OBJECTS) Makefile
	$(CXX) $(OBJECTS) -o $(EXEC) $(LINK_FLAGS)

# To obtain object files
%.o: %.cpp Makefile
	$(CXX) -c $(CXX_FLAGS) $(EXTRA_FLAGS) $< -o $@

# test
test: all
	@./$(EXEC) $(THREADS) $(INPUT)

# To remove generated files
clean:
	rm -f $(EXEC) *.o
