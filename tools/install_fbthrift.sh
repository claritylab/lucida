#!/bin/bash
installCheck () {
  if [ ! -d fbthrift ]; then
    return 1
  fi
  python -mthrift_compiler.main --gen cpp2 check_thrift.thrift
  if [ -d gen-cpp2 ]; then
    rm -rf gen-cpp2
    return 0
  else
    return 1
  fi
}

if installCheck "$0"; then
  echo "Facebook Thrift installed";
  exit 0;
fi

if [ ! -d fbthrift ]; then
  git clone https://github.com/facebook/fbthrift.git
  if [ $? -ne 0 ]; then
    echo "Could not clone FBThrift!!! Please try again later..."
    exit 1
  fi
fi

cd fbthrift/thrift \
 && git checkout b2c1f8ed2937c04d9d7de6b07fc6303aec67fb46 \
 && echo "d6cd4a4a1502a57022ecfd83a988b87512613c06" > ./build/FOLLY_VERSION \
 && echo "cfb38af8c1e4b27e4405c3953212379b13521e5a" > ./build/WANGLE_VERSION \
 && ./build/deps_ubuntu_14.04.sh \
 && autoreconf -ivf \
 && ./configure \
 && make \
 && make install \
 && cd ../..

if installCheck "$0"; then
  echo "Facebook Thrift installed";
  exit 0;
else
  echo "Failed to install Facebook Thrift";
  exit 1;
fi
