COMMON=../tonic-common
IMG-COMMON=common-img
export LIBRARY_PATH=/usr/local/lib
export LD_LIBRARY_PATH=/usr/local/lib

SRC=common-img/src
GEN=../gen-cpp

CXX = g++ -std=c++11
CXX_FLAGS  = -O3 \
						 -w \
						 -fpermissive \
						 -I$(COMMON)/include \
						 -I$(CAFFE)/include \
						 -Iz \
						 -I/usr/local/include \

ifeq ($(CPU_ONLY), 1)
    CXX_FLAGS  += -DCPU_ONLY
endif

LINK_FLAGS = $(COMMON)/libtonic.a \
             -lopencv_core \
             -lopencv_highgui \
             -lopencv_imgproc \
             -lopencv_flann \
             -lopencv_objdetect \
             -lopencv_features2d \
             -lopencv_gpu \
             -lrt \
             -lboost_program_options \
             -lboost_filesystem \
             -lboost_system \
             -lthrift \
             -lboost_thread \
						 -lglog \
             -lpthread \
						 -lthrift \
             $(IMG-COMMON)/libflandmark_static.a \
             $(CAFFE)/lib/libcaffe.so

# File names
IMG_SERVICE		= IMGService
IMG_CLIENT 		= testClient
SOURCES = $(wildcard $(SRC)/*.cpp) $(wildcard $(GEN)/*.cpp) 
IMG_CLIENT_OBJ 		= $(SOURCES:.cpp=.o) TestImageClient.o
IMG_SERVICE_OBJ 	= $(SOURCES:.cpp=.o) IMGService.o

.PHONY: all common-img clean
# Main target
all: common-img $(IMG_SERVICE) $(IMG_CLIENT)

common-img:
	make -C $@ all

$(IMG_CLIENT): $(IMG_CLIENT_OBJ) TestImageClient.cpp;
	  $(CXX) $(IMG_CLIENT_OBJ) -o $(IMG_CLIENT) $(CXX_FLAGS)  $(LINK_FLAGS)

$(IMG_SERVICE): $(IMG_SERVICE_OBJ) IMGService.cpp;
	  $(CXX) $(IMG_SERVICE_OBJ) -o $(IMG_SERVICE) $(CXX_FLAGS)  $(LINK_FLAGS)

# To obtain object files
%.o: %.cpp Makefile ;  $(CXX) -I$(INCLUDE_DIRS) -c $(CXX_FLAGS) $(EXTRA_FLAGS) $< -o $@

# To remove generated files
clean:
	rm -f $(IMG_SERVICE) $(IMG_CLIENT) $(GEN)/*.o $(COMMON)/src/*.o *.o
	make -C common-img clean
