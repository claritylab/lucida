CC_DIR=$(LUCIDAROOT)/commandcenter/gen-cpp
CXX = g++
PROTO = protoc
# Needed for linking
GEN_DIR = ./gen-cpp
COMMON_DIR = ../common

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
 
TARGET_SERVER = imm_server

CMDOBJECT_SERVER = $(CC_DIR)/CommandCenter.o \
                  $(CC_DIR)/commandcenter_types.o
# CMDOBJECT_SERVER = CommandCenter.o commandcenter_types.o
OBJECTS_SERVER = $(CMDOBJECT_SERVER) \
					./gen-cpp/ImageMatchingService.o \
					../common/detect.o \
					../common/kp_protobuf.o \
					../common/keypoints.pb.cc \
					../common/store_mat.pb.cc \
					IMSDaemon.o

all: proto thrift common $(TARGET_SERVER)

proto:
	$(PROTO) --proto_path=$(COMMON_DIR) --cpp_out=$(COMMON_DIR) $(wildcard $(COMMON_DIR)/*.proto)

thrift:
	thrift --gen cpp service.thrift

$(TARGET_SERVER): $(OBJECTS_SERVER)
	$(CXX) $(CXX_FLAGS) $(OBJECTS_SERVER) -o $(TARGET_SERVER) $(LINK_FLAGS)

common:
	$(CXX) -I$(CC_DIR) -I$(GEN_DIR) -I$(COMMON_DIR) -c $(CXX_FLAGS) $(EXTRA_FLAGS) $(COMMON_DIR)/detect.cpp -o $(COMMON_DIR)/detect.o
	$(CXX) -I$(CC_DIR) -I$(GEN_DIR) -I$(COMMON_DIR) -c $(CXX_FLAGS) $(EXTRA_FLAGS) $(COMMON_DIR)/kp_protobuf.cpp -o $(COMMON_DIR)/kp_protobuf.o

%.o: %.cpp
	$(CXX) -I$(CC_DIR) -I$(GEN_DIR) -I$(COMMON_DIR) -c $(CXX_FLAGS) $(EXTRA_FLAGS) $< -o $@

clean:
	rm -rf $(TARGET_SERVER) *.o

.PHONY:	all
