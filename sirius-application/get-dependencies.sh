#!/usr/bin/env bash

# Install (some) dependencies
# run as sudo

apt-get install \
  default-jdk ant automake autoconf libtool bison libboost-all-dev ffmpeg swig

# Get tessaract text recognition
apt-get install \
  tesseract-ocr tesseract-ocr-eng libtesseract-dev libleptonica-dev

# Get ATLAS library for Kaldi
apt-get install libatlas-dev
