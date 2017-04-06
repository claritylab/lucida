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

git clone https://github.com/facebook/fbthrift.git
cd fbthrift/thrift/
git checkout d811b530a4f5e11a520f5fb416a5a3a8a5f42ef8
./build/deps_ubuntu_14.04.sh
autoreconf -ivf
./configure
sudo make
sudo make install
cd ../..

if installCheck "$0"; then
  echo "Facebook Thrift installed";
  exit 0;
else
  echo "Failed to install Facebook Thrift";
  exit 1;
fi
