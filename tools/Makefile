include ../Makefile.common

all-here::
	sudo ./apt_deps.sh
	sudo ./install_java7.sh
	./install_protobuf.sh
	./install_opencv.sh
	./install_caffe.sh
	./install_thrift.sh

clean-here::
	./clean.sh
