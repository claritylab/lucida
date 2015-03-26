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

### Deployment with Docker
Dockerfiles have been created to provide an alternate deployment model.
- Based on Ubuntu 12.04
- 2 stage docker process

## Docker Stage 1 "BUILD" - Base Image Build
After cloning the sirius github repository, run the following command:

```
// From your /sirius directory (cloned instance of github repo)
 # docker build -t siriusbase:latest .
```
- This step will take 10-15 minutes to complete
- Image size approx 1GB

At this point, the 'siriusbase:latest' image can be pushed to your preferred docker registry for future re-use, if required.

## Docker Stage 2 "RUN" - Expose ports --> Link sirius source --> Compile binaries --> Run (manual process)
```
 # docker run -it \
 	-e JAVA_HOME=/usr/lib/jvm/java-7-openjdk-amd64/jre \
 	-e JAVA_TOOL_OPTIONS=-Dfile.encoding=UTF8 \
 	-v /foo/bar/sirius/:/opt/sirius \
 	-p 8080:8080 \
 	-p 8081:8081 \
 	-p 8082:8082 \
 	siriusbase:latest bash

 // then, from within the container
 # cd /opt/sirius/sirius-application
 # ./start.sh

 // TODO: e2e start.sh automated process, including starting servers

```
- In AWS, this step takes ~30 minutes to complete
- On completion of start.sh, follow the normal steps to invoke the required sirius server components from /sirius/sirius/sirius-application/run-scripts/


## Docker TODO
```
 // TODO: NOTE: Only Q&A Server tested successfully with current build (26/3/15)
 // TODO: dockerfile for sirius-web
 // TODO: e2e automated deployment
```
