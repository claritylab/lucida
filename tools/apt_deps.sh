## Installs all package manager dependencies
## FROM ubuntu:14.04
## Run as sudo

sed 's/main$/main universe/' -i /etc/apt/sources.list
apt-get update
apt-get install -y software-properties-common \
                   gfortran \
                   make \
                   ant \
                   gcc \
                   g++ \
                   wget \
                   automake \
                   git \
                   curl \
                   libboost-all-dev \
                   libevent-dev \
                   libtool \
                   pkg-config \
                   libtesseract-dev \
                   libopenblas-dev \
                   libblas-dev \
                   libatlas-dev \
                   libatlas-base-dev \
                   liblapack-dev \
                   cmake \
                   zip \
                   unzip \
                   sox \
                   libsox-dev \
                   autoconf \
                   bison \
                   swig \
                   python-pip \
                   subversion \
                   libssl-dev \
                   libprotoc-dev \
                   supervisor \
                   flac \
                   gawk \
                   imagemagick \
                   libgflags-dev libgoogle-glog-dev liblmdb-dev \
                   libleveldb-dev libsnappy-dev libhdf5-serial-dev \
                   bc \
                   python-numpy
