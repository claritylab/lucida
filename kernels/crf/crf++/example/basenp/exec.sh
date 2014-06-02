#!/bin/sh
../../crf_learn -c 10.0 template train.data model
../../crf_test  -m model test.data

../../crf_learn -a MIRA template train.data model
../../crf_test  -m model test.data
rm -f model
