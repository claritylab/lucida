CC_DIR=$(LUCIDAROOT)/commandcenter/gen-cpp
# Makefile for generating the kaldi-thrift service
CXX = g++
FLAGS = --std=c++11 -O2
TARGET_SERVER = asr_server
LINK_FLAGS = -lthrift -lsox

# Needed for finding header files during preprocessing
CMDOBJECT_SERVER = $(CC_DIR)/CommandCenter.o \
                  $(CC_DIR)/commandcenter_types.o

OBJECTS_SERVER = gen-cpp/LucidaService.o gen-cpp/lucidatypes_types.cpp Kaldi_Server.o $(CMDOBJECT_SERVER)

debug: FLAGS += -g3 -DDEBUG
debug: clean all

all: thrift  $(TARGET_SERVER)

$(TARGET_SERVER): $(OBJECTS_SERVER)
	$(CXX) -g $(FLAGS) -I$(CC_DIR) $(OBJECTS_SERVER) -o $(TARGET_SERVER) $(LINK_FLAGS)
thrift:
	thrift --gen cpp lucidaservice.thrift
	thrift --gen cpp lucidatypes.thrift
%.o: %.cpp
	$(CXX) -g $(FLAGS) -I$(CC_DIR) -c $(CXX_FLAGS) $(EXTRA_FLAGS) $< -o $@

clean:
	rm -rf $(TARGET_CLIENT) $(TARGET_SERVER) gen-cpp *.o *~ *wav

.PHONY:	all
