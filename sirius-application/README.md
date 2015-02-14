### Sirius

### Installation

Sirius has been tested on Ubuntu 12.04, 14.04, and Java version 1.7.

### Prerequisites

Sirius has several dependencies which can be resolved with the
`get-<package.sh` scripts. Sirius also depends on:

- [Protobuf](https://code.google.com/p/protobuf) (v2.5)

### [QA system](question-answer)

Extract the Wikipedia index in the top directory:
```bash
$ tar xzvf wiki_indri_index.tar.gz
```
2) Run the QA service:
```bash
$ ./start-qa-server.sh
```
3) You should expect logs like this:  
```bash
...
$ 2014-10-16 19:08:14.240:INFO:oejs.Server:jetty-8.1.14.v20131031
$ 2014-10-16 19:08:14.264:INFO:oejs.AbstractConnector:Started SelectChannelConnector@0.0.0.0:8080
```
4) To test the QA pipeline:
```bash
./sirius-qa-test.sh
```

### ASR system (Sphinx)

1) Run ASR server:
```bash
$ ./start-asr-server.sh
```
2) You should expect logs like this:
```bash
...
$ 2014-10-16 19:26:07.353:INFO:oejs.Server:jetty-8.1.14.v20131031
$ 2014-10-16 19:26:07.379:INFO:oejs.AbstractConnector:Started SelectChannelConnector@0.0.0.0:8081
```
3) To test the ASR service:
```bash
$ ./sirius-asr-test.sh
```

### IMM system (OpenCV)

1) see [README](image-matching) for installation and compilation.

2) Run the detect program as a web service:
```bash
$ ./start-imm-server.sh
```
3) You should expect logs like this:
```bash
...
Starting server on localhost:8082, use <Ctrl-C> to stop
```
4) To test the IMM service:
```bash
$ ./sirius-imm-test.sh
```
### Additional info

- lib: java libs for full application
- speech-recognition/models: acoustic models for speech-recognition
- speech-recognition/sphinx4: alternative to pocketsphinx for speech recognition
    - slower but more accurate (see ./start-asr-server.sh).
