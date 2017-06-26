#!/usr/bin/env python2

# Standard import
import os
import sys
from pymongo import *

class MongoDB(object):
	def __init__(self):
		mongodb_addr = os.environ.get('MONGO_PORT_27017_TCP_ADDR')
		if mongodb_addr:
			self.db = MongoClient(mongodb_addr, 27017).lucida
		else:
			self.db = MongoClient().lucida

	def add_service(self, name, acronym, num, hostport, input_type, learn_type, location):
		collection = self.db.service_info

		# check if current service is in MongoDB
		count = collection.count({'name': name})
		if count != 0:
			#collection.delete_many({"name" : sys.argv[2]})
			print('[python error] service already in MongoDB.')
			exit(1)

		# list the attributes for the interface
		post = {
			"name": name, # name of service
			"acronym": acronym, # acronym of service
			"num": int(num), # number of instance
			"host_port": hostport, # host/port pair of instances
			"input": input_type, # input type
			"learn": learn_type, # learn type
			"location": location # location of service in local
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
			"name": name, # name of workflow
			"input": input_type, # allowed input type
			"classifier": classifier_path, # classifier data path
			"code": class_code # code for implementation of the workflow class
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

	def add_instance(self, name, host, port):
		collection = self.db.service_info

		if not validate_ip_port(host, port):
			print('[python error] service not exists in MongoDB.')
			exit(1)

		# check if current service is in MongoDB
		count = collection.count({'name': name})
		if count != 1:
			print('[python error] service not exists in MongoDB.')
			exit(1)

		collection.update_one({'name': name}, {'$inc': {'num': 1}})
		collection.update_one({'name': name}, {'$push': {'host_port': {
			'host': host,
			'port': port
		}}})

	def delete_instance(self, name, host, port):
		collection = self.db.service_info

		# check if current service is in MongoDB
		count = collection.count({'name': name})
		if count != 1:
			print('[python error] workflow not exists in MongoDB.')
			exit(1)

		collection.update_one({'name': name}, {'$inc': {'num': -1}})
		collection.update_one({'name': name}, {'$pullAll': {'host_port': [{
			'host': host,
			'port': port
		}]}})

	# import this module and call start_server(name) to start
	def start_server(self, name):
		location, host_port = self.search_path(name)

		# start each instance
		for pair in host_port:
			port = pair['port']
			wrapper_begin = 'gnome-terminal -x bash -c "'
			wrapper_end = '"'
			code = 'cd ' + location + "; "
			code = code + "make start_server port=" + str(port)
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

		return result[0]['location'], result[0]['host_port']

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