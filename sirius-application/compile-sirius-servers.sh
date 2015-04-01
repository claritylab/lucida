#!/usr/bin/env bash
# exit if any server fails
set -e

# compiles all servers

hash javac 2>/dev/null || {
  echo >&2 "$0: [ERROR] javac is not installed. Aborting."
  exit 1
}

hash ant 2>/dev/null || {
  echo >&2 "$0: [ERROR] ant is not installed. Aborting."
  exit 1
}

# thread for parallel build
[[ -n "$THREADS" ]] || THREADS=4

asr_ps=speech-recognition/pocketsphinx
asr_sphinx=speech-recognition/sphinx
asr_kaldi=speech-recognition/kaldi
qa=question-answer
imm=image-matching

export MODELS_PATH="`pwd`/sphinx/models/"

################
# Sphinx
################
cd $asr_sphinx;
javac -cp .:./lib/servlet.jar:./lib/jetty.jar:lib/sphinx4.jar Sphinx4Server.java
echo "Sphinx4 server done."

##################
# Java PocketSphinx
##################
javac -cp .:./lib/servlet.jar:./lib/jetty.jar:./lib/pocketsphinx.jar PocketsphinxServer.java
cd - > /dev/null
echo "Java Pocketsphinx server done."

##################
# C++ PocketSphinx
##################

cd $asr_ps
./compile.sh
cd - > /dev/null
echo "C++ Pocketsphinx server done."

################
# Kaldi
################
cd $asr_kaldi
# Untar the src and tools tarball
if [ ! -d tools ]; then
  tar -xzf tools.tar.gz
  cd ./tools/
  make -j $THREADS ;
  cd ..
fi

tar -xzf src.tar.gz --overwrite
cd ./src/
./configure ;
make -j $THREADS ;

cd ./online2bin
make -j $THREADS ;
cd ../../../../
echo "Kaldi server done."

################
# OpenEphyra
################
if [ ! -d question-answer ]; then
  tar -xzf question-answer.tar.gz
fi
cd $qa;
ant ;#> /dev/null
cd - > /dev/null
echo "OpenEphyra server done."

################
# Image Matching
################
cd $imm
protoc --cpp_out=. store_mat.proto keypoints.proto
make -j$THREADS ;
echo "Image-matching server done."
