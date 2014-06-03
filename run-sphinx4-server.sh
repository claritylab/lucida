#!/bin/bash

<<<<<<< HEAD
export MODELS_PATH="/Users/vinicius/umvoice/models/"
export CONF_FILE="/Users/vinicius/umvoice/sphinx_batch_conf.xml"
=======
export MODELS_PATH="/home/$USER/umvoice/models/"
export CONF_FILE="/home/$USER/umvoice/sphinx_batch_conf.xml"
>>>>>>> 9054a32ac8e057c46f7da51a91d057724fad3fe8

#java -Djava.library.path=./lib -cp .:./lib/servlet-api.jar:./lib/jetty-all.jar:lib/sphinx4-core-1.0-SNAPSHOT.jar Sphinx4Server

java -Djava.library.path=./lib -cp .:./lib/servlet-api.jar:./lib/jetty-all.jar:sphinx4/sphinx4-core/target/sphinx4-core-1.0-SNAPSHOT.jar Sphinx4Server



