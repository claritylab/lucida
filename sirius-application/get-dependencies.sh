#!/usr/bin/env bash

# Install (some) dependencies

sudo apt-get install \
  default-jdk ant automake autoconf libtool bison libboost-all-dev ffmpeg

# Get tessaract text recognition
sudo apt-get install \
  tesseract-ocr tesseract-ocr-eng libtesseract-dev libleptonica-dev

# Get ATLAS library for Kaldi
sudo apt-get install \
  libatlas-dev
