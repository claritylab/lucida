# original thrift interface
#gen-py/IMC-remote imageClassification 1

# new lucida interface
#python gen-py/LucidaService-remote -h localhost:9003 infer input/tp-999-227.jpg 2
#python gen-py/LucidaService-remote -h localhost:9003 infer input/cat-281-227.jpg 2
python gen-py/LucidaService-remote -h localhost:9003 learn input/cat-281-227.jpg 2
