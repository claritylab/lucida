#!/usr/bin/env bash
#WMAT01 - 10/03/2015

# Start... run as sudo
echo "Starting WMAT01 installation procedure..." && \
wait 5 && \

# Get dependencies
./get-depencies.sh && \

# Get Kaldi
./get-kaldi.sh && \

# Get OpenCV 2.4.9
./get-opencv.sh && \

# Final Compile
echo " " && \
echo " " && \
echo "Preparation for final compile completed..." && \
echo " " && \
echo " " && \
./compile-sirius-servers.sh && \
echo "SIRIUS SETUP ALL DONE!!  ~WMAT01"

#Prepare QA System:
echo "Preparing QA System..."
wget http://web.eecs.umich.edu/~jahausw/download/wiki_indri_index.tar.gz
tar xzvf wiki_indri_index.tar.gz -C question-answer/


