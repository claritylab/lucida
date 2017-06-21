#!/bin/bash

python ms_mongo.py delete_all

python wf_mongo.py delete_all

echo "All service deleted successfully!"