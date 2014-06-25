#!/bin/bash

# set this
opencv=opencv/opencv-2.4.9/build

export CV_DIR=$HOME/$opencv
export PKG_DIR=`pwd`/pkgconfig

# add to globals
export PATH=$PATH:$CV_DIR
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$PKG_DIR
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PKG_DIR

make
