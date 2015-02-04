CAFFE=/usr/local/include/caffe/
CUDA=/usr/local/cuda
CXX=g++
CXX_FLAGS = -w -fpermissive -I$(CAFFE) -I$(CUDA)/include -g
LINK_FLAGS = -L/$(CUDA)/lib64 -lcudart -ldl -lrt

all: dnn-asr

dnn-asr: $(OBJECTS) Makefile
	$(CXX) $(OBJECTS) -o dnn-asr $(LINK_FLAGS) /usr/local/lib/libcaffe.so

%.o: %.cpp Makefile
	$(CXX) -c $(CXX_FLAGS) $< -o $@

clean:
	rm -f dnn-asr *.o 
