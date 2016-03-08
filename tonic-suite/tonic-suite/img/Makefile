COMMON=../../common
include $(COMMON)/Makefile.config

CXX = g++
CXX_FLAGS  = -O3 \
						 $(INCLUDE_DIRS) \
						 -fpermissive \
						 -I$(COMMON)/include \
						 -I$(CAFFE)/include \
						 -I$(CUDA)/include

ifeq ($(CPU_ONLY), 1)
	CXX_FLAGS  += -DCPU_ONLY
endif

LINK_FLAGS = $(LIBRARY_DIRS) \
						 -lopencv_core \
						 -lopencv_highgui \
						 -lopencv_imgproc \
						 -lopencv_objdetect \
						 $(COMMON)/libtonic.a \
						 libflandmark_static.a \
						 -lboost_program_options \
						 -lboost_filesystem \
						 -lboost_system \
						 -lglog \
						 -lrt \
						 -lpthread \
						 $(CAFFE)/lib/libcaffe.so

SRC=src

# File names
EXEC = tonic-img
SOURCES = $(wildcard $(SRC)/*.cpp) $(COMMON)/src/tonic.cpp
OBJECTS = $(SOURCES:.cpp=.o)

all: $(EXEC)

test: test-here

# Main target
$(EXEC): $(OBJECTS) Makefile
	./get-flandmark.sh
	$(CXX) $(OBJECTS) -o $(EXEC) $(LINK_FLAGS)

# To obtain object files
%.o: %.cpp Makefile
	$(CXX) -c $(CXX_FLAGS) $(EXTRA_FLAGS) $< -o $@

test-here::
	./tonic-img --task imc --network imc.prototxt --weight imc.caffemodel --input imc-list.txt --djinn 0
	./tonic-img --task dig --network dig.prototxt --weight dig.caffemodel --input dig-list.txt --djinn 0
	./tonic-img --task face --network face.prototxt --weight face.caffemodel --input face-list.txt --djinn 0 --align 0

# To remove generated files
clean:
	rm -f $(EXEC) $(SRC)/*.o $(COMMON)/src/*.o
