export PROTOBUF_VERSION=2.5.0

if [ -z "$THREADS" ]; then
  THREADS=4
fi

if [ -d protobuf-$PROTOBUF_VERSION ]; then
  echo "Protobuf already installed, skipping"
  exit
fi

wget "https://github.com/google/protobuf/releases/download/v$PROTOBUF_VERSION/protobuf-$PROTOBUF_VERSION.tar.gz" \
  && tar xf protobuf-$PROTOBUF_VERSION.tar.gz \
  && cd protobuf-$PROTOBUF_VERSION \
  && ./configure \
  && make -j$THREADS \
  && sudo make install
