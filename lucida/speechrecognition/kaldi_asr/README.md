# Kaldi ASR
Kaldi ASR uses [Kaldi](http://kaldi.sourceforge.net/), a speech recognition toolkit
written in C++ that is freely available under the Apache license. 
It uses [Apache Thrift](http://thrift.apache.org/) to communicate with the command center,
in which case it acts as its client,
but it can also run and be tested independently.

This directory contains two sub-directories:

```
kaldi/ 
server/
test/
```

- `kaldi/`: Contains the actual Kaldi package and scripts, with some
  modifications
- `server/`: Contains the main program that listens to requests
- `test/`: Contains a Kaldi testing client

## Build

```
$ make
```

## Run and Test

Start the server:

```
$ cd server
$ ./asr_server (port number of ASR) (port number of command center, optional)
```

Alternatively,
```
$ make start_server (port number of ASR) (port number of command center, optional)
```

Note: There are two modes of usage. 
⋅⋅* If the port number of the command center is not provided,
or the command center cannot be connected to,
the server runs as a stand-alone program.
⋅⋅* Otherwise, the server can interact with the command center
and act as its client.

In either case, the server can interact with a testing client.
To run the testing client:

```
$ cd test
$ ./asr_client (port number of ASR) (path of audio file **relative to test**)
``` 

Alternatively,
```
$ make start_test (port number of ASR) (path of audio file **relative to test**)
```

An example audio file is provided `test.wav`.

## Example Usage as a Summary

```
$ make
$ make start_server 8081
$ # Wait until you see "LOG (online2-wav-nnet2-latgen-faster:ComputeDerivedVars():ivector-extractor.cc:201) Done." in the server terminal.
$ make start_test 8081 test.wav
```
