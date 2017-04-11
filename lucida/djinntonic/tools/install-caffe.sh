#!/bin/bash
## Install Djinn specific version of Caffe
which nvcc
if [ $? -ne 0 ]; then
  export CPU_ONLY=1
fi

if [ -z "$THREADS" ]; then
  THREADS=`nproc`
fi

if [ ! -d caffe ]; then
  git clone https://github.com/jhauswald/caffe.git
  if [ $? -ne 0 ]; then
    echo "Could not clone caffe!!! Please try again later..."
    exit 1;
  fi
fi
cd caffe \
  && git checkout ipa \
  && cp Makefile.config.example Makefile.config \
  && export COMMON_FLAGS="-I/usr/include/hdf5/serial" \
  && sed s/NOT_IMPLEMENTED/NOT_IMPLMENTED_YET/g src/caffe/util/math_functions.cu > math_functions.cu \
  && mv math_functions.cu src/caffe/util/math_functions.cu \
  && pushd . \
  && cd /usr/lib/x86_64-linux-gnu \
  && if [ ! -f libhdf5.so ]; then sudo ln -s libhdf5_serial.so libhdf5.so; fi \
  && if [ ! -f libhdf5_hl.so ]; then sudo ln -s libhdf5_serial_hl.so libhdf5_hl.so; fi \
  && popd \
  && make -j$THREADS \
  && make distribute
