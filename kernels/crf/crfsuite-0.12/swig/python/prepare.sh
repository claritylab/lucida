#!/bin/sh

ln -s ../crfsuite.cpp
ln -s ../export.i

if [ "$1" = "--swig" ];
then
    swig -c++ -python -I../../include -o export_wrap.cpp export.i
fi
