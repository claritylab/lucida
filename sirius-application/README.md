## Sirius-application

### Prerequisites

Sirius has several dependencies which can be resolved with the
`get-<package>.sh` scripts.

### Installation

Sirius requires Ubuntu 12.04, 14.04 with Java version 1.7. After resolving
Sirius' dependencies, use `compile-sirius-servers.sh` to compile all the
servers included.

### [ASR systems](speech-recognition)
Included are three ASR systems: Kaldi, Pocketsphinx, and Sphinx4.

### [IMM System](image-matching)
This directory includes the image-matching pipeline, databases, and sample
image queries.

### [QA system](question-answer)
This directory contains OpenEphyra and OpenEphyraServer used by Sirius.

### [Inputs](inputs)
Included are a set of pre-recorded questions that use various components of
Sirius.

### Additional info
Refer to the [sirius installation](http://sirius.clarity-lab.org/sirius) page
for additional information to set up the end-to-end system.
