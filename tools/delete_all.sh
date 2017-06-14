#!/bin/bash

python service_mongo.py delete questionanswering
python service_mongo.py delete calendar
python service_mongo.py delete imagematching
python service_mongo.py delete imageclassification
python service_mongo.py delete facerecognition
python service_mongo.py delete digitrecognition
python service_mongo.py delete weather
python service_mongo.py delete musicservice

python workflow_mongo.py delete QA
python workflow_mongo.py delete CA
python workflow_mongo.py delete IMM
python workflow_mongo.py delete IMC
python workflow_mongo.py delete FACE
python workflow_mongo.py delete DIG
python workflow_mongo.py delete WE
python workflow_mongo.py delete MS

echo "All service deleted successfully!"