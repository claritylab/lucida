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
THREADS=4

asr_sphinx=speech-recognition/sphinx
asr_kaldi=speech-recognition/kaldi
qa=question-answer
imm=image-matching

export MODELS_PATH="`pwd`/sphinx/models/"

cd $asr_sphinx;
javac -cp .:./lib/servlet.jar:./lib/jetty.jar:lib/sphinx4.jar Sphinx4Server.java
echo "Sphinx4 server done."

javac -cp .:./lib/servlet.jar:./lib/jetty.jar:./lib/pocketsphinx.jar PocketsphinxServer.java
cd ../../ 
echo "Pocketsphinx server done."

cd $asr_kaldi

# Untar the src and tools tarball
if [ ! -d tools ]; then
  tar -xvzf tools.tar.gz
  cd ./tools/
  make -j $THREADS 1>/dev/null
  cd ..
fi

if [ -d src ]; then
  cd ./src/
  make clean
  cd ..
fi

tar -xvzf --overwrite src.tar.gz
cd ./src/
./configure
make -j $THREADS 1>/dev/null

cd ./online2bin
make -j $THREADS 1>/dev/null
cd ../../../../
echo "Kaldi server done."

cd $qa;
ant > /dev/null
cd .. 
echo "OpenEphyra server done."

cd $imm
make -j $THREADS 1>/dev/null
echo "Image-matching server done."
