# NOTE ABOUT CLASSPATHS:
# Classpaths contain jar files and paths to the TOP of package hierarchies.
# For example, say a program imports the class info.ephyra.OpenEphyra
# Now javac knows to look for the class info.ephyra.OpenEphyra in the directory info/ephyra/
# However, javac still needs the classpath to the package.

# Add my classes to class path
# NOTE: these class paths are only necessary when running the program
export JAVA_CLASS_PATH=$JAVA_CLASS_PATH:`pwd`:`pwd`/gen-java

# Add command center to class path
cd $LUCIDAROOT/commandcenter
	export JAVA_CLASS_PATH=$JAVA_CLASS_PATH:`pwd`/gen-java

# Rather than forking a subshell, execute all commands
# in java-config.sh in the current shell.
cd $LUCIDAROOT/questionanswering/common
	source ./qa-runtime-config.inc
# NOTE: this script starts in ../common/question-answer

PORT=$1
CC_PORT=$2

# Use cp flag to avoid cluttering up the CLASSPATH environment variable
java -cp $JAVA_CLASS_PATH -Djava.library.path=lib/search/ -server -Xms1024m -Xmx2048m QADaemon $PORT $CC_PORT
