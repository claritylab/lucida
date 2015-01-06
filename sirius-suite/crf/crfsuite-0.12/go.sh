make
make install
cd example/ ;
crfsuite tag -qt -m model.model test.crfsuite.txt
cd - ;
