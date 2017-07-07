#!/usr/bin/env python2

import sys
import os
from pymongo import *

# import this module and call start_server(name) to start
def start_server(name):
	location, port = search_path(name)
	wrapper_begin = 'gnome-terminal -x bash -c "'
	wrapper_end = '"'
	code = 'cd ' + location + "; "
	code = code + "make start_server port=" + port
	os.system(wrapper_begin + code + wrapper_end)

def search_path(name):
	# connect to current MongoDB server
	mongodb_addr = os.environ.get('MONGO_PORT_27017_TCP_ADDR')
	if mongodb_addr:
		#print('MongoDB: ' + mongodb_addr)
		db = MongoClient(mongodb_addr, 27017).lucida
	else:
		#print('MongoDb: localhost')
		db = MongoClient('localhost', 27017).lucida

	# get collection for service information
	collection = db.service_info

	result = collection.find({'name': name})

	# check if current service is in MongoDB
	count = result.count()
	if count != 1:
		#collection.delete_many({"name" : sys.argv[2]})
		print('[python error] service not in MongoDB.')
		exit(1)

	return result[0]['location'], result[0]['port']

if __name__ == '__main__':
	start_server(sys.argv[1])
