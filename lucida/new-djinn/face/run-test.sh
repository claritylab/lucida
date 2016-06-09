#!/bin/bash
FILES=/home/xiaoweiw/research/localtests/djinn-1.0/tonic-suite/img/input/face/cameron-diaz-11/*
# new lucida interface
#python gen-py/LucidaService-remote -h localhost:9006 infer input/cameron-diaz-11-152.jpg 2
#python gen-py/LucidaService-remote -h localhost:9006 infer input/beyonce-knowles-09.jpg 2
#python gen-py/LucidaService-remote -h localhost:9006 infer input/uma-thurman-79.jpg 2
#python gen-py/LucidaService-remote -h localhost:9006 infer input/zac-efron-83.jpg 2

#for f in $FILES
#do 
  #python gen-py/LucidaService-remote -h localhost:9006 infer $f 2
  #echo "processing $f"
#done

python gen-py/LucidaService-remote -h localhost:9006 infer /home/xiaoweiw/research/localtests/djinn-1.0/tonic-suite/img/input/face/cameron-diaz-11/test_000011-000006-152.jpg 2
python gen-py/LucidaService-remote -h localhost:9006 infer /home/xiaoweiw/research/localtests/djinn-1.0/tonic-suite/img/input/face/cameron-diaz-11/test_000011-000006-152.jpg 2
python gen-py/LucidaService-remote -h localhost:9006 infer /home/xiaoweiw/research/localtests/djinn-1.0/tonic-suite/img/input/face/cameron-diaz-11/test_000011-000006-152.jpg 2
python gen-py/LucidaService-remote -h localhost:9006 infer /home/xiaoweiw/research/localtests/djinn-1.0/tonic-suite/img/input/face/cameron-diaz-11/test_000011-000006-152.jpg 2
