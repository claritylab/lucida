# Add Thrift ParserService classes to class path
export JAVA_CLASS_PATH=$JAVA_CLASS_PATH:gen-java
# Add other jar files to class path
export JAVA_CLASS_PATH=$JAVA_CLASS_PATH:lib/commons-lang3-3.4.jar:lib/indri.jar:lib/jsoup-1.8.3.jar:lib/libthrift-0.9.2.jar:lib/slf4j-api-1.7.13.jar:lib/slf4j-simple-1.7.13.jar

# collect the port number
PORT=$1

# run the parser server
java -cp $JAVA_CLASS_PATH -Djava.library.path=lib -server -Xms1024m -Xmx2048m ParserDaemon $PORT
