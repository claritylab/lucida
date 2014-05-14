#!/bin/sh

make -f ../training/Makefile DIRECTION="-B" CORPUS=train.data MODEL=tmp MULTI_CLASS=2 YAMCHA=../src/yamcha TOOLDIR=../libexec train
echo "Evaluating test.data,  please wait ..."
../src/yamcha -m tmp.model < test.data > tmp.result
perl ../libexec/conlleval.pl < tmp.result



	
