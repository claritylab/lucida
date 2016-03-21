# Generate thrift files
echo -e "./compile-parser-client-and-server.sh: `pwd`"
echo -e "./compile-parser-client-and-server.sh: Compiling thrift source code..."

# Add jar files to class path
export JAVA_CLASS_PATH=$JAVA_CLASS_PATH:lib/commons-lang3-3.4.jar:lib/indri.jar:lib/jsoup-1.8.3.jar:lib/libthrift-0.9.2.jar:lib/slf4j-api-1.7.13.jar:lib/slf4j-simple-1.7.13.jar

# Use cp flag to avoid cluttering up the CLASSPATH environment variable
echo -e "javac -cp $JAVA_CLASS_PATH ParserDaemon.java ParserServiceHandler.java gen-java/parserstubs/ParserService.java ParserClient.java\n\n"
javac -cp $JAVA_CLASS_PATH ParserDaemon.java ParserServiceHandler.java gen-java/parserstubs/ParserService.java ParserClient.java
