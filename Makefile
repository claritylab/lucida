DOCKER_CONTAINER=lucida
VERSION=latest

THREADS=4

include ./Makefile.common

.PHONY: docker local

## build docker environment
docker:
	docker build -t $(DOCKER_CONTAINER):$(VERSION) .

## build local environment
export LD_LIBRARY_PATH=/usr/local/lib

local: 
	cd tools && make && cd ../lucida && make

start_all:
	cd tools && ./start_all.sh
