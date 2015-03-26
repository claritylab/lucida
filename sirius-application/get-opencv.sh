#!/usr/bin/env bash

# Install OpenCV
# jahausw@umich.edu
# 2014

# run as sudo

hash wget 2>/dev/null || {
  echo >&2 "$0: [ERROR] wget is not installed. Aborting."
  exit 1
}

hash unzip 2>/dev/null || {
  echo >&2 "$0: [ERROR] unzip is not installed. Aborting."
  exit 1
}

NUM_THREADS=4
ver=2.4.9

git clone https://github.com/Itseez/opencv.git opencv-$ver
cd opencv-$ver
git checkout $ver
mkdir build
cd build
cmake ..
make -j$NUM_THREADS
make -j$NUM_THREADS install

