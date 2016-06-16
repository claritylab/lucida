####
# Builds the lucida base image
FROM ubuntu:14.04

#### environment variables
ENV LUCIDAROOT /usr/local/lucida/lucida
ENV LD_LIBRARY_PATH /usr/local/lib

ENV OPENCV_VERSION 2.4.9
ENV THRIFT_VERSION 0.9.3
ENV THREADS 4
ENV PROTOBUF_VERSION 2.5.0
ENV JAVA_VERSION 8
ENV JAVA_TOOL_OPTIONS=-Dfile.encoding=UTF8 

#### common package installations
RUN sed 's/main$/main universe/' -i /etc/apt/sources.list
RUN apt-get update
RUN apt-get install -y zlib1g-dev
RUN apt-get install -y libatlas3-base
RUN apt-get install -y python2.7-dev
RUN apt-get install -y libblas3
RUN apt-get install -y libblas-dev
RUN apt-get install -y liblapack3
RUN apt-get install -y liblapack-dev
RUN apt-get install -y libc6
RUN apt-get install -y software-properties-common
RUN apt-get install -y gfortran
RUN apt-get install -y make
RUN apt-get install -y ant
RUN apt-get install -y gcc
RUN apt-get install -y g++
RUN apt-get install -y wget
RUN apt-get install -y automake
RUN apt-get install -y git
RUN apt-get install -y curl
RUN apt-get install -y libboost-all-dev
RUN apt-get install -y libevent-dev
RUN apt-get install -y libdouble-conversion-dev
RUN apt-get install -y libtool
RUN apt-get install -y liblz4-dev
RUN apt-get install -y liblzma-dev
RUN apt-get install -y binutils-dev
RUN apt-get install -y libjemalloc-dev
RUN apt-get install -y pkg-config
RUN apt-get install -y libtesseract-dev
RUN apt-get install -y libopenblas-dev
RUN apt-get install -y libblas-dev
RUN apt-get install -y libatlas-dev
RUN apt-get install -y libatlas-base-dev
RUN apt-get install -y libiberty-dev
RUN apt-get install -y liblapack-dev
RUN apt-get install -y cmake
RUN apt-get install -y zip
RUN apt-get install -y unzip
RUN apt-get install -y sox
RUN apt-get install -y libsox-dev
RUN apt-get install -y autoconf
RUN apt-get install -y autoconf-archive
RUN apt-get install -y bison
RUN apt-get install -y swig
RUN apt-get install -y python-pip
RUN apt-get install -y subversion
RUN apt-get install -y libssl-dev
RUN apt-get install -y libprotoc-dev
RUN apt-get install -y supervisor
RUN apt-get install -y flac
RUN apt-get install -y gawk
RUN apt-get install -y imagemagick
RUN apt-get install -y libgflags-dev libgoogle-glog-dev liblmdb-dev
RUN apt-get install -y libleveldb-dev libsnappy-dev libhdf5-serial-dev
RUN apt-get install -y bc
RUN apt-get install -y python-numpy
RUN apt-get install -y flex
RUN apt-get install -y libkrb5-dev
RUN apt-get install -y libsasl2-dev
RUN apt-get install -y libnuma-dev
RUN apt-get install -y scons

#### package specific routines
RUN \
  echo oracle-java$JAVA_VERSION-installer shared/accepted-oracle-license-v1-1 select true | debconf-set-selections && \
  add-apt-repository -y ppa:webupd8team/java && \
  apt-get update && \
  apt-get install -y oracle-java$JAVA_VERSION-installer && \
  rm -rf /var/lib/apt/lists/* && \
  rm -rf /var/cache/oracle-jdk$JAVA_VERSION-installer

## install MongoDB, OpenCV, Thrift, and FBThrift
RUN mkdir -p /usr/local/lucida
ADD . /usr/local/lucida/tools
WORKDIR "/usr/local/lucida/tools"
RUN /bin/bash apt_deps.sh
RUN /bin/bash install_mongodb.sh
RUN /bin/bash install_opencv.sh
RUN /bin/bash install_thrift.sh
RUN /bin/bash install_fbthrift.sh
RUN \
  rm -rf mongo-c-driver/ && \
  rm -rf mongo-cxx-driver/ && \
  rm -rf fbthrift/ && \
  rm -rf libbson/ && \
  rm -rf opencv-2.4.9/
  