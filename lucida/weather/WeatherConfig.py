"""
Weather API configuration details
"""

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
result = collection.find_one({"name" : "weather"})
PORT = int(result["port"])

# Weather Underground API key
# https://www.wunderground.com/weather/api/
WU_API_URL_BASE = 'http://api.wunderground.com/api/'
WU_API_KEY = 'ff76e8e80c802d46' # TODO: add your API key here

# Open Weather Map API key
# https://openweathermap.org/api
OWM_API_URL_BASE = 'http://api.openweathermap.org/data/2.5/weather?'
OWM_API_KEY = '362537b891e6df03a316e82565fe4df3' # TODO: add your API key here
