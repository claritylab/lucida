cd ..
python -mthrift_compiler.main --gen cpp2 lucidaservice.thrift
python -mthrift_compiler.main --gen cpp2 lucidatypes.thrift
cd imc
make
cd ../dig
make
cd ../face
make
