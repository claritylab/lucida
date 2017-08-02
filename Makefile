DOCKER_CONTAINER=lucida
VERSION=latest
CUR_DIR=$(shell pwd)

ifeq ($(THREADS),)
  THREADS=$(shell grep -c ^processor /proc/cpuinfo)
endif

include ./Makefile.common

.PHONY: docker local

## build docker environment
docker:
	docker build -t $(DOCKER_CONTAINER):$(VERSION) .

## build local environment
export LD_LIBRARY_PATH=/usr/local/lib

local:
	cd tools && sudo make && cd ../lucida && make

all_service:
	cd lucida && make all

clean_all_service:
	cd lucida && make clean

clean_all_tools:
	cd tools && make clean

dep_core:
	cd tools && sudo make && cd ../lucida/commandcenter && make

start_lucida:
	cd ($CUR_DIR)/lucida/commandcenter && make start_server
