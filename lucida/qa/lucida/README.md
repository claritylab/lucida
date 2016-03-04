# Question Answering

The question-answer program in `lucida/` belongs to a collection of services that
communicate with Lucida.

## Basic Setup
If Lucida is not already running, first follow the directions [here](../../command-center/README.md).

1) Compile server

Before compiling:

You need to download [thrift v9.2](http://www.apache.org/dyn/closer.cgi?path=/thrift/0.9.2/thrift-0.9.2.tar.gz) and compile only java stuff via ant ([Instructions available on thrift web site](https://thrift.apache.org/docs/BuildingFromSource)):
`./bootstrap.sh`

`./configure --with-java --without-qt4
  --without-qt5
  --without-c_glib
  --without-csharp
  --without-erlang
  --without-nodejs
  --without-lua
  --without-python
  --without-perl
  --without-php
  --without-php_extension
  --without-dart
  --without-ruby
  --without-haskell
  --without-go
  --without-haxe
  --without-d`

`make`

`sudo make install`

``export thrift=`pwd```

FINALLY, TO COMPILE (from lucida/lucida/qa/lucida directory) : `./compile-qa.sh`

2) Start server: `./start-qa.sh (PORT) (COMMAND CENTER PORT)`

Last Modified: 03/01/16
