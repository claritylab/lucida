## Sirius-suite

Each folder contains standalone algorithmic components extracted from the
end-to-end Sirius system. Every kernel contains a baseline, pthread, GPU
version (CRF and Regex were not ported to the GPU), and input set. The kernels
are either adapted from open-source implementations or where none are
available, written from scratch.

Each kernel is part of a specific service from Sirius:  
    - Automatic Speech Recognition (ASR): dnn-asr, gmm  
    - Image Matching (IMM): fe, fd  
    - Question Answering (QA): crf, regex, stemmer

The `utils/` folder contains timing and printing functions used by the kernels.
The `scripts/` folder has python scripts to run the benchmarks multiple times
and parse the resulting output.

To find out more about the suite, please visit
[sirius-suite](http://sirius.clarity-lab.org/sirius-suite/) webpage. The suite
in the repo does not contain larger input sets and models.
[Download](http://sirius.clarity-lab.org/downloads) the latest version which
includes the inputs.
