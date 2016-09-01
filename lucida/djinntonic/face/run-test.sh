#!/bin/bash
FILES=$(find input/ -type f -name '*.*')

for f in $FILES
do 
  echo "processing $f"
  python ../gen-py/LucidaService-remote -h localhost:8086 infer $f 2
done
