import os
import sys
import requests
import json
from pymongo import *
from bson.objectid import ObjectId

url_pfx = 'http://127.0.0.1:3000' # url that the index page is hosted

class MongoDB(object):
	def __init__(self):
		mongodb_addr = os.environ.get('MONGO_PORT_27017_TCP_ADDR')
		if mongodb_addr:
			self.db = MongoClient(mongodb_addr, 27017).lucida
		else:
			self.db = MongoClient().lucida

	def get_services(self):
		dictReturn = []

		url = url_pfx + '/api/v1/service'
		r = requests.get(url)
		ret_JSON = r.json()
		dictReturn = ret_JSON['service_list']
		
		return dictReturn

	def add_service(self):
		"""
		return code:
		0: success
		"""

		# list the attributes for the interface
		post = {
			"option": "add_empty"
			# "location": location # location of service in local
		}

		url = url_pfx + '/api/v1/service'
		headers = {'Content-type':'application/json', 'Accept': 'text/plain'}
		r = requests.post(url, data=json.dumps(post), headers=headers)
		ret_JSON = r.json()
		ret_status = r.status_code
		if ret_status == 200:
			return 0, ret_JSON['_id']
		else:
			return -1, ''

	def update_service(self, _id, op, value):
		"""
		op: field of what you want to update
		value: update value for the field
		return code:
		0: success
		1: service name not found
		2: name already used
		3: acronym already used
		"""

		post = {
			"option": "update",
			"_id": _id,
			"op": op,
			"value": value
		}
		
		url = url_pfx + '/api/v1/service'
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
		
		url = url_pfx + '/api/v1/service'
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

	def get_workflows(self):
		dictReturn = []

		url = url_pfx + '/api/v1/workflow'
		r = requests.get(url)
		ret_JSON = r.json()
		dictReturn = ret_JSON['workflow_list']
		
		return dictReturn

	def add_workflow(self):
		"""
		return code:
		0: success
		"""

		# list the attributes for the interface
		post = {
			"option": "add_empty"
		}

		url = url_pfx + '/api/v1/workflow'
		headers = {'Content-type':'application/json', 'Accept': 'text/plain'}
		r = requests.post(url, data=json.dumps(post), headers=headers)
		ret_JSON = r.json()
		ret_status = r.status_code
		if ret_status == 200:
			return 0, ret_JSON['_id']
		else:
			return -1, ''

	def update_workflow(self, _id, op, value):
		"""
		op: field of what you want to update
		value: update value for the field
		return code:
		0: success
		1: workflow name not found
		2: updated name already used
		"""

		post = {
			"option": "update",
			"_id": _id,
			"op": op,
			"value": value
		}
		
		url = url_pfx + '/api/v1/workflow'
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
		
		url = url_pfx + '/api/v1/workflow'
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

	def add_instance(self, _id):
		"""
		return code:
		0: success
		1: service not valid
		"""

		# list the attributes for the interface
		post = {
			"option": "add_empty",
			"_id": _id
		}

		url = url_pfx + '/api/v1/instance'
		headers = {'Content-type':'application/json', 'Accept': 'text/plain'}
		r = requests.post(url, data=json.dumps(post), headers=headers)
		ret_JSON = r.json()
		ret_status = r.status_code
		if ret_status == 200:
			return 0, ret_JSON['instance_id']
		else:
			error = ret_JSON['error']
			if error == 'Service not exists':
				return 1, ''
			else:
				return -1, ''

	def update_instance(self, _id, instance_id, op, value):
		"""
		op: field of what you want to update
		value: update value for the field
		return code:
		0: success
		1: instance not found
		2: host/port not valid
		3: host/port already used
		"""

		post = {
			"option": "update",
			"_id": _id,
			"instance_id": instance_id,
			"op": op,
			"value": value 
		}

		url = url_pfx + '/api/v1/instance'
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
		1: instance not exist
		"""
		
		post = {
			"option": "delete",
			"_id": _id, 
			"instance_id": instance_id 
		}

		url = url_pfx + '/api/v1/instance'
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