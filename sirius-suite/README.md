## Sirius-Suite

Each folder contains standalone algorithmic components extracted from the
end-to-end Sirius system. Every kernel contains a baseline, pthread, GPU
version (CRF and Regex were not ported to the GPU), input set, and a more
detailed README. The kernels are either adapted from open-source
implementations or where none are available, written from scratch.

Each kernel is part of a specific service from Sirius:  
    - Automatic Speech Recognition (ASR): dnn-asr, gmm  
    - Image Matching (IMM): fe, fd  
    - Question Answering (QA): crf, regex, stemmer

### Running the kernels:
- Most dependencies can be installed using `get-<dependency>.sh` included in
[sirius-application](../sirius-application). `dnn-asr` and `crf` require
additional libraries (see READMEs).
- Build all:  
```bash
$ make
```
- Or Run the kernels on all supported platforms with the default inputs and
check output compared to the baseline implementation:
```bash
$ make test
```

Download models and larger inputs at: http://sirius.clarity-lab.org/downloads
