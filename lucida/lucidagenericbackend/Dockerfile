# Inherit official Java image, see https://hub.docker.com/_/java/
FROM java:8 

# Update and install dependencies [cmp. https://docs.docker.com/engine/articles/dockerfile_best-practices/]
RUN apt-get update && apt-get install -y \
    gradle \
    libgfortran3

# JAVA_HOME is not set by default
ENV JAVA_HOME /usr/lib/jvm/java-1.8.0-openjdk-amd64/

# Same as "export TERM=dumb"; prevents error "Could not open terminal for stdout: $TERM not set"
ENV TERM dumb

# Copy source code into image
ADD . /ensemble

# Define working directory
WORKDIR /ensemble

# Run YodaQA preparation steps
RUN ./gradlew check

# Expose ports for web interface (9090 REST, 9091 Thrift)
EXPOSE 9090
EXPOSE 9091
