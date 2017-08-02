####
# Builds the lucida base image
FROM ubuntu:14.04

#### environment variables
ENV LUCIDAROOT /usr/local/lucida/lucida

## install lucida
RUN mkdir -p /usr/local/lucida
ADD . /usr/local/lucida
WORKDIR "/usr/local/lucida/"
RUN make dep_core
RUN /bin/bash commandcenter/apache/install_apache.sh
RUN mkdir -p /etc/letsencrypt/live/host

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
