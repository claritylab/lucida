include ../../Makefile.common
fname=djinn-weights-1.0.tar.gz

all: 
	cd tools; \
	./install-protobuf.sh; \
	./install-caffe.sh; \
	cp common.hpp caffe/distribute/include/caffe/common.hpp; \
	cd ..; \
	python -mthrift_compiler.main --gen cpp2 ../lucidaservice.thrift; \
	python -mthrift_compiler.main --gen cpp2 ../lucidatypes.thrift; \
	cd models; \
	wget http://web.eecs.umich.edu/~jahausw/download/${fname}; \
	tar xvzf ${fname}; \
	rm -rf ${fname}; \
	cd ../imc; \
	make; \
	cd ../dig; \
	make; \
	cd ../face; \
	make

%:
	@:
