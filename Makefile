SUBDIRS=lucida-suite lucida

DOCKER_CONTAINER=claritylab/lucida
VERSION=latest

THREADS=4

include ./Makefile.common

.PHONY: docker local

## build docker environment
docker:
	docker build -t $(DOCKER_CONTAINER):$(VERSION) .

## build local environment
THRIFT_VERSION=0.9.2
export THRIFT_ROOT=$(shell pwd)/tools/thrift-$(THRIFT_VERSION)
export CAFFE=$(shell pwd)/tools/caffe/distribute
export LUCIDAROOT=$(shell pwd)/lucida
local:
	cd tools && make && cd - && cd lucida && ./thrift-gen.sh && cd - && make all

## start all services
start_all:
	cd lucida/commandcenter && make start_server && sleep 5 && cd ../speechrecognition/kaldi_gstreamer_asr && make start_server && cd ../../imagematching/opencv_imm && make start_server && cd ../../questionanswering/OpenEphyra && make start_server
