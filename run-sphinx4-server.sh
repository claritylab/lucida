#!/bin/bash

export MODELS_PATH="/home/$USER/umvoice/models/"
export CONF_FILE="/home/$USER/umvoice/sphinx_batch_conf.xml"

#java -Djava.library.path=./lib -cp .:./lib/servlet-api.jar:./lib/jetty-all.jar:lib/sphinx4-core-1.0-SNAPSHOT.jar Sphinx4Server

java -Djava.library.path=./lib -cp .:./lib/servlet-api.jar:./lib/jetty-all.jar:sphinx4/sphinx4-core/target/sphinx4-core-1.0-SNAPSHOT.jar Sphinx4Server



