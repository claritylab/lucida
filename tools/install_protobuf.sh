export PROTOBUF_VERSION=2.5.0

if [ -z "$THREADS" ]; then
  THREADS=4
fi

wget "https://github.com/google/protobuf/releases/download/v$PROTOBUF_VERSION/protobuf-$PROTOBUF_VERSION.tar.gz" \
  && tar xf protobuf-$PROTOBUF_VERSION.tar.gz \
  && cd protobuf-$PROTOBUF_VERSION \
  && ./configure --prefix=`pwd`/install\
  && make -j$THREADS \
  && make install
