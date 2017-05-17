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
 && git checkout v2017.03.20.00 \
 && echo "a1abbb7abcb259acbd94d0d0929b79607a8ce806" > ./build/FOLLY_VERSION \
 && echo "a5503c88e1d6799dcfb337caf09834a877790c92" > ./build/WANGLE_VERSION \
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
