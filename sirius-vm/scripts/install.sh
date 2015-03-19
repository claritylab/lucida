#!/usr/bin/env bash

# Add additional repositories (ffmpeg)
add-apt-repository ppa:jon-severinsson/ffmpeg

# Enable multiverse sources (libfaac-dev)
sed -i "/^# deb.*multiverse/ s/^# //" /etc/apt/sources.list
# possible better option for newer ubuntu releases:
#apt-add-repository multiverse

# Update sources and install basics
apt-get update
apt-get -y install vim git zip unzip dos2unix

# Change to the application directory
cd /home/sirius/sirius-application

# Convert all possible bad line endings (CRLF) to unix line endings (LF)
# Just to take sure... if someone commits (CRLF) endings...
find . -iname '*.sh' -print0 | xargs -0 dos2unix

sudo sh ./get-dependencies.sh 
#sh ./get-kaldi.sh
sudo sh ./get-opencv.sh
#sudo sh ./compile-sirius-servers.sh
