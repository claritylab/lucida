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

The `utils/` folder contains timing and printing functions used by the kernels.  
The `scripts/` folder has python scripts to run the benchmarks multiple times and parse the resulting output.

### Running the kernels:
- Most dependencies can be installed using `get-<dependency>.sh` included in
[sirius-application](../sirius-application). Additional information can be found on the [Sirius-suite](http://sirius.clarity-lab.org/sirius-suite/) webpage.
- Build all:  
```bash
$ make
```
- Or build and run the kernels on all supported platforms with the default configurations and inputs, and
validate the output w.r.t the baseline implementation:
```bash
$ make clean
$ make test
```

Download models and larger inputs at: http://sirius.clarity-lab.org/downloads
