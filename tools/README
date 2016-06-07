# Tools (Dependencies)

1. This setup is for Ubuntu 14.04.

2. Each script performs a simple check on whether the package is
 installed. If for some reason the install failed, or the simple check
 is not sufficient and you want to force reinstallation,
 please openthe script and run the commands manually to make sure each command succeeds.

Type `make` to install all dependencies necessary for Lucida in the following order:

- apt_deps.sh: packages installed using apt-get and pip.
Python 2 is used and `scikit-learn`, `numpy`, and `pandas` take most of the time.

- install_java.sh: Java 7 for QA and CA

- install_opencv.sh: [OpenCV](http://opencv.org/) for IMM

- install_thrift.sh: [Apache Thrift 0.9.3](https://thrift.apache.org/) for QA, and CA

- install_fbthrift.sh: [Facebook Thrift](https://github.com/facebook/fbthrift) for IMM

- install_mongodb.sh: [MongoDB](https://www.mongodb.com/)
 and [C++ legacy driver](https://github.com/mongodb/mongo-cxx-driver/tree/legacy) for CMD and IMM

## Notes

1. It is likely that you will encounter problems, especially when installing Facebook Thrift
  and MongoDB C++ driver. You're welcome to open up issues and we will try our best to help. 

2. Both Apache Thrift and Facebook are necessary. To make sure IMM
  compiles successfully, you must install Apache Thrift, and then install Facebook Thrift.
  If you reverse the order, and already `make` successfully for both,
  simply go back to Facebook Thrift: `cd fbthrift/thrift/`
  and `sudo make install` which should not take long.
  
