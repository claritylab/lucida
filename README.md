## Sirius: Speech and Vision Based Personal Assistant

Johann Hauswald (jahausw@umich.edu)
Vinicius Petrucci (vpetrucci@gmail.com)
University of Michigan, 2014
[ASPLOS 2015 Publication](http://jasonmars.org/wp-content/papercite-data/pdf/hauswald15asplos.pdf)

### Build all Systems

`(Tested with Ubuntu 12.04, 14.04)`

1) To install basic depedencies (may not be exhaustive):
```bash
$ ./sirius-deps.sh
```
2) To install Sphinx and pocketsphinx:
```bash
$ ./get-pocketsphinx.sh
```
3) To compile the Sirius server:
```bash
$ ./compile-sirius-servers.sh
```

### QA system (OpenEphyra)

`(tested with Java version 1.7)`

1) Extract the Wikipedia index in the top directory:
```bash
$ tar xzvf wiki_indri_index.tar.gz
```
2) Point the index folder to the correct path in:
`SIRIUS_ROOT/openephyra/scripts/OpenEphyraServer.sh`
3) Run the QA service:
```bash
$ ./start-qa-server.sh
```
4) You should expect logs like this:  
```bash
...
$ 2014-10-16 19:08:14.240:INFO:oejs.Server:jetty-8.1.14.v20131031
$ 2014-10-16 19:08:14.264:INFO:oejs.AbstractConnector:Started SelectChannelConnector@0.0.0.0:8081
```
5) To test the QA pipeline:
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
$ 2014-10-16 19:26:07.379:INFO:oejs.AbstractConnector:Started SelectChannelConnector@0.0.0.0:8080
```
3) To test the ASR service:
```bash
$ ./sirius-asr-test.sh
```

### IMM system (OpenCV)

1) see [README](imm) for installation and compilation.

2) Run the detect program as a web service:
```bash
$ ./start-imm-server.sh
```
3) To test the IMM service:
```bash
$ ./sirius-imm-test.sh
```

### Additional Test Scripts

Test scripts are all included in [run-scripts](run-scripts) folder.
- To test ASR + QA:
```bash
$ ./sirius-asr-qa-test.sh
```

Audio examples can be found under [inputs](inputs).

### Web-App

To run as a web-application, see [sirius-web](sirius-web).

### Sirius-suite

[Sirius-suite](sirius-suite) contains all the kernels extracted from the
end-to-end application. See [README](sirius-suite/README.md) for more information.

### Additional info

- bits: folder with old/useful scripts
- lib: java libs for full application
- models: accoustic models for speech-recognition
- sphinx4: alternative to pocketsphinx for speech reconition
    - slower but more accurate (see ./start-asr-server.sh).
