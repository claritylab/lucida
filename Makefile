DOCKER_CONTAINER=lucida
VERSION=latest

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

start_all:
	cd tools && ./start_all_tmux.sh

start_all_secure:
	cd tools && ./start_all_tmux.sh secure

start_test_all:
	cd tools && ./start_all_tmux.sh test

all_service:
	cd lucida && make all

clean_all_service:
	cd lucida && make clean

clean_all_tools:
	cd tools && make clean
