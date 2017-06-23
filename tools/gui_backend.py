#!/usr/bin/env python2

# Standard import
import os
import sys
from pymongo import *

class MongoDB(object):
	def __init__(self):
		mongodb_addr = os.environ.get('MONGO_PORT_27017_TCP_ADDR')
		if mongodb_addr:
			log('MongoDB: ' + mongodb_addr)
			self.db = MongoClient(mongodb_addr, 27017).lucida
		else:
			log('MongoDB: localhost')
			self.db = MongoClient().lucida

	def add_service(self, name, acronym, num, host, port, input_type, learn_type, location):
		collection = self.db.service_info

		# check if current service is in MongoDB
		count = collection.count({'name': name})
		if count != 0:
			#collection.delete_many({"name" : sys.argv[2]})
			print('[python error] service already in MongoDB.')
			exit(1)

		# list the attributes for the interface
		post = {
			"name": name,
			"acronym": acronym,
			"num": num,
			"host": host,
			"port": port,
			"input": input_type,
			"learn": learn_type,
			"location": location
		}

		# insert the service information into MongoDB
		collection.insert_one(post)

	def delete_service(self, name):
		collection = self.db.service_info

		# check if current service is in MongoDB
		count = collection.count({'name': name})
		if count == 0:
			print('[python error] service not exists in MongoDB.')
			exit(1)

		collection.remove({'name': name})

	def add_workflow(self, name, input_type, classifier_path, class_code):
		collection = self.db.workflow_info

		# check if current workflow is in MongoDB
		count = collection.count({'name': name})
		if count != 0:
			#collection.delete_many({"name" : sys.argv[2]})
			print('[python error] service already in MongoDB.')
			exit(1)

		# list the attributes for the interface
		post = {
			"name": name,
			"input": input_type,
			"classifier": classifier_path,
			"code": class_code
		}

		collection.insert_one(post)

	def delete_workflow(self, name):
		collection = self.db.workflow_info

		# check if current service is in MongoDB
		count = collection.count({'name': name})
		if count == 0:
			print('[python error] workflow not exists in MongoDB.')
			exit(1)

		collection.remove({'name': name})

	# import this module and call start_server(name) to start
	def start_server(self, name):
		location, port = search_path(name)
		wrapper_begin = 'gnome-terminal -x bash -c "'
		wrapper_end = '"'
		code = 'cd ' + location + "; "
		code = code + "make start_server port=" + port
		os.system(wrapper_begin + code + wrapper_end)

	def search_path(self, name):
		# get collection for service information
		collection = self.db.service_info

		result = collection.find({'name': name})

		# check if current service is in MongoDB
		count = result.count()
		if count != 1:
			#collection.delete_many({"name" : sys.argv[2]})
			print('[python error] service not in MongoDB.')
			exit(1)

		return result[0]['location'], result[0]['port']

def validate_ip_port(s, p):
	"""
	Check if ip/port is valid with ipv4 
	"""

	a = s.split('.')
	if len(a) != 4:
		return False
	for x in a:
		if not x.isdigit():
			return False
		i = int(x)
		if i < 0 or i > 255:
			return False
	port = int(p)
	if p < 0 or p > 65535:
		return False
	return True

db = MongoDB()