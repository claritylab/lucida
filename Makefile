SUBDIRS=lucida lucida-suite
DOCKER_CONTAINER=claritylab/lucida
VERSION=latest

include ./Makefile.common

.PHONY: docker docker-test

## bulid docker environment
docker:
	docker build -t $(DOCKER_CONTAINER):$(VERSION) .

## run `make test' in docker environment
docker-test: docker
	docker run -it -e UNDER_TEST=1 $(DOCKER_CONTAINER) /usr/bin/make test
