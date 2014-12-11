
========================
= Sirius Benchmark 1.0 =
========================
Univ of Michigan, 2014

./compile-sirius-servers.sh
./start-<type>-server.sh

QA system (OpenEphyra)
++++++++++++++++++++++

(tested with Java version 1.7)

1) Extract the Wikipedia index:
$ tar xzvf wiki_indri_index.tar.gz

2) Point the index folder to the correct path on <SIRIUS_ROOT>/openephyra/scripts/OpenEphyraServer.sh
export INDRI_INDEX=<YOUR_PATH>/indri_index

3) Run the QA service
$ ./start-qa-server.sh

You should see this log:
...
2014-10-16 19:08:14.240:INFO:oejs.Server:jetty-8.1.14.v20131031
2014-10-16 19:08:14.264:INFO:oejs.AbstractConnector:Started SelectChannelConnector@0.0.0.0:8081


ASR system
++++++++++++++++++++++++

1) Run ASR server
$ ./start-asr-server.sh

You should see this log:
2014-10-16 19:26:07.353:INFO:oejs.Server:jetty-8.1.14.v20131031
2014-10-16 19:26:07.379:INFO:oejs.AbstractConnector:Started SelectChannelConnector@0.0.0.0:8080


IMM system (OpenCV)
+++++++++++++++++++

1) see README under <SIRIUS_ROOT>/vision/ for installation and compilation

2) Run the detect program as a web service
./detect --match_service --database <SIRIUS_ROOT>/vision/matching/buildings/db_small


Test Scripts (localhost)
++++++++++++++++++++++++

Audio examples can be found under <SIRIUS_ROOT>/wav

1) Testing ASR:
./sirius-asr-test.sh

2) Testing QA:
./sirius-qa-test.sh

3) Testing IMM:
./sirius-imm-test.sh

4) Testing ASR+QA
./sirius-asr-qa-test.sh

Client script 
+++++++++++++

This script records audio and send queries to the server.

Packages required (Tested on MAC OS X 10.9.5 via MacPorts; e.g., sudo port install sox ffmpeg)
sox
ffmpeg

1) Edit the "sirius-client.sh" changing ASR_SERVER and QA_SERVER using the correct hostnames

2) Run the client script:
$ ./sirius-client.sh



