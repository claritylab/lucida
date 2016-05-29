all: thrift

thrift:
	@if [ ! -d "src/main/java/thrift" ]; then \
       thrift --gen java ../lucidaservice.thrift; \
       thrift --gen java ../lucidatypes.thrift; \
       sed -i '1s/^/package thrift;\n/' gen-java/LucidaService.java; \
       sed -i '1s/^/package thrift;\n/' gen-java/QueryInput.java; \
       sed -i '1s/^/package thrift;\n/' gen-java/QuerySpec.java; \
       mkdir src/main/java/thrift/; \
       mv gen-java/* src/main/java/thrift/; \
       rmdir gen-java/; \
    fi

start_server: 
	gnome-terminal -x bash -c "./gradlew run $(filter-out $@,$(MAKECMDGOALS)); read -n1"

start_test: 
	gnome-terminal -x bash -c "java -classpath bin:lib/learn/jsoup-1.8.3.jar:lib/ml/maxent.jar:lib/ml/minorthird.jar:lib/nlp/jwnl.jar:lib/nlp/lingpipe.jar:lib/nlp/opennlp-tools.jar:lib/nlp/plingstemmer.jar:lib/nlp/snowball.jar:lib/nlp/stanford-ner.jar:lib/nlp/stanford-parser.jar:lib/nlp/stanford-postagger.jar:lib/qa/javelin.jar:lib/search/bing-search-java-sdk.jar:lib/search/googleapi.jar:lib/search/indri.jar:lib/search/yahoosearch.jar:lib/thrift/libthrift-0.9.2.jar:lib/thrift/log4j-1.2.14.jar:lib/thrift/slf4j-api-1.5.8.jar:lib/thrift/slf4j-log4j12-1.5.8.jar:lib/util/commons-logging.jar:lib/util/gson.jar:lib/util/htmlparser.jar:lib/util/jetty-all.jar:lib/util/log4j.jar:lib/util/servlet-api.jar:lib/util/trove.jar lucida.test.QAClient $(filter-out $@,$(MAKECMDGOALS)); read -n1"

clean:

.PHONY:	all thrift gradle start_server start_test clean

%:
	@:
