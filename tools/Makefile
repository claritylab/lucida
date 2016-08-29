include ../Makefile.common

all:
	(sudo ./apt_deps.sh) || (exit 1)
	(sudo ./install_python.sh) || (exit 1)
	(sudo ./install_java.sh) || (exit 1)
	(sudo ./install_opencv.sh) || (exit 1)
	(sudo ./install_thrift.sh) || (exit 1)
	(sudo ./install_fbthrift.sh) || (exit 1)
	(sudo ./install_mongodb.sh) || (exit 1)

clean:
	./clean.sh

