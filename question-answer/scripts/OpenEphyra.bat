@echo off
rem - Runs OpenEphyra in command line mode.

rem - The '-server' option of the Java VM improves the runtime of Ephyra.
rem - We recommend using 'java -server' if your VM supports this option.

title OpenEphyra

set CLASSPATH=bin;lib/ml/maxent.jar;lib/ml/minorthird.jar;lib/nlp/jwnl.jar;lib/nlp/lingpipe.jar;lib/nlp/opennlp-tools.jar;lib/nlp/plingstemmer.jar;lib/nlp/snowball.jar;lib/nlp/stanford-ner.jar;lib/nlp/stanford-parser.jar;lib/nlp/stanford-postagger.jar;lib/qa/javelin.jar;lib/search/bing-search-java-sdk.jar;lib/search/googleapi.jar;lib/search/indri.jar;lib/search/yahoosearch.jar;lib/util/commons-logging.jar;lib/util/gson.jar;lib/util/htmlparser.jar;lib/util/log4j.jar;lib/util/trove.jar

cd ..

java -Xms512m -Xmx1024m info.ephyra.OpenEphyra

cd scripts
