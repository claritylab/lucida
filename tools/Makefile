include ../Makefile.common

all-here::
	(sudo ./apt_deps.sh) || (exit 1)
	(sudo ./install_java.sh) || (exit 1)
	(sudo ./install_opencv.sh) || (exit 1)
	(sudo ./install_thrift.sh) || (exit 1)
	(sudo ./install_fbthrift.sh) || (exit 1)
	(sudo ./install_mongodb.sh) || (exit 1)

clean-here::
	./clean.sh

