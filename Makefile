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
