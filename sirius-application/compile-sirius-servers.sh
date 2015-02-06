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

asr_sphinx=speech-recognition/sphinx
asr_kaldi=speech-recognition/kaldi
qa=question-answer
imm=image-matching
export MODELS_PATH="`pwd`/sphinx/models/"

cd $asr_sphinx;
javac -cp .:./lib/servlet.jar:./lib/jetty.jar:lib/sphinx4.jar Sphinx4Server.java
echo "Sphinx4 server done."

javac -cp .:./lib/servlet.jar:./lib/jetty.jar:./lib/pocketsphinx.jar PocketsphinxServer.java
echo "Pocketsphinx server done."
cd ../../ 

cd $asr_kaldi/src;
./configure
make -j 4
echo "Kaldi server done."
cd ../../ 

cd $qa;
ant > /dev/null
cd .. 
echo "OpenEphyra server done."

cd $imm
make 1>/dev/null
echo "Image-matching server done."
cd .. 
