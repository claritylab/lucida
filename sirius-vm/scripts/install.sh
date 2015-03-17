#!/usr/bin/env bash

# bring the system up to date
sudo apt-get update
sudo apt-get -y upgrade

# install some useful basics
sudo apt-get -y install vim git

# install Sirius (based on the sirius install instructions)
# http://sirius.clarity-lab.org/sirius/

cd /home/sirius/sirius-application

sudo ./get-dependencies.sh 
sudo ./get-kaldi.sh
./get-opencv.sh
sudo ./compile-sirius-servers.sh