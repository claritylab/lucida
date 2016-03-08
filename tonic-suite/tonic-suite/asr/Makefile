include ../../../Makefile.common

all-here::
	cd tools && ./atlas_header.sh && ./install_openfst.sh && cd -;
	cd src && ./configure && make && cd -;
	cd tonic-suite/asr/egs/voxforge/s5/exp/ && tar -xvzf tri3b.tar.gz

clean-here::
	cd src && make clean && cd -
	rm -f *.pyc
