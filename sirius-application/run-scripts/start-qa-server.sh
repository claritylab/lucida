#!/usr/bin/env bash

# start the QA server

function print_usage {
  echo "Starts the IMM server"
  echo "    Usage $0 <ip> <port>"
  echo "    Default example: $0 localhost 8080"
}

if [ "$1" == "help" ]; then
  print_usage
  exit
fi

hash java 2>/dev/null || {
  echo >&2 "$0: [ERROR] java is not installed. Aborting."
  exit 1
}

ip=localhost
port=8080

if [[ -n "$1" ]]; then
  ip=$1
fi
if [[ -n "$2" ]]; then
  port=$2
fi

# start from top directory
cd ../question-answer ;

export CLASSPATH=bin:lib/ml/maxent.jar:lib/ml/minorthird.jar:lib/nlp/jwnl.jar:lib/nlp/lingpipe.jar:lib/nlp/opennlp-tools.jar:lib/nlp/plingstemmer.jar:lib/nlp/snowball.jar:lib/nlp/stanford-ner.jar:lib/nlp/stanford-parser.jar:lib/nlp/stanford-postagger.jar:lib/qa/javelin.jar:lib/search/bing-search-java-sdk.jar:lib/search/googleapi.jar:lib/search/indri.jar:lib/search/yahoosearch.jar:lib/util/commons-logging.jar:lib/util/gson.jar:lib/util/htmlparser.jar:lib/util/log4j.jar:lib/util/trove.jar:lib/util/servlet-api.jar:lib/util/jetty-all.jar:lib/util/commons-codec-1.9.jar

export INDRI_INDEX=`pwd`/wiki_indri_index/
export THREADS=$(cat /proc/cpuinfo | grep processor | wc -l)
if [ $THREADS -lt 8 ]; then
  export THREADS=8
fi

java -Djava.library.path=lib/search/ -server -Xms1024m -Xmx2048m \
  info.ephyra.OpenEphyraServer $ip $port
