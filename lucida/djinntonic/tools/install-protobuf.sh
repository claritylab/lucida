#!/bin/bash
export PROTOBUF_VERSION=2.5.0

if [ -z "$THREADS" ]; then
  THREADS=`nproc`
fi

if [ ! -d protobuf-$PROTOBUF_VERSION ]; then
  wget -c "https://github.com/google/protobuf/releases/download/v$PROTOBUF_VERSION/protobuf-$PROTOBUF_VERSION.tar.gz" && tar xf protobuf-$PROTOBUF_VERSION.tar.gz
  if [ $? -ne 0 ]; then
    echo "Could not download protobuf!!! Please try again later..."
    exit 1;
  fi
fi
cd protobuf-$PROTOBUF_VERSION \
  && ./configure \
  && make -j$THREADS \
  && sudo make install \
  && sudo ldconfig
