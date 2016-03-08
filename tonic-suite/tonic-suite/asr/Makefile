include ../../../Makefile.common

all-here::
	cd tools && ./atlas_header.sh && ./install_openfst.sh && cd -;
	cd src && ./configure && make -j$(THREADS) && cd -;
	cd egs/voxforge/s5/exp/ && tar -xvzf tri3b.tar.gz

test-here::
	./tonic-asr --network asr.prototxt --weight asr.caffemodel --input asr-list.txt --djinn 0

clean-here::
	cd src && make clean && cd -
	rm -f *.pyc
