# Rather than forking a subshell, execute all commands
# in java-config.sh in the current shell.

# Build OpenEphyra
echo -e "./compile-openephyra.sh: `pwd`"
echo -e "./compile-openephyra.sh: Building OpenEphyra..."
cd question-answer
ant
cd ..

# Sample small db provided for testing. 
if [ ! -d "question-answer/small_db.tar.gz" ]; then
	echo "Downloading Small database..."
	tar xzvf small_db.tar.gz -C question-answer/
fi

echo "OpenEphyra done"
