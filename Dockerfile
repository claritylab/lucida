####
# Builds the lucida base image
FROM ubuntu:14.04

#### environment variables
ENV LUCIDAROOT /usr/local/lucida/lucida
ENV THRIFT_ROOT /usr/local/lucida/tools/thrift-$THRIFT_VERSION
ENV LD_LIBRARY_PATH /usr/local/lib
ENV CAFFE /usr/local/lucida/tools/caffe/distribute
ENV CPU_ONLY 1 # for caffe

ENV OPENCV_VERSION 2.4.9
ENV THRIFT_VERSION 0.9.3
ENV THREADS 4
ENV PROTOBUF_VERSION 2.5.0
ENV JAVA_VERSION 8
ENV JAVA_TOOL_OPTIONS=-Dfile.encoding=UTF8 

## install lucida
RUN mkdir -p /usr/local/lucida
ADD . /usr/local/lucida
WORKDIR "/usr/local/lucida/tools"
RUN /usr/bin/make
WORKDIR "/usr/local/lucida/lucida"
RUN /usr/bin/make
RUN /bin/bash commandcenter/apache/install_apache.sh

### function docker-flush(){
###     dockerlist=$(docker ps -a -q)
###     if [ "${dockerlist}" != "" ]; then
###         for d in ${dockerlist}; do
###             echo "***** ${d}"
###             docker stop ${d} 2>&1 > /dev/null
###             docker rm ${d} 2>&1 > /dev/null
###         done
###     fi
### }