#!/usr/bin/env bash

# Install (some) dependencies

if [ "$EUID" -ne 0 ]
then
  echo >&2 "$0: [ERROR] Sudo access required. Aborting."
  exit 1
fi

apt-get install \
  default-jdk ant automake autoconf libtool bison libboost-all-dev ffmpeg

# Get tessaract text recognition
apt-get install \
  tesseract-ocr tesseract-ocr-eng libtesseract-dev libleptonica-dev

# Get ATLAS library for Kaldi
apt-get install \
  libatlas3-base  
