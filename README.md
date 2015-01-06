## Sirius: Speech and Vision Based Personal Assistant

University of Michigan, 2014  
jahausw@umich.edu  
vpetrucci@gmail.com  

### Build all Systems

./sirius-deps.sh  
./compile-sirius-servers.sh

### QA system (OpenEphyra)

(tested with Java version 1.7)

1) Extract the Wikipedia index in the top directory:  
$ tar xzvf wiki_indri_index.tar.gz

2) Point the index folder to the correct path in SIRIUS_ROOT/openephyra/scripts/OpenEphyraServer.sh

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

1) see README under SIRIUS_ROOT/vision for installation and compilation  

2) Run the detect program as a web service  
$ ./start-imm-server.sh

### Test Scripts

- Testing ASR: ./sirius-asr-test.sh
- Testing QA: ./sirius-qa-test.sh
- Testing IMM: ./sirius-imm-test.sh
- Testing ASR+QA: ./sirius-asr-qa-test.sh

Audio examples can be found under SIRIUS_ROOT/inputs

### Web-App

To run as a web-application, see [sirius-web](sirius-web)

### Sirius-suite

[Sirius-suite](sirius-suite) contains all the kernels extracted from the
end-to-end application. See README for more information.

### Additional info

- bits: folder with old/useful scripts
