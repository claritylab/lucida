cd tools
./install_openfst.sh
./atlas_header.sh
cd ../src
./configure
make depend -j $THREADS
make -j $THREADS
cd ..
python -mthrift_compiler.main --gen cpp2 ../lucidaservice.thrift 
python -mthrift_compiler.main --gen cpp2 ../lucidatypes.thrift 
make all
