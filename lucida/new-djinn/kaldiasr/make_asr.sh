cd tools
./install_openfst.sh
cd ../src
./configure
make depend -j $THREADS
make -j $THREADS
cd ..
make all
