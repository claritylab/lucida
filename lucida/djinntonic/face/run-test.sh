#!/bin/bash
FILES=input/*.jpg
#FILES2=input/cameron-diaz-11/*11-0000??-152*

for f in $FILES
do 
  echo "processing $f"
  python gen-py/LucidaService-remote -h localhost:8086 infer $f 2
done

#for f in $FILES2
#do 
  #echo "processing $f"
  #python gen-py/LucidaService-remote -h localhost:8086 infer $f 2
#done
