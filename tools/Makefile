include ../Makefile.common

all-here::
	sudo ./apt_deps.sh
	sudo ./install_java.sh
	./install_protobuf.sh
	./install_opencv.sh
	./install_caffe.sh
	./install_thrift.sh
	./install_fbthrift.sh
	./install_mongodb.sh

clean-here::
	./clean.sh
