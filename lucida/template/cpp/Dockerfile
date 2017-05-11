## build from lucida base image
FROM lucida_base

## environment variables
ENV LUCIDAROOT /usr/local/lucida/lucida
ENV LD_LIBRARY_PATH /usr/local/lib

## install template service
RUN mkdir -p /usr/local/lucida/lucida/template/cpp
ADD . /usr/local/lucida/lucida/template/cpp
WORKDIR "/usr/local/lucida/lucida/template/cpp"
RUN mv Makefile.common ../../../ 
RUN mv lucidaservice.thrift ../../ && mv lucidatypes.thrift ../../
RUN /usr/bin/make
