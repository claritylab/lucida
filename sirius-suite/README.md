## Sirius-Suite

Each folder contains standalone algorithmic components extracted from the
end-to-end Sirius system. Every kernel contains a baseline, pthread, GPU
version (CRF and Regex were not ported to the GPU), input set, and a more
detailed README. The kernels are either adapted from open-source
implementations or where none are available, written from scratch.

Each kernel is part of a specific service from Sirius:
    -ASR: dnn-asr, gmm
    -IMM: fe, fd
    -QA: crf, regex, stemmer

### Running the kernels:
- Most dependencies can be installed using `get-<dependency>.sh` included in
[sirius-application](../sirius-application). `dnn-asr` and `crf` require
additional libraries (see READMEs).
- Build all:  
```bash
$ make
```
- Run the kernels on all supported platforms with the default inputs and
check output compared to the baseline implementation:
```bash
$ make test
```
- The kernels produce .json output that is easily parsable. Kernel
information, input data sizes, and timing information is printed during
execution of the kernel. The `scripts/` folder includes useful python scripts
to loop and parse the kernel output.

Download larger inputs at: TODO

http://web.eecs.umich.edu/~jahausw

http://sirius.clarity-lab.org
