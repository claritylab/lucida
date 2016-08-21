export THRIFT_VERSION=0.9.3

installCheck () {
  if [ ! -d thrift-$THRIFT_VERSION ]; then
    return 1
  fi
  thrift --gen java check_thrift.thrift
  if [ -d gen-java ]; then
    rm -rf gen-java
    return 0
  else
    return 1
  fi
}

if installCheck $0; then
  echo "Apache Thrift installed";
  exit 0;
fi

sudo apt-get remove -y thrift-compiler

wget "http://archive.apache.org/dist/thrift/$THRIFT_VERSION/thrift-$THRIFT_VERSION.tar.gz" \
  && tar xf thrift-$THRIFT_VERSION.tar.gz \
  && cd thrift-$THRIFT_VERSION \
  && ./configure --with-lua=no --with-ruby=no --with-go=no --with-erlang=no --with-nodejs=no \
  && make -j $THREADS\
  && sudo make -j $THREADS install \
  && cd lib/py/ \
  && sudo python setup.py install \
  && cd ../../lib/java/ \
  && ant \
  && cd ../../..

if installCheck $0; then
  echo "Apache Thrift installed";
  exit 0;
else
  echo "Faile to install Apache Thrift";
  exit 1;
fi
