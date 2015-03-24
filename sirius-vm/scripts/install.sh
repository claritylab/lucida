#!/usr/bin/env bash

# install some helper for working inside the vm
sudo apt-get update
sudo apt-get -y install \
     vim dos2unix

# Change to the application directory
cd /home/sirius/sirius-application

# Convert all possible bad line endings (CRLF) to unix line endings (LF)
# Just to take sure... if someone commits (CRLF) endings...
find . -iname '*.sh' -print0 | xargs -0 dos2unix

sudo sh ./get-dependencies.sh 
./get-kaldi.sh
./get-opencv.sh
THREADS=1 ./compile-sirius-servers.sh
