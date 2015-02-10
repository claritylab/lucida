## [Sirius-suite ASR]: Gaussian Mixture Model

This is a Gaussian Mixture Model (GMM) kernel commonly used in ASR
applications to score Hidden Markov Model (HMM) state transitions.

### Backend
This kernel is extracted from
[PocketSphinx](http://cmusphinx.sourceforge.net), the embedded C version of
Sphinx. This kernel

### Directory structure
Download the input data at TODO. This file includes the accoustic model and 1
set of HMM state transitions to score by the GMM.

### Running the kernel
1. Build all:  
```bash
$ make
```
2. Run the kernel on all platforms with the default inputs:
```bash
$ make test
```
or
```bash
$ ./gmm_scoring ../input/gmm_data.txt
```
The kernel does the following:
  1. Reads the accoustic model features,
  2. Reads the states that need to be scored,
  3. Scores each HMM state transition.
