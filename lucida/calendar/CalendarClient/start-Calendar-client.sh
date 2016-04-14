# Add Thrift classes to class path
export JAVA_CLASS_PATH=$JAVA_CLASS_PATH:thrift
# Add other jar files to class path
export JAVA_CLASS_PATH=$JAVA_CLASS_PATH:lib/libthrift-0.9.3.jar:lib/slf4j-api-1.7.13.jar:lib/slf4j-simple-1.7.13.jar

# run the server
java -cp $JAVA_CLASS_PATH CalendarClient "$@"
