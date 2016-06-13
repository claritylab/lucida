git clone https://github.com/facebook/folly.git
cd folly/folly
autoreconf -ivf
./configure
make
make check
sudo make install
cd ../
rm -rf .git
cd ../
git clone https://github.com/facebook/wangle.git
cd wangle/wangle
cmake .
make
ctest
sudo make install

git clone https://github.com:facebook/fbthrift.git
autoreconf -if
./configure
sudo make
sudo make install
cd ..
rm -rf .git
cd ..
