#!/bin/bash

python service_mongo.py add questionanswering QA localhost 8082 text text "$PWD"/../lucida/questionanswering/class.txt
python service_mongo.py add imagematching IMM localhost 8083 image image "$PWD"/../lucida/imagematching/class.txt
python service_mongo.py add calendar CA localhost 8084 text none "$PWD"/../lucida/calendar/class.txt
python service_mongo.py add imageclassification IMC localhost 8085 image none "$PWD"/../lucida/imageclassification/class.txt
python service_mongo.py add facerecognition FACE localhost 8086 image none "$PWD"/../lucida/facerecognition/class.txt
python service_mongo.py add digitrecognition DIG localhost 8087 image none "$PWD"/../lucida/digitrecognition/class.txt
python service_mongo.py add weather WE localhost 8088 text none "$PWD"/../lucida/weather/class.txt
python service_mongo.py add musicservice MS localhost 8089 text none "$PWD"/../lucida/musicservice/class.txt

python workflow_mongo.py add QA text\&text_image code
python workflow_mongo.py add IMM image\&text_image code
python workflow_mongo.py add CA text code
python workflow_mongo.py add IMC image\&text_image code
python workflow_mongo.py add FACE image\&text_image code
python workflow_mongo.py add DIG image\&text_image code
python workflow_mongo.py add WE text code
python workflow_mongo.py add MS text code

echo "All service installed successfully!"
