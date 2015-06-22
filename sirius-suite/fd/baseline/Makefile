CXX = g++
CXX_FLAGS  = -O2 -ftree-vectorize -mavx -mavx2 -mfma -ffast-math
CXX_FLAGS += -I/home/jahausw/opencv-multi/sse/install/include
# LINK_FLAGS = -lopencv_core \
# 			 -lopencv_highgui \
# 			 -lopencv_imgproc \
# 			 -lopencv_nonfree \
# 			 -lopencv_features2d
LINK_FLAGS = /home/jahausw/opencv-multi/sse/install/lib/libopencv_core.so \
			 /home/jahausw/opencv-multi/sse/install/lib/libopencv_highgui.so \
			 /home/jahausw/opencv-multi/sse/install/lib/libopencv_imgproc.so \
			 /home/jahausw/opencv-multi/sse/install/lib/libopencv_imgcodecs.so \
			 /home/jahausw/opencv-multi/sse/install/lib/libopencv_xfeatures2d.so
LINK_FLAGS += -lpthread -ldl

INPUT = ../input/2048x2048.jpg

# File names
EXEC = surf-fd
SOURCES = surf-fd.cpp ../../utils/timer.cpp
OBJECTS = $(SOURCES:.cpp=.o)

.PHONY: all test

all : $(EXEC)
$(EXEC): $(OBJECTS) Makefile
	$(CXX) $(OBJECTS) -o $(EXEC) $(LINK_FLAGS)

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
