SUBDIRS=kaldi test
include ../../../Makefile.common

# CC_DIR=$(LUCIDAROOT)/commandcenter/gen-cpp
# Makefile for generating the kaldi-thrift service
CXX = g++
FLAGS = --std=c++11 -O3
TARGET_SERVER = asr_server
LINK_FLAGS = -lthrift -lsox

# Needed for finding header files during preprocessing
# CMDOBJECT_SERVER = $(CC_DIR)/CommandCenter.o \
                  $(CC_DIR)/commandcenter_types.o

OBJECTS_SERVER = gen-cpp/LucidaService.o gen-cpp/lucidatypes_types.o Kaldi_Server.o

debug: FLAGS += -g3
debug: clean all

all: thrift $(TARGET_SERVER)

$(TARGET_SERVER): $(OBJECTS_SERVER)
	$(CXX) -g $(FLAGS) $(OBJECTS_SERVER) -o $(TARGET_SERVER) $(LINK_FLAGS)
thrift:
	@if [ ! -d "gen-cpp" ]; then \
       thrift --gen cpp ../../lucidaservice.thrift; \
       thrift --gen cpp ../../lucidatypes.thrift; \
    fi
%.o: %.cpp
	$(CXX) -g $(FLAGS) -c $(CXX_FLAGS) $(EXTRA_FLAGS) $< -o $@

clean:
	rm -rf $(TARGET_CLIENT) $(TARGET_SERVER) gen-cpp *.o *~ *.wav

.PHONY:	all
