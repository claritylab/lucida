#!/usr/bin/env bash

# start the ASR server
# pass 3 arguments
# jahausw@umich.edu

function print_usage {
  echo "Starts the ASR server"
  echo "    Usage $0 <kaldi|sphinx4|pocketsphinx> <ip> <port>"
  echo "    Default example: $0 kaldi localhost 8081"
}

if [ "$1" == "help" ]; then
  print_usage
  exit
fi

asr=kaldi
ip=localhost
port=8081

if [[ -n "$1" ]]; then
  asr=$1
fi
if [[ -n "$2" ]]; then
  ip=$2
fi
if [[ -n "$3" ]]; then
  port=$3
fi

# start from top directory
cd ../speech-recognition ;

export MODELS_PATH="`pwd`/sphinx/models/"
export CONF_FILE="`pwd`/sphinx/sphinx_batch_conf.xml"
export THREADS=$(cat /proc/cpuinfo | grep processor | wc -l)

if [ "$asr" == "sphinx4" ]; then
  cd ./sphinx/
  java -server -Djava.library.path=./lib \
    -cp .:./lib/servlet.jar:./lib/jetty.jar:./lib/sphinx4.jar Sphinx4Server \
    $ip $port
elif [ "$asr" == "pocketsphinx" ]; then
  cd ./pocketsphinx
  ./start-asr-ps-server.py $ip $port
elif [ "$asr" == "kaldi" ];then
  cd ./kaldi/scripts/
  ./start-asr-dnn-server.py $ip $port
elif [ "$asr" == "pocketsphinx-old" ]; then
# old version included for now
  cd ./sphinx/
  java -server -Djava.library.path=./lib \
    -cp .:./lib/servlet.jar:./lib/jetty.jar:./lib/pocketsphinx.jar \
    PocketsphinxServer $ip $port
fi
