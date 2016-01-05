#!/bin/bash
# script must be run with sudo.

set -e

MAKE="make --jobs=$NUM_THREADS"

# This ppa is for gflags and glog
add-apt-repository -y ppa:tuleu/precise-backports
apt-get -y update
apt-get -y install \
    wget git curl \
    python-dev python-numpy \
    libleveldb-dev libsnappy-dev \
    libboost-all-dev \
    libprotobuf-dev protobuf-compiler \
    libblas-dev libopenblas-dev \
    libhdf5-serial-dev libgflags-dev libgoogle-glog-dev \
    thrift-compiler libthrift-java \
    libopencv-dev bison libatlas-dev libsox-dev libboost-all-dev \
    bc javacc ant

# hack
# sudo cp /usr/lib/openblas-base/* /usr/lib

# # Install CUDA, if needed
# if $WITH_CUDA; then
#   CUDA_URL=http://developer.download.nvidia.com/compute/cuda/repos/ubuntu1204/x86_64/cuda-repo-ubuntu1204_6.5-14_amd64.deb
#   CUDA_FILE=/tmp/cuda_install.deb
#   curl $CUDA_URL -o $CUDA_FILE
#   dpkg -i $CUDA_FILE
#   rm -f $CUDA_FILE
#   apt-get -y update
#   # Install the minimal CUDA subpackages required to test Caffe build.
#   # For a full CUDA installation, add 'cuda' to the list of packages.
#   apt-get -y install cuda-core-6-5 cuda-cublas-6-5 cuda-cublas-dev-6-5 cuda-cudart-6-5 cuda-cudart-dev-6-5 cuda-curand-6-5 cuda-curand-dev-6-5
#   # Create CUDA symlink at /usr/local/cuda
#   # (This would normally be created by the CUDA installer, but we create it
#   # manually since we did a partial installation.)
#   ln -s /usr/local/cuda-6.5 /usr/local/cuda
# fi
#
# # Install LMDB
# LMDB_URL=ftp://ftp.openldap.org/pub/OpenLDAP/openldap-release/openldap-2.4.39.tgz
# LMDB_FILE=/tmp/openldap.tgz
# pushd .
# curl $LMDB_URL -o $LMDB_FILE
# tar -C /tmp -xzvf $LMDB_FILE
# cd /tmp/openldap*/libraries/liblmdb/
# $MAKE
# $MAKE install
# popd
# rm -f $LMDB_FILE
#
# # Install the Python runtime dependencies via miniconda (this is much faster
# # than using pip for everything).
# wget http://repo.continuum.io/miniconda/Miniconda-latest-Linux-x86_64.sh -O miniconda.sh
# chmod +x miniconda.sh
# ./miniconda.sh -b
# export PATH=/home/travis/miniconda/bin:$PATH
# conda update --yes conda
# conda install --yes numpy scipy matplotlib scikit-image pip
# pip install protobuf
#
# # I'll just leave this cute comment here
# # Hey Johann, I don't want to put too much pressure on you, so I will
# # just put this comment here without telling you. (￢д￢)
# wget http://web.eecs.umich.edu/~jahausw/download/sirius-caffe.tar.gz
# tar xzf sirius-caffe.tar.gz
# cd sirius-caffe/
# if $WITH_CUDA; then
#   echo "CPU_ONLY := 0" >> Makefile.config
#   # Only generate compute_50.
#   GENCODE="-gencode arch=compute_50,code=sm_50"
#   GENCODE="$GENCODE -gencode arch=compute_50,code=compute_50"
#   echo "CUDA_ARCH := $GENCODE" >> Makefile.config
# fi
# sudo ./make-and-install.sh
