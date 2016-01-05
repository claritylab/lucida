# Add Thrift ParserService classes to class path
export JAVA_CLASS_PATH=$JAVA_CLASS_PATH:gen-java
# Add other jar files to class path
export JAVA_CLASS_PATH=$JAVA_CLASS_PATH:lib/libthrift-0.9.2.jar:lib/slf4j-api-1.7.13.jar:lib/slf4j-simple-1.7.13.jar

# run the parser server
java -cp $JAVA_CLASS_PATH -Djava.library.path=lib -server -Xms1024m -Xmx2048m ParserClient "$@"
