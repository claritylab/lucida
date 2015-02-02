#!/usr/bin/env bash

# start the ASR server
# pass 3 arguments
# jahausw@umich.edu

function print_usage {
  echo "Starts the ASR server"
  echo "    Usage $0 <sphinx4|pocketsphinx> <ip> <port>"
  echo "    Default example: $0 pocketsphinx localhost 8080"
}

if [ "$1" == "help" ]; then
  print_usage
  exit
fi

asr=sphinx4
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

export MODELS_PATH="`pwd`/models/"
export CONF_FILE="`pwd`/sphinx_batch_conf.xml"
export THREADS=8

if [ "$asr" == "sphinx4" ]; then
  java -server -Djava.library.path=./lib \
    -cp .:./lib/servlet.jar:./lib/jetty.jar:./lib/sphinx4.jar Sphinx4Server \
    $ip $port
elif [ "$asr" == "pocketsphinx" ]; then
  java -server -Djava.library.path=./lib \
    -cp .:./lib/servlet.jar:./lib/jetty.jar:./lib/pocketsphinx.jar \
    PocketsphinxServer $ip $port
fi
