printdivision()
{
	echo -e "\n"
	for i in `seq 1 70`; do
		echo -n "/";
	done
	echo -e "\n"
}

# Generate thrift files
echo -e "./compile-Calendar-client.sh: `pwd`"
echo -e "./compile-Calendar-client.sh: Compiling thrift source code..."
thrift --gen java ../../lucidaservice.thrift
thrift --gen java ../../lucidatypes.thrift
mv gen-java thrift
printdivision

# Add jar files to class path
export JAVA_CLASS_PATH=$JAVA_CLASS_PATH:lib/libthrift-0.9.3.jar:lib/slf4j-api-1.7.13.jar:lib/slf4j-simple-1.7.13.jar

# Use cp flag to avoid cluttering up the CLASSPATH environment variable
echo -e "javac -cp $JAVA_CLASS_PATH CalendarClient.java thrift/LucidaService.java thrift/QueryInput.java thrift/QuerySpec.java 
\n\n"
javac -cp $JAVA_CLASS_PATH CalendarClient.java thrift/LucidaService.java thrift/QueryInput.java thrift/QuerySpec.java
