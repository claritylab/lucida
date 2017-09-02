include ../../Makefile.common
fname=djinn-weights-1.0.tar.gz

all:
	cd tools \
	&& sh install-protobuf.sh \
	&& sh install-caffe.sh \
	&& cp common.hpp caffe/distribute/include/caffe/common.hpp \
	&& cd .. \
	&& if [ ! -d "gen-cpp2" ]; then \
		python -mthrift_compiler.main --gen cpp2 ../lucidaservice.thrift; \
		python -mthrift_compiler.main --gen cpp2 ../lucidatypes.thrift; \
	fi \
	&& cd models \
	&& if [ ! -f face.caffemodel ]; then wget -c http://web.eecs.umich.edu/~jahausw/download/${fname} && tar xvzf ${fname} && rm -rf ${fname}; if [ $? -ne 0 ]; then echo "Could not download caffe models!!! Please try again later..."; exit 1; fi; fi \
        && export CXXFLAGS="-I/usr/include/hdf5/serial" \
	&& cd ../imc \
	&& make \
	&& cd ../dig \
	&& make \
	&& cd ../face \
	&& make \
	&& cd ..

clean:
	cd dig \
	&& make clean \
	&& cd ../face \
	&& make clean \
	&& cd ../imc \
	&& make clean \
	&& cd .. \
	&& rm -rf gen-cpp2

%:
	@:
