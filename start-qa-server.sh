#!/bin/bash

ip=$1
port=$2

export CLASSPATH=bin:lib/ml/maxent.jar:lib/ml/minorthird.jar:lib/nlp/jwnl.jar:lib/nlp/lingpipe.jar:lib/nlp/opennlp-tools.jar:lib/nlp/plingstemmer.jar:lib/nlp/snowball.jar:lib/nlp/stanford-ner.jar:lib/nlp/stanford-parser.jar:lib/nlp/stanford-postagger.jar:lib/qa/javelin.jar:lib/search/bing-search-java-sdk.jar:lib/search/googleapi.jar:lib/search/indri.jar:lib/search/yahoosearch.jar:lib/util/commons-logging.jar:lib/util/gson.jar:lib/util/htmlparser.jar:lib/util/log4j.jar:lib/util/trove.jar:lib/util/servlet-api.jar:lib/util/jetty-all.jar:lib/util/commons-codec-1.9.jar

export INDRI_INDEX=`pwd`/wiki_indri_index/
export THREADS=8

cd openephyra/

java -Djava.library.path=lib/search/ -server -Xms1024m -Xmx2048m info.ephyra.OpenEphyraServer $ip $port
