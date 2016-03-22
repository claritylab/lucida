# NOTE ABOUT CLASSPATHS:
# Classpaths contain jar files and paths to the TOP of package hierarchies.
# For example, say a program imports the class info.ephyra.OpenEphyra
# Now javac knows to look for the class info.ephyra.OpenEphyra in the directory info/ephyra/
# However, javac still needs the classpath to the package.

# Compile server
# 'source' command: Rather than forking a subshell, execute all commands
# in the current shell.
cd ../common
./compile-openephyra.sh
. ./qa-compile-config.inc
cd -

# Add command center to class path
export JAVA_CLASS_PATH=$JAVA_CLASS_PATH:$LUCIDAROOT/commandcenter/gen-java

# Use cp flag to avoid cluttering up the CLASSPATH environment variable
echo -e "javac -cp $JAVA_CLASS_PATH QADaemon.java QAServiceHandler.java gen-java/qastubs/QAService.java\n\n"
javac -cp $JAVA_CLASS_PATH QADaemon.java QAServiceHandler.java gen-java/qastubs/QAService.java
