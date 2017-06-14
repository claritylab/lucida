"""
This a guide for how to add your own microservice into Lucida interface
"""

# Server Port number (necessary for every microservice server)
import ConfigParser, sys, os
from pymongo import *

mongodb_addr = os.environ.get('MONGO_PORT_27017_TCP_ADDR')
if mongodb_addr:
    print('MongoDB: ' + mongodb_addr)
    db = MongoClient(mongodb_addr, 27017).lucida
else:
    print('MongoDb: localhost')
    db = MongoClient().lucida

collection = db.service_info
result = collection.find_one({"name" : "template"})
PORT = int(result["port"])

# TODO: Other configuration for your own service
