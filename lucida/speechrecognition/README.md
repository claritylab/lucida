# Automatic Speech Recognition
ASR uses [Kaldi](http://kaldi.sourceforge.net/) is a speech recognition toolkit
written in C++ that is freely available under the Apache license. ASR uses
[Apache Thrift](http://thrift.apache.org/) to communicate with the command
center.

`asr/` directory contains several files and folder:
```
common/
lucida/
template/
```

- `common/`: Contains the actual Kaldi package and scripts, with some
  modifications
- `lucida/`: Contains a Kaldi micro-service that communicates with the Lucida
  Command Center
- `template/`: Contains a stand-alone Kaldi micro-service for use in any
  application

## Building Kaldi
Some preliminary steps are need before running Kaldi in either `lucida/` folder
or `template/` folder

```
$ cd common/scripts/
$ ./prepare.sh
$ ./compile-kaldi.sh
```

Navigate to `lucida` or `template` to continue the build process after building
Kaldi in `common`.

## Notes
1. Make sure to resolve any extra dependencies that may appear before compiling
   kaldi with `compile-kaldi.sh`
2. Error with Openfst may appear when compiling, to resolve go to the `tools/`
   directory  and type `make` and then try compiling again with
   `compile-kaldi.sh`
