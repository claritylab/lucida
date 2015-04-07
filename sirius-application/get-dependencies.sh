#!/usr/bin/env bash

# Install (some) dependencies
# run as sudo

# Add additional repositories (ffmpeg)
add-apt-repository ppa:kirillshkrogalev/ffmpeg-next

# doesn't exist anymore
# add-apt-repository ppa:jon-severinsson/ffmpeg

# Enable multiverse sources (libfaac-dev)
apt-add-repository multiverse

# Update sources and install basics
apt-get update
apt-get -y install \
  git zip unzip subversion sox \
  default-jdk ant automake autoconf libtool bison libboost-all-dev ffmpeg \
  swig python-pip curl

# Get opencv dependencies
apt-get -y install \
  build-essential checkinstall git cmake libfaac-dev libjack-jackd2-dev \
  libmp3lame-dev libopencore-amrnb-dev libopencore-amrwb-dev libsdl1.2-dev \
  libtheora-dev libva-dev libvdpau-dev libvorbis-dev libx11-dev libxfixes-dev \
  libxvidcore-dev texi2html yasm zlib1g-dev

# Get tessaract text recognition
apt-get -y install \
  tesseract-ocr tesseract-ocr-eng libtesseract-dev libleptonica-dev

# Get ATLAS library for Kaldi
apt-get -y \
	install libatlas-dev libatlas-base-dev

# Get protobuf for image-matching
apt-get -y \
	install libprotobuf-dev protobuf-compiler

# get deps for web application
pip install wtforms Flask requests pickledb
