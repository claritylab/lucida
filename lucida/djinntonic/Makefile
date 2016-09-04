include ../../Makefile.common
fname=djinn-weights-1.0.tar.gz

all:
	cd tools; \
	sh install-protobuf.sh; \
	sh install-caffe.sh; \
	cp common.hpp caffe/distribute/include/caffe/common.hpp; \
	cd ..; \
	if [ ! -d "gen-cpp2" ]; then \
		python -mthrift_compiler.main --gen cpp2 ../lucidaservice.thrift; \
		python -mthrift_compiler.main --gen cpp2 ../lucidatypes.thrift; \
	fi; \
	cd models; \
	test ! -s face.caffemodel && wget http://web.eecs.umich.edu/~jahausw/download/${fname} && tar xvzf ${fname} && rm -rf ${fname}; \
	cd ../imc; \
	make; \
	cd ../dig; \
	make; \
	cd ../face; \
	make; \
	cd ..; \

%:
	@:
