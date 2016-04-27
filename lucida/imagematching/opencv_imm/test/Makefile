CXX = g++
# Needed for linking
GEN_DIR = ./gen-cpp
COMMON_DIR = ../opencv

CXX_FLAGS = -std=c++11
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
						 -lboost_thread \
						 -lboost_regex \
						 -lprotobuf \
						 -ltesseract \
						 -lthrift
 
TARGET_CLIENT = imm_client

OBJECTS_SERVER =    ./gen-cpp/lucidatypes_types.o \
					./gen-cpp/LucidaService.o \
					../opencv/detect.o \
					../opencv/kp_protobuf.o \
					../opencv/keypoints.pb.cc \
					../opencv/store_mat.pb.cc \
					imm_client.o

all: CXX_FLAGS += -O3
all: thrift $(TARGET_CLIENT)

debug: CXX_FLAGS += -g3
debug: thrift $(TARGET_CLIENT)

thrift:
	@if [ ! -d "gen-cpp" ]; then \
       thrift --gen cpp ../../../lucidaservice.thrift; \
       thrift --gen cpp ../../../lucidatypes.thrift; \
    fi

$(TARGET_CLIENT): $(OBJECTS_SERVER)
	$(CXX) $(CXX_FLAGS) $(OBJECTS_SERVER) -o $(TARGET_CLIENT) $(LINK_FLAGS)

%.o: %.cpp
	$(CXX) -I$(GEN_DIR) -I$(COMMON_DIR) -c $(CXX_FLAGS) $< -o $@

clean:
	rm -rf $(TARGET_CLIENT) *.o gen-cpp

.PHONY:	all debug thrift clean
