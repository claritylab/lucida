CC_DIR=$(LUCIDAROOT)/commandcenter/gen-cpp

CXX = g++
PROTO = protoc

CXX_FLAGS = -O2
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
 
TARGET_CLIENT = imclient 
TARGET_SERVER = imserver

OBJECTS_CLIENT = ./gen-cpp/ImageMatchingService.o IMSClient.o 
OBJECTS_SERVER = ./gen-cpp/ImageMatchingService.o \
								 ../common/detect.o \
								 ../common/kp_protobuf.o \
								 ../common/keypoints.pb.cc \
								 ../common/store_mat.pb.cc \
								 IMSDaemon.o

# Needed for finding header files during preprocessing
GEN_DIR = ./gen-cpp
COMMON_DIR = ../common

all: proto thrift common $(TARGET_SERVER) $(TARGET_CLIENT)

proto:
	$(PROTO) --proto_path=$(COMMON_DIR) --cpp_out=$(COMMON_DIR) $(wildcard $(COMMON_DIR)/*.proto)

thrift:
	thrift --gen cpp service.thrift

$(TARGET_CLIENT): $(OBJECTS_CLIENT)
	$(CXX) $(CXX_FLAGS) $(OBJECTS_CLIENT) -o $(TARGET_CLIENT) $(LINK_FLAGS)

$(TARGET_SERVER): $(OBJECTS_SERVER)
	$(CXX) $(CXX_FLAGS) $(OBJECTS_SERVER) -o $(TARGET_SERVER) $(LINK_FLAGS)

common:
	$(CXX) -I$(CC_DIR) -I$(GEN_DIR) -I$(COMMON_DIR) -c $(CXX_FLAGS) $(EXTRA_FLAGS) $(COMMON_DIR)/detect.cpp -o $(COMMON_DIR)/detect.o
	$(CXX) -I$(CC_DIR) -I$(GEN_DIR) -I$(COMMON_DIR) -c $(CXX_FLAGS) $(EXTRA_FLAGS) $(COMMON_DIR)/kp_protobuf.cpp -o $(COMMON_DIR)/kp_protobuf.o

%.o: %.cpp
	$(CXX) -I$(CC_DIR) -I$(GEN_DIR) -I$(COMMON_DIR) -c $(CXX_FLAGS) $(EXTRA_FLAGS) $< -o $@

clean:
	rm -rf $(TARGET_CLIENT) $(TARGET_SERVER) *.o

.PHONY:	all
