## Sirius: Speech and Vision Based Question-Answering System

University of Michigan, 2014  
jahausw@umich.edu  
vpetrucci@gmail.com  

./compile-sirius-servers.sh

### QA system (OpenEphyra)

(tested with Java version 1.7)

1) Extract the Wikipedia index:  
$ tar xzvf wiki_indri_index.tar.gz

2) Point the index folder to the correct path in <SIRIUS_ROOT>/openephyra/scripts/OpenEphyraServer.sh

3) Run the QA service  
$ ./start-qa-server.sh

You should see this log:  
...  
2014-10-16 19:08:14.240:INFO:oejs.Server:jetty-8.1.14.v20131031  
2014-10-16 19:08:14.264:INFO:oejs.AbstractConnector:Started SelectChannelConnector@0.0.0.0:8081  

### ASR system

1) Run ASR server  
$ ./start-asr-server.sh

You should see this log:  
2014-10-16 19:26:07.353:INFO:oejs.Server:jetty-8.1.14.v20131031  
2014-10-16 19:26:07.379:INFO:oejs.AbstractConnector:Started SelectChannelConnector@0.0.0.0:8080

### IMM system (OpenCV)

1) see README under <SIRIUS_ROOT>/vision/ for installation and compilation  

2) Run the detect program as a web service  
$ ./start-imm-server.sh

### Test Scripts

Audio examples can be found under <SIRIUS_ROOT>/wav

1) Testing ASR: ./sirius-asr-test.sh

2) Testing QA: ./sirius-qa-test.sh

3) Testing IMM: ./sirius-imm-test.sh

4) Testing ASR+QA: ./sirius-asr-qa-test.sh
