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

# Get correct opencv
apt-get install -y \
  build-essential checkinstall git cmake libfaac-dev libjack-jackd2-dev \
  libmp3lame-dev libopencore-amrnb-dev libopencore-amrwb-dev libsdl1.2-dev \
  libtheora-dev libva-dev libvdpau-dev libvorbis-dev libx11-dev libxfixes-dev \
  libxvidcore-dev texi2html yasm zlib1g-dev

git clone https://github.com/Itseez/opencv.git;
mv opencv opencv-$ver
cd opencv-$ver;
git checkout $ver;
mkdir build;
cd build;
cmake ..;
make -j$NUM_THREADS
make -j$NUM_THREADS install
