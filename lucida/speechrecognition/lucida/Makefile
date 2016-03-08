CC_DIR=$(LUCIDAROOT)/commandcenter/gen-cpp
# Makefile for generating the kaldi-thrift service
CXX = g++
FLAGS = --std=c++11 -O2
TARGET_SERVER = asr_server
LINK_FLAGS = -lthrift -lsox

# Needed for finding header files during preprocessing
CMDOBJECT_SERVER = $(CC_DIR)/CommandCenter.o \
                  $(CC_DIR)/commandcenter_types.o

OBJECTS_SERVER = gen-cpp/KaldiService.o Kaldi_Server.o $(CMDOBJECT_SERVER)

all: thrift  $(TARGET_SERVER)

$(TARGET_SERVER): $(OBJECTS_SERVER)
	$(CXX) $(FLAGS) -I$(CC_DIR) $(OBJECTS_SERVER) -o $(TARGET_SERVER) $(LINK_FLAGS)
thrift:
	thrift --gen cpp kaldi.thrift

%.o: %.cpp
	$(CXX) $(FLAGS) -I$(CC_DIR) -c $(CXX_FLAGS) $(EXTRA_FLAGS) $< -o $@

clean:
	rm -rf $(TARGET_CLIENT) $(TARGET_SERVER) gen-cpp *.o *~ *wav

.PHONY:	all
