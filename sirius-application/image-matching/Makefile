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

all: $(EXEC)
# Main target
$(EXEC):
	protoc --cpp_out=. keypoints.proto
	protoc --cpp_out=. store_mat.proto
	$(CXX) -c $(CXX_FLAGS) $(EXTRA_FLAGS) detect.cpp -o detect.o
	$(CXX) -c $(CXX_FLAGS) $(EXTRA_FLAGS) kp_protobuf.cpp -o kp_protobuf.o
	$(CXX) -c $(CXX_FLAGS) $(EXTRA_FLAGS) keypoints.pb.cc -o keypoints.o
	$(CXX) -c $(CXX_FLAGS) $(EXTRA_FLAGS) store_mat.pb.cc -o store_mat.o
	$(CXX) $(OBJECTS) -o $(EXEC) $(LINK_FLAGS)

%.o: %.cpp Makefile
	$(CXX) -c $(CXX_FLAGS) $(EXTRA_FLAGS) $< -o $@

# To remove generated files
clean:
	rm -f $(EXEC) *.o *.pb.*
