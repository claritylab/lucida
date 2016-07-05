## build from lucida base image
FROM lucida_base

#### environment variables
ENV LUCIDAROOT /usr/local/lucida/lucida
ENV LD_LIBRARY_PATH /usr/local/lib

## install CMD
RUN mkdir -p /usr/local/lucida/lucida/commandcenter
ADD . /usr/local/lucida/lucida/commandcenter
WORKDIR "/usr/local/lucida/lucida/commandcenter"
RUN mv lucidaservice.thrift ../ && mv lucidatypes.thrift ../
RUN /usr/bin/make
WORKDIR "/usr/local/lucida/lucida/commandcenter/apache"
RUN /bin/bash install_apache.sh
