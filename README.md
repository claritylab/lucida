# Lucida

Lucida is a speech and vision based intelligent personal assistant inspired by
[Sirius](http://sirius.clarity-lab.org). Visit the provided readmes in
[lucida](lucida) for instructions to build Lucida and follow the instructions to
build [lucida-suite here](http://sirius.clarity-lab.org/sirius-suite/).  Post to
[Lucida-users](http://groups.google.com/forum/#!forum/sirius-users) for more
information and answers to questions. The project is released under [BSD
license](LICENSE), except certain submodules contain their own specific
licensing information. We would love to have your help on improving Lucida, and
see [CONTRIBUTING](CONTRIBUTING.md) for more details.

## Overview

- `lucida`: back-end services and command center (CMD). 
Currently, there are 4 categories of back-end services:
speech recognition (ASR), image matching (IMM), question answering (QA),
and calendar (CA). By default, Lucida uses the following ports:
3000, 8080 for CMD; 8888 for ASR (web socket listener as part of CMD) ; 
8082 for IMM; 8083 for QA; 8084 for CA.

- `tools`: dependencies necessary for compiling Lucida.

## Lucida Local Development

- From this directory, type: `make local`. This will run scripts in `tools/` to
  install all the required depedencies. After that, it compiles back-end services
  in `lucida/`. Note: if you would like to install the
  packages locally, each install script must be modified accordingly. This will
  also build `lucida-suite` and `lucida`.
- Similar to what is set in the Makefile, you must set a few environment
  variables. From the top directory:
```
export LD_LIBRARY_PATH=/usr/local/lib
export LUCIDAROOT=`pwd`/lucida
```
- Start all the services:
```
make start_all
```
- To test, open your browser, and go to `http://localhost:3000/`.

## Lucida Docker Deployment

- Install Docker: refer to
  [https://docs.docker.com/engine/installation/](https://docs.docker.com/engine/installation/)
- Install Docker Compose: use `pip install docker-compose` or refer to
  [https://docs.docker.com/compose/install/](https://docs.docker.com/compose/install/)
- Pull the Lucida image. There are several available:  
`docker pull claritylab/lucida:latest # add your own facts` (TODO!!!!!)
- Pull the speech recognition image (based on
  [kaldi-gstreamer-server](https://github.com/alumae/kaldi-gstreamer-server)):  
`docker pull claritylab/lucida-asr`
- From the top directory of Lucida:  
`docker-compose up`
- In Chrome, navigate to `localhost:8081`

Note: Instructions to download and build Sirius can be found at
[http://sirius.clarity-lab.org](http://sirius.clarity-lab.org)
