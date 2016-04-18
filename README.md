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

## Lucida Local Development

- From this directory, type: `make local`. This will run scripts in `tools/` to
  install all the required depedencies. Note: if you would like to install the
packages locally, each install script must be modified accordingly. This will
also build `lucida-suite` and `lucida`.
- Similar to what is set in the Makefile, you must set a few environment
  variables. From the top directory:
```
export THRIFT_ROOT=`pwd`/tools/thrift-0.9.2
export CAFFE=`pwd`/tools/caffe/distribute
export LUCIDAROOT=`pwd`/lucida
```
- Start all the services using supervisord:
```
cd tools
supervisord -c lucida.conf
```
- To test, in another terminal navigate to `lucida/commandcenter` and use the
  following commands (replacing (PORT) with 8090 as defined in `lucida.conf`):
```
# Test image matching, speech recognition, and question-answering
./ccclient --asr <AUDIO_FILE> --imm <IMAGE_FILE> (PORT)
# Test speech recognition, and question-answering
./ccclient --asr <AUDIO_FILE> (PORT)
# Test question-answering
./ccclient --qa <QUESTION> (PORT)
```
- The above example uses a small test database for the QA system. To use all of
  Wikipedia:
```
cd tools;
./download_wiki_index.sh # make sure to set INDRI_INDEX as recommended
```
Then restart all the services.

## Lucida Docker Deployment

- Install Docker: refer to
  [https://docs.docker.com/engine/installation/](https://docs.docker.com/engine/installation/)
- Install Docker Compose: use `pip install docker-compose` or refer to
  [https://docs.docker.com/compose/install/](https://docs.docker.com/compose/install/)
- Pull the Lucida image. There are several available:  
`docker pull claritylab/lucida:latest # add your own facts`
- Pull the speech recognition image (based on
  [kaldi-gstreamer-server](https://github.com/alumae/kaldi-gstreamer-server)):  
`docker pull claritylab/lucida-asr`
- From the top directory of Lucida:  
`docker-compose up`
- In Chrome, navigate to `localhost:8081`

Note: Instructions to download and build Sirius can be found at
[http://sirius.clarity-lab.org](http://sirius.clarity-lab.org)
