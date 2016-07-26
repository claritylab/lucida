# Tools (Dependencies)

This directory contains common tools that are used by Lucida micro-services, 
so please install all of them before compiling any micro-service.

## Install

Type `make` to install all dependencies necessary for Lucida in the following order:

- apt_deps.sh: various packages installed using `apt-get` and `pip2`. 

- install_java.sh: Java 8

- install_opencv.sh: [OpenCV](http://opencv.org/)

- install_thrift.sh: [Apache Thrift 0.9.3](https://thrift.apache.org/)

- install_fbthrift.sh: [Facebook Thrift](https://github.com/facebook/fbthrift)

- install_mongodb.sh: [MongoDB](https://www.mongodb.com/)
and [C++ legacy driver](https://github.com/mongodb/mongo-cxx-driver/tree/legacy)

## Notes

1. This setup has been tested for Ubuntu 14.04 (64 bit), gcc 4.8, and Python 2.7.6, but
you are welcome to improve the build system.
For example, the current idea of putting shared dependencies in this directory
makes removing a service from Lucida hard. If you have a better way to solve
dependency issues for multiple services within one Github repo, please make a pull request.

2. Each script performs a simple check on whether the package is
installed. If for some reason the installation failed, or the simple check
is not sufficient and you want to force reinstallation,
please either ```sudo ./install_xxx.sh```, 
or open the script and run the commands manually to make sure each command succeeds.

3. Both Apache Thrift and Facebook are necessary, and you must install Apache Thrift first,
and then install Facebook Thrift. The `Makefile` guarantees that, but
if for some reason you reverse the order (both compiled and installed),
simply go back to Facebook Thrift: `cd fbthrift/thrift/`
and `sudo make install` which should not take too long because it is already compiled.
