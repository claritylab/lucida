#!/bin/sh
../../crf_learn -p2 -f 3 -c 4.0 template train.data model
../../crf_test -m model test.data

../../crf_learn -a MIRA -f 3 template train.data model
../../crf_test -m model test.data
rm -f model
