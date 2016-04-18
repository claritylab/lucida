# Rather than forking a subshell, execute all commands
# in the current shell.

# Add thrift libraries to class path
export JAVA_CLASS_PATH=$JAVA_CLASS_PATH:$THRIFT_ROOT/lib/java/build/libthrift-0.9.2.jar:$THRIFT_ROOT/lib/java/build/lib/slf4j-api-1.5.8.jar:$THRIFT_ROOT/lib/java/build/lib/slf4j-log4j12-1.5.8.jar:$THRIFT_ROOT/lib/java/build/lib/log4j-1.2.14.jar

# Add OpenEphyra libraries to class path; start from top directory (question-answer)
cd question-answer

export JAVA_CLASS_PATH=$JAVA_CLASS_PATH:bin:lib/ml/maxent.jar:lib/ml/minorthird.jar:lib/nlp/jwnl.jar:lib/nlp/lingpipe.jar:lib/nlp/opennlp-tools.jar:lib/nlp/plingstemmer.jar:lib/nlp/snowball.jar:lib/nlp/stanford-ner.jar:lib/nlp/stanford-parser.jar:lib/nlp/stanford-postagger.jar:lib/qa/javelin.jar:lib/search/bing-search-java-sdk.jar:lib/search/googleapi.jar:lib/search/indri.jar:lib/search/yahoosearch.jar:lib/util/commons-logging.jar:lib/util/gson.jar:lib/util/htmlparser.jar:lib/util/log4j.jar:lib/util/trove.jar:lib/util/servlet-api.jar:lib/util/jetty-all.jar:lib/util/commons-codec-1.9.jar

if [ -z "$INDRI_INDEX" ]; then
  export INDRI_INDEX=`pwd`/small_db/
fi

echo "Using OpenEphyra database ${INDRI_INDEX}"
