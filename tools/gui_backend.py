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

	def add_service(self, name, acronym, input_type, learn_type, location):
		"""
		return code:
		0: success
		1: name has already exists
		2: acronym has already used
		"""

		num = 0
		instance = []
		collection = self.db.service_info

		# check if current service name is used
		count = collection.count({'name': name})
		if count != 0:
			#collection.delete_many({"name" : sys.argv[2]})
			print('[python error] service name already used.')
			return 1

		# check if current service acronym is used
		count = collection.count({'acronym': acronym})
		if count != 0:
			#collection.delete_many({"name" : sys.argv[2]})
			print('[python error] service acronym already used.')
			return 2

		# list the attributes for the interface
		post = {
			"name": name, # name of service
			"acronym": acronym, # acronym of service
			"num": num, # number of instance
			"instance": instance, # host/port pair of instances
			"input": input_type, # input type
			"learn": learn_type, # learn type
			"location": location # location of service in local
		}

		# insert the service information into MongoDB
		collection.insert_one(post)
		return 0

	def update_service(self, name, op, value):
		"""
		op: field of what you want to update
		value: update value for the field
		return code:
		0: success
		1: service name not found
		"""

		collection = self.db.service_info

		count = collection.count({'name': name})
		if count == 0:
			print('[python error] service not exists in MongoDB.')
			return 1

		collection.update({'name': name}, {'$set': {op: value }})
		return 0

	def delete_service(self, name):
		"""
		return code:
		0: success
		1: service not exist
		"""

		collection = self.db.service_info

		# check if current service is in MongoDB
		count = collection.count({'name': name})
		if count == 0:
			print('[python error] service not exists in MongoDB.')
			return 1

		collection.remove({'name': name})
		return 0

	def add_workflow(self, name, input_type, classifier_path, class_code):
		"""
		return code:
		0: success
		1: workflow name already exists
		"""

		collection = self.db.workflow_info

		# check if current workflow is in MongoDB
		count = collection.count({'name': name})
		if count != 0:
			#collection.delete_many({"name" : sys.argv[2]})
			print('[python error] service already in MongoDB.')
			return 1

		# list the attributes for the interface
		post = {
			"name": name, # name of workflow
			"input": input_type, # allowed input type
			"classifier": classifier_path, # classifier data path
			"code": class_code # code for implementation of the workflow class
		}

		collection.insert_one(post)
		return 0

	def update_service(self, name, op, value):
		"""
		op: field of what you want to update
		value: update value for the field
		return code:
		0: success
		1: workflow name not found
		"""

		collection = self.db.workflow_info

		count = collection.count({'name': name})
		if count == 0:
			print('[python error] service not exists in MongoDB.')
			return 1

		collection.update({'name': name}, {'$set': {op: value }})
		return 0

	def delete_workflow(self, name):
		"""
		return code:
		0: success
		1: workflow not exists
		"""
		
		collection = self.db.workflow_info

		# check if current workflow is in MongoDB
		count = collection.count({'name': name})
		if count == 0:
			print('[python error] workflow not exists in MongoDB.')
			return 1

		collection.remove({'name': name})
		return 0

	def add_instance(self, service_name, name, host, port):
		"""
		return code:
		0: success
		1: host/port not valid
		2: service name not exist
		3: instance name already exists
		4: host/port already used
		"""

		collection = self.db.service_info

		if not validate_ip_port(host, port):
			print('[python error] Host/port pair is not valid.')
			return 1

		# check if current service is in MongoDB
		count = collection.count({'name': service_name})
		if count != 1:
			print('[python error] service not exists in MongoDB.')
			return 2

		# check if name is used
		result = collection.find({'instance.name': name})
		if result.count() != 0:
			print('[python error] Instance name has already been used.')
			return 3

		# check if host and port is used
		result = collection.find({'instance' : { '$elemMatch': { 'host': host, 'port': port}}})
		if result.count() != 0:
			print('[python error] Host/port has already been used.')
			return 4

		collection.update_one({'name': service_name}, {'$inc': {'num': 1}})
		collection.update_one({'name': service_name}, {'$push': {'instance': {
			'name': name,
			'host': host,
			'port': port
		}}})
		return 0

	def update_instance(self, name, op, value):
		"""
		op: field of what you want to update
		value: update value for the field
		return code:
		0: success
		1: instance name not found
		"""

		collection = self.db.service_info

		# check if current service is in MongoDB
		result = collection.find({'instance.name': name})
		if result.count() != 1:
			print('[python error] Instance name not exists.')
			return 1

		service_name = result[0]['name']
		op = 'instance.$.'+op
		collection.update({'instance.name': name}, {'$set': {op: value }})
		return 0

	def delete_instance(self, name):
		"""
		return code:
		0: success
		1: instance name not exist
		"""
		collection = self.db.service_info

		# check if current service is in MongoDB
		result = collection.find({'instance.name': name})
		if result.count() != 1:
			print('[python error] Instance name not exists.')
			return 1

		service_name = result[0]['name']
		collection.update_one({'name': service_name}, {'$inc': {'num': -1}})
		collection.update({}, {'$pull': {'instance': {'name': name }}})
		return 0

	"""
	# import this module and call start_server(name) to start
	def start_server(self, name):
		location, instance = self.search_path(name)

		# start each instance
		for pair in instance:
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

		return result[0]['location'], result[0]['instance']
	"""

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
	if p < 0 or p > 65535:
		return False
	return True

db = MongoDB()