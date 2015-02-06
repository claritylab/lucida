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

asr=speech-recognition
qa=question-answer
imm=image-matching
export MODELS_PATH="`pwd`/models/"

cd $asr ;
javac -cp .:./lib/servlet.jar:./lib/jetty.jar:lib/sphinx4.jar Sphinx4Server.java
echo "Sphinx4 server done."

javac -cp .:./lib/servlet.jar:./lib/jetty.jar:./lib/pocketsphinx.jar PocketsphinxServer.java
cd ..;
echo "Pocketsphinx server done."

cd $qa;
ant > /dev/null
cd ..;
echo "OpenEphyra server done."

cd $imm
make 1>/dev/null
echo "Image-matching server done."
