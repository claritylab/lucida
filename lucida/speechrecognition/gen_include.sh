#!/bin/bash

cd "`dirname $(readlink -f $0)`"
rm -rf include
mkdir -p include
cat defs.yaml | grep -Poe "^[A-Z_]+\s*:\s*\d+$" | while read line; do name=`echo $line | grep -Poe "^[A-Z_]+"`; value=`echo $line | grep -Poe "\d+$"`;
  # Create Python header
  echo "$name = $value" >> include/defs.py
  # Create C header
  echo "#define $name $value" >> include/defs.h
done

thrift -o include --gen c_glib asrthriftservice.thrift
thrift -o include --gen py asrthriftservice.thrift

rm -f include/__init__.py
