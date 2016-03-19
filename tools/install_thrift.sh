export THRIFT_VERSION=0.9.2

if [ -z "$THREADS" ]; then
  THREADS=4
fi

if [ -d thrift-$THRIFT_VERSION ]; then
  echo "Thrift already installed, skipping"
  exit
fi

wget "http://archive.apache.org/dist/thrift/$THRIFT_VERSION/thrift-$THRIFT_VERSION.tar.gz" \
  && tar xf thrift-$THRIFT_VERSION.tar.gz \
  && cd thrift-$THRIFT_VERSION \
  && ./configure --with-lua=no --with-ruby=no \
  && make -j $THREADS\
  && sudo make -j $THREADS install \
  && cd lib/py/ \
  && sudo python setup.py install \
  && cd ../../lib/java/ \
  && ant \
  && cd ../../..
