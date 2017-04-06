#!/bin/bash
## Install Djinn specific version of Caffe
if [ -z "$THREADS" ]; then
  THREADS=4
fi

if [ -d caffe ]; then
  echo "Caffe already installed, skipping"
  exit
fi

git clone https://github.com/jhauswald/caffe.git \
  && cd caffe \
  && git checkout ipa \
  && cp Makefile.config.example Makefile.config \
  && export CXXFLAGS="-I/usr/include/hdf5/serial" \
  && mv src/caffe/util/math_functions.cu temp.cu \
  && sed s/NOT_IMPLEMENTED/NOT_IMPLMENTED_YET/g temp.cu > src/caffe/util/math_functions.cu \
  && rm -f temp.cu \
  && pushd . \
  && cd /usr/lib/x86_64-linux-gnu \
  && sudo ln -s libhdf5_serial.so libhdf5.so \
  && sudo ln -s libhdf5_serial_hl.so libhdf5_hl.so \
  && popd \
  && make -j$THREADS \
  && make distribute
