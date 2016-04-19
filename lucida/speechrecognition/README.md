# Automatic Speech Recognition
The current implementation of ASR uses [Kaldi](http://kaldi.sourceforge.net/),
a speech recognition toolkit written in C++ that is freely available under the Apache license. 

## Notes:

1. `Makefile` in the current directory should be called by the topmost `Makefile`,
i.e. `../../Makefile`.
If you want to build the Kaldi ASR service manually,
please navigate to `kaldi_asr` and follow the `README.md` there.

2. If you want to use another ASR implementation,
you can start by creating a separate folder parallel to `kaldi_asr`.
