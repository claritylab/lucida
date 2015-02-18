CXX = g++
CXX_FLAGS = -O3
LINK_FLAGS = -lpthread \
			 -lopencv_core \
			 -lopencv_highgui \
			 -lopencv_imgproc \
			 -lopencv_nonfree \
			 -lopencv_features2d

INPUT = ../input/2048x2048.jpg
THREADS = 4

# File names
EXEC = surf-fd
SOURCES = surf-fd.cpp ../../utils/timer.cpp
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
	@./$(EXEC) $(THREADS) $(INPUT)

# To remove generated files
clean:
	$(RM) -f $(EXEC) *.o ../input/$(EXEC).pthread
