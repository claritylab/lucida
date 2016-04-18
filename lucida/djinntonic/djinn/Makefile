COMMON=../tonic-common

GEN=../gen-cpp

CXX = g++
CXX_FLAGS  = -O3 \
						 -w \
						 -fpermissive \
						 -I$(COMMON)/include \
						 -I$(CAFFE)/include \
						 -Iz \
						 -I/usr/local/include \
						 -I$(GEN)

ifeq ($(CPU_ONLY), 1)
    CXX_FLAGS  += -DCPU_ONLY
endif

LINK_FLAGS = $(COMMON)/libtonic.a \
             -lrt \
             -lboost_program_options \
             -lboost_filesystem \
             -lboost_system \
             -lopencv_core \
             -lopencv_highgui \
             -lopencv_imgproc \
             -lthrift \
             -lboost_thread \
						 -lglog \
             -lpthread \
             $(CAFFE)/lib/libcaffe.so \
						 -lthrift \
						 -lprotobuf \
						 -lleveldb \
						 -llmdb

# File names
DJINN_SERVICE = DjinnService

SOURCES             = $(wildcard $(COMMON)/src/*.cpp) $(wildcard $(GEN)/*.cpp) 
DJINN_SERVICE_OBJ 	= $(SOURCES:.cpp=.o) DjinnService.o utils.o
DJINN_TEST_OBJ    	= $(SOURCES:.cpp=.o) DjinnTestClient.o

all: $(DJINN_SERVICE)

# Main target
$(DJINN_SERVICE): $(DJINN_SERVICE_OBJ) DjinnService.cpp utils.cpp;
	  $(CXX) $(DJINN_SERVICE_OBJ)  -o $(DJINN_SERVICE)  $(CXX_FLAGS) $(LINK_FLAGS)

# To obtain object files
%.o: %.cpp Makefile ;  $(CXX) -I$(INCLUDE_DIRS) -c $(CXX_FLAGS) $(EXTRA_FLAGS) $< -o $@

# To remove generated files
clean:
	rm -f $(DJINN_SERVICE) $(GEN)/*.o $(COMMON)/src/*.o *.o
