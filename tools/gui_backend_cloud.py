import os
import sys
import requests
import json
from pymongo import *
from bson.objectid import ObjectId

class MongoDB(object):
	def __init__(self):
		mongodb_addr = os.environ.get('MONGO_PORT_27017_TCP_ADDR')
		if mongodb_addr:
			self.db = MongoClient(mongodb_addr, 27017).lucida
		else:
			self.db = MongoClient().lucida

	def get_services(self):
		dictReturn = []

		url = 'http://127.0.0.1:3000/api/v1/service'
		r = requests.get(url)
		ret_JSON = r.json()
		dictReturn = ret_JSON['service_list']
		
		return dictReturn

	def add_service(self, name, acronym, input_type, learn_type):
		"""
		return code:
		0: success
		1: name has already exists
		2: acronym has already used
		"""

		num = 0
		instance = []

		# list the attributes for the interface
		post = {
			"option": "add",
			"name": name, # name of service
			"acronym": acronym, # acronym of service
			"num": num, # number of instance
			"instance": instance, # host/port pair of instances
			"input": input_type, # input type
			"learn": learn_type # learn type
			# "location": location # location of service in local
		}

		url = 'http://127.0.0.1:3000/api/v1/service'
		headers = {'Content-type':'application/json', 'Accept': 'text/plain'}
		r = requests.post(url, data=json.dumps(post), headers=headers)
		ret_JSON = r.json()
		ret_status = r.status_code
		if ret_status == 200:
			return 0, ret_JSON['_id']
		else:
			error = ret_JSON['error']
			if error == 'Service name has already existed':
				return 1, ''
			elif error == 'Service acronym has already used':
				return 2, ''
			else:
				return -1, ''

	def update_service(self, _id, op, value):
		"""
		op: field of what you want to update
		value: update value for the field
		return code:
		0: success
		1: service name not found
		"""

		post = {
			"option": "update",
			"_id": _id,
			"op": op,
			"value": value
		}
		
		url = 'http://127.0.0.1:3000/api/v1/service'
		headers = {'Content-type':'application/json', 'Accept': 'text/plain'}
		r = requests.post(url, data=json.dumps(post), headers=headers)
		ret_JSON = r.json()
		ret_status = r.status_code
		if ret_status == 200:
			return 0
		else:
			error = ret_JSON['error']
			if error == 'Service not exists':
				return 1
			elif error == 'Updated name already used':
				return 2
			elif error == 'Updated acronym already used':
				return 3
			else:
				return -1

	def delete_service(self, _id):
		"""
		return code:
		0: success
		1: service not exist
		"""

		post = {
			"option": "delete",
			"_id": _id
		}
		
		url = 'http://127.0.0.1:3000/api/v1/service'
		headers = {'Content-type':'application/json', 'Accept': 'text/plain'}
		r = requests.post(url, data=json.dumps(post), headers=headers)
		ret_JSON = r.json()
		ret_status = r.status_code
		if ret_status == 200:
			return 0
		else:
			error = ret_JSON['error']
			if error == 'Service not exists':
				return 1
			else:
				return -1

	def add_workflow(self, name, input_type, classifier_path, class_code):
		"""
		return code:
		0: success
		1: workflow name already exists
		"""

		# list the attributes for the interface
		post = {
			"option": "add",
			"name": name, # name of service
			"input": input_type, # acronym of service
			"classifier": classifier_path, # number of instance
			"code": class_code # host/port pair of instances
			# "location": location # location of service in local
		}

		url = 'http://127.0.0.1:3000/api/v1/workflow'
		headers = {'Content-type':'application/json', 'Accept': 'text/plain'}
		r = requests.post(url, data=json.dumps(post), headers=headers)
		ret_JSON = r.json()
		ret_status = r.status_code
		if ret_status == 200:
			return 0, ret_JSON['_id']
		else:
			error = ret_JSON['error']
			if error == 'Workflow name has already existed':
				return 1, ''
			else:
				return -1, ''

	def update_workflow(self, _id, op, value):
		"""
		op: field of what you want to update
		value: update value for the field
		return code:
		0: success
		1: workflow name not found
		"""

		post = {
			"option": "update",
			"_id": _id,
			"op": op,
			"value": value
		}
		
		url = 'http://127.0.0.1:3000/api/v1/workflow'
		headers = {'Content-type':'application/json', 'Accept': 'text/plain'}
		r = requests.post(url, data=json.dumps(post), headers=headers)
		ret_JSON = r.json()
		ret_status = r.status_code
		if ret_status == 200:
			return 0
		else:
			error = ret_JSON['error']
			if error == 'Workflow not exists':
				return 1
			elif error == 'Updated name already used':
				return 2
			else:
				return -1

	def delete_workflow(self, _id):
		"""
		return code:
		0: success
		1: workflow not exists
		"""
		
		post = {
			"option": "delete",
			"_id": _id
		}
		
		url = 'http://127.0.0.1:3000/api/v1/workflow'
		headers = {'Content-type':'application/json', 'Accept': 'text/plain'}
		r = requests.post(url, data=json.dumps(post), headers=headers)
		ret_JSON = r.json()
		ret_status = r.status_code
		if ret_status == 200:
			return 0
		else:
			error = ret_JSON['error']
			if error == 'Workflow not exists':
				return 1
			else:
				return -1

	def add_instance(self, _id, name, host, port):
		"""
		return code:
		0: success
		1: host/port not valid
		2: service name not exist
		3: host/port already used
		"""

		# list the attributes for the interface
		post = {
			"option": "add",
			"_id": _id, # name of service
			"name": name, # acronym of service
			"host": host, # number of instance
			"port": port # host/port pair of instances
			# "location": location # location of service in local
		}

		url = 'http://127.0.0.1:3000/api/v1/instance'
		headers = {'Content-type':'application/json', 'Accept': 'text/plain'}
		r = requests.post(url, data=json.dumps(post), headers=headers)
		ret_JSON = r.json()
		ret_status = r.status_code
		if ret_status == 200:
			return 0, ret_JSON['instance_id']
		else:
			error = ret_JSON['error']
			if error == 'Host/port pair not valid':
				return 1, 0
			elif error == 'Service not exists':
				return 2, 0
			elif error == 'Host/port pair already used':
				return 3, 0
			else:
				return -1, 0

	def update_instance(self, _id, instance_id, op, value):
		"""
		op: field of what you want to update
		value: update value for the field
		return code:
		0: success
		1: instance name not found
		"""

		post = {
			"option": "update",
			"_id": _id, # name of service
			"instance_id": instance_id, # acronym of service
			"op": op, # number of instance
			"value": value # host/port pair of instances
		}

		url = 'http://127.0.0.1:3000/api/v1/instance'
		headers = {'Content-type':'application/json', 'Accept': 'text/plain'}
		r = requests.post(url, data=json.dumps(post), headers=headers)
		ret_JSON = r.json()
		ret_status = r.status_code
		if ret_status == 200:
			return 0
		else:
			error = ret_JSON['error']
			if error == 'Instance not exists':
				return 1
			elif error == 'Host/port pair is not valid':
				return 2
			elif error == 'Updated host/port has already been used':
				return 3
			else:
				return -1

	def delete_instance(self, _id, instance_id):
		"""
		return code:
		0: success
		1: instance name not exist
		"""
		
		post = {
			"option": "delete",
			"_id": _id, # name of service
			"instance_id": instance_id # acronym of service
		}

		url = 'http://127.0.0.1:3000/api/v1/instance'
		headers = {'Content-type':'application/json', 'Accept': 'text/plain'}
		r = requests.post(url, data=json.dumps(post), headers=headers)
		ret_JSON = r.json()
		ret_status = r.status_code
		if ret_status == 200:
			return 0
		else:
			error = ret_JSON['error']
			if error == 'Instance not exists':
				return 1
			else:
				return -1
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