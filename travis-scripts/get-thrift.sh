cd
wget http://archive.apache.org/dist/thrift/0.9.2/thrift-0.9.2.tar.gz
tar xzf thrift-0.9.2.tar.gz
cd thrift-0.9.2
./configure
sudo make install -j
cd lib/java;
ant
