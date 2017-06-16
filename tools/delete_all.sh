#!/bin/bash

python ms_mongo.py delete questionanswering
python ms_mongo.py delete calendar
python ms_mongo.py delete imagematching
python ms_mongo.py delete imageclassification
python ms_mongo.py delete facerecognition
python ms_mongo.py delete digitrecognition
python ms_mongo.py delete weather
python ms_mongo.py delete musicservice

python wf_mongo.py delete QAWF
python wf_mongo.py delete CAWF
python wf_mongo.py delete IMMWF
python wf_mongo.py delete IMCWF
python wf_mongo.py delete FACEWF
python wf_mongo.py delete DIGWF
python wf_mongo.py delete WEWF
python wf_mongo.py delete MSWF

echo "All service deleted successfully!"