#!/bin/bash

python service_mongo.py delete questionanswering
python service_mongo.py delete calendar
python service_mongo.py delete imagematching
python service_mongo.py delete imageclassification
python service_mongo.py delete facerecognition
python service_mongo.py delete digitrecognition
python service_mongo.py delete weather
python service_mongo.py delete musicservice

python workflow_mongo.py delete QAWF
python workflow_mongo.py delete CAWF
python workflow_mongo.py delete IMMWF
python workflow_mongo.py delete IMCWF
python workflow_mongo.py delete FACEWF
python workflow_mongo.py delete DIGWF
python workflow_mongo.py delete WEWF
python workflow_mongo.py delete MSWF

echo "All service deleted successfully!"