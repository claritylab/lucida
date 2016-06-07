# Automatic Speech Recognition (ASR)

The current implementation of ASR uses [Kaldi](http://kaldi.sourceforge.net/),
a speech recognition toolkit written in C++ that is freely available under the Apache license. 

## Notes:

1. `kaldi_gstreamer_asr` contains the implementation of the Kaldi ASR service.

2. If you want to create and use another ASR implementation,
you can start by making a directory parallel to `kaldi_gstreamer_asr` and modify `Makefile`.
Make sure to reference `../lucidaservice.thrift` and `../lucidatypes.thrift`.

3. Type `make` to build all ASR implementations,
or type `cd kaldi_gstreamer_asr` and `make` to only build the Kaldi ASR service.

