echo -e "./qa-compile-config.sh: `pwd`"
echo -n -e "./qa-compile-config.sh: setting compile-time class path for question-answer... "

# Add thrift libraries to class path
export JAVA_CLASS_PATH=$THRIFT_ROOT/lib/java/build/libthrift-0.9.2.jar:$THRIFT_ROOT/lib/java/build/lib/slf4j-api-1.5.8.jar:$THRIFT_ROOT/lib/java/build/lib/slf4j-log4j12-1.5.8.jar:$THRIFT_ROOT/lib/java/build/lib/log4j-1.2.14.jar

# Add OpenEphyra libraries to class path
export JAVA_CLASS_PATH=$JAVA_CLASS_PATH:$LUCIDAROOT/questionanswering/common/question-answer/bin

echo $JAVA_CLASS_PATH

echo -e "Done"
