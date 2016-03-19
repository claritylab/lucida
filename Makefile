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
local:
	OPENCV_VERSION=2.4.9
	THRIFT_VERSION=0.9.2
	PROTOBUF_VERSION=2.5.0
	JAVA_VERSION=7
	LUCIDAROOT=$(shell pwd)/lucida
	THRIFT_ROOT=$(shell pwd)/tools/thrift-$(THRIFT_VERSION)
	OPENCV_INSTALL=$(shell pwd)/tools/opencv-$(OPENCV_VERSION)/install
	LD_LIBRARY_PATH=/usr/local/lib:$(THRIFT_ROOT):$(OPENCV_INSTALL)
	CAFFE=$(shell pwd)/tools/caffe/distribute
	cd tools && make && cd -
	cd lucida/ && ./thrift-gen.sh && cd -
	make all
