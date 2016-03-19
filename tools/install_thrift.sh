export THRIFT_VERSION=0.9.2

if [ -z "$THREADS" ]; then
  THREADS=4
fi

wget "http://archive.apache.org/dist/thrift/$THRIFT_VERSION/thrift-$THRIFT_VERSION.tar.gz" \
  && tar xf thrift-$THRIFT_VERSION.tar.gz \
  && cd thrift-$THRIFT_VERSION \
  && ./configure \
  && make -j $THREADS\
  && make -j $THREADS install \
  && cd lib/py/ \
  && python setup.py install \
  && cd ../../lib/java/ \
  && ant \
  && cd ../../..
