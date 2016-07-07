#!/bin/bash
FILES=/home/xiaoweiw/research/clarity-lab/lucida/lucida/new-djinn/imc/input/*.jpg
# new lucida interface

for f in $FILES
do 
  echo "processing $f"
  python gen-py/LucidaService-remote -h localhost:8085 infer $f 2
  #python gen-py/LucidaService-remote -h localhost:9003 infer $f 2
  #python gen-py/LucidaService-remote -h localhost:9003 infer $f 2
  #python gen-py/LucidaService-remote -h localhost:9003 infer $f 2
done

