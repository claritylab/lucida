include ../../Makefile.common

all: 
	cd tools; \
	make; \
	cd ..; \
	python -mthrift_compiler.main --gen cpp2 lucidaservice.thrift; \
	python -mthrift_compiler.main --gen cpp2 lucidatypes.thrift; \
	cd models; \
	fname=djinn-weights-1.0.tar.gz; \
	wget http://web.eecs.umich.edu/~jahausw/download/$fname; \
	tar xvzf $fname; \
	rm -rf $fname; \
	cd ../imc; \
	make; \
	cd ../dig; \
	make; \
	cd ../face; \
	make

%:
	@:
