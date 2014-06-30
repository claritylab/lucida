#!/bin/bash

export MODELS_PATH="/home/kmcatee/umvoice/models/"
export CONF_FILE="/home/kmcatee/umvoice/sphinx_batch_conf.xml"

#java -Djava.library.path=./lib -cp .:./lib/servlet-api.jar:./lib/jetty-all.jar:lib/sphinx4-core-1.0-SNAPSHOT.jar Sphinx4Server

java -Djava.library.path=./lib -cp .:./lib/servlet-api.jar:./lib/jetty-all.jar:./lib/sphinx4-core-1.0-SNAPSHOT.jar Sphinx4Server




