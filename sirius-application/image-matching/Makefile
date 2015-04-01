CXX = g++
CXX_FLAGS =
LINK_FLAGS = -lopencv_core \
			 -lopencv_highgui \
			 -lopencv_imgproc \
			 -lopencv_nonfree \
			 -lopencv_flann \
			 -lopencv_objdetect \
			 -lopencv_features2d \
			 -lopencv_gpu \
			 -lrt \
			 -lboost_program_options \
			 -lboost_filesystem \
			 -lboost_system \
			 -lprotobuf \
			 -ltesseract

# File names
EXEC = match
SOURCES = detect.cpp kp_protobuf.cpp $(wildcard *.cc)
OBJECTS = $(SOURCES:.cpp=.o)

# Main target
$(EXEC): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(EXEC) $(LINK_FLAGS)

# To obtain object files
%.o: %.cpp Makefile
	$(CXX) -c $(CXX_FLAGS) $(EXTRA_FLAGS) $< -o $@

# To remove generated files
clean:
	rm -f $(EXEC) *.o
