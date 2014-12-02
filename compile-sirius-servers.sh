#!/bin/bash
# compiles all servers

javac -cp .:./lib/servlet.jar:./lib/jetty.jar:lib/sphinx4.jar Sphinx4Server.java
echo "Sphinx4 server done."

javac -cp .:./lib/servlet.jar:./lib/jetty.jar:./lib/pocketsphinx.jar PocketsphinxServer.java
echo "Pocketsphinx server done."

cd openephyra ;
ant > /dev/null
echo "OpenEphyra server done."
