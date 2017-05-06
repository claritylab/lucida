#!/bin/bash
if [ -z $THREADS ]; then
  THREADS=`nproc`
fi

export THRIFT_VERSION=0.9.3
THREADS=1

installCheck () {
  if [ ! -d thrift-$THRIFT_VERSION ]; then
    return 1
  fi
  thrift --gen java check_thrift.thrift
  if [ -d gen-java ]; then
    rm -rf gen-java
    return 0
  else
    return 1
  fi
}

if installCheck "$0"; then
  echo "Apache Thrift installed";
  exit 0;
fi

sudo apt-get remove -y thrift-compiler

if [ ! -d thrift-$THRIFT_VERSION ]; then
  wget -c "http://archive.apache.org/dist/thrift/$THRIFT_VERSION/thrift-$THRIFT_VERSION.tar.gz" && tar xf thrift-$THRIFT_VERSION.tar.gz
  if [ $? -ne 0 ]; then
    echo "Could not download Thrift!!! Please try again later..."
    exit 1
  fi
fi

cd thrift-$THRIFT_VERSION \
  && ./configure --with-lua=no --with-ruby=no --with-go=no --with-erlang=no --with-nodejs=no --with-qt4=no --with-qt5=no \
  && make -j$THREADS \
  && make -j$THREADS install \
  && cd lib/py/ \
  && python setup.py install \
  && cd ../../lib/java/ \
  && ant \
  && cd ../../..

if installCheck "$0"; then
  echo "Apache Thrift installed";
  exit 0;
else
  echo "Failed to install Apache Thrift";
  exit 1;
fi
