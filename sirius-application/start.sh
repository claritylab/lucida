#!/usr/bin/env bash

# Get OpenCV, Kaldi, Comile Servers and TODO: Run Servers
# 2015

cd /opt/sirius/sirius-application

./get-opencv.sh
echo "*** OpenCV Done ***"
./get-kaldi.sh
echo "*** Kaldi Done ***"
./compile-sirius-servers.sh
echo "*** Server Compilation Complete ***"