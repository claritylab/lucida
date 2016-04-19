# Kaldi ASR
Kaldi ASR uses [Kaldi](http://kaldi.sourceforge.net/), a speech recognition toolkit
written in C++ that is freely available under the Apache license. 
It uses [Apache Thrift](http://thrift.apache.org/) to communicate with the command center,
in which case it acts as its client,
but it can also run and be tested independently.

This directory contains two sub-directories:

```
kaldi/ 
test/
```

- `kaldi/`: Contains the actual Kaldi package and scripts, with some
  modifications
- `test/`: Contains a Kaldi testing client
- Current directory: Contains the main program of the Kaldi server

## Build

```
$ make
```

## Run and Test

Start the server:

```
$ ./start_server.sh (port number of Kaldi) (port number of the command center, optional)
```

Note: There are two modes of usage. 
If the port number of the command center is not provided,
or the command center cannot be connected to,
the server runs as a stand-alone program.
Otherwise, the server can interact with the command center
and act as its client.

In either case, the server can interact with a testing client.
To run the testing client:

```
$ cd test
$ ./client (port number of Kaldi) (path of the audio file)
``` 

An example audio file is provided `how.tall.is.this.wav`.
