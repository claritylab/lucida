
export MODELS_PATH="/home/vinicius/umvoice/models/"

java -Djava.library.path=./lib -cp .:lib/servlet-api.jar:lib/jetty-all.jar:lib/pocketsphinx-android-0.8-nolib.jar PocketsphinxServer

