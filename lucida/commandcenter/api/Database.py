import os
import sys
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

		service_list = self.db["service_info"].find()
		count_service = service_list.count()
		for i in range(count_service):
			document = service_list[i]
			document['_id'] = str(document['_id'])
			dictReturn.append(document)
		
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
		collection = self.db.service_info

		# check if current service name is used
		count = collection.count({'name': name})
		if count != 0:
			#collection.delete_many({"name" : sys.argv[2]})
			print('[python error] service name already used.')
			return 1, ''

		# check if current service acronym is used
		count = collection.count({'acronym': acronym})
		if count != 0:
			#collection.delete_many({"name" : sys.argv[2]})
			print('[python error] service acronym already used.')
			return 2, ''

		# list the attributes for the interface
		post = {
			"name": name, # name of service
			"acronym": acronym, # acronym of service
			"num": num, # number of instance
			"instance": instance, # host/port pair of instances
			"input": input_type, # input type
			"learn": learn_type # learn type
			# "location": location # location of service in local
		}

		# insert the service information into MongoDB
		post_id = collection.insert_one(post).inserted_id
		return 0, str(post_id)

	def add_empty_service(self):
		collection = self.db.service_info

		name = ''
		acronym = ''
		input_type = 'text'
		learn_type = 'none'
		num = 0
		instance = []

		post = {
			"name": name, # name of service
			"acronym": acronym, # acronym of service
			"num": num, # number of instance
			"instance": instance, # host/port pair of instances
			"input": input_type, # input type
			"learn": learn_type # learn type
			# "location": location # location of service in local
		}

		post_id = collection.insert_one(post).inserted_id
		return 0, str(post_id)


	def update_service(self, _id, op, value):
		"""
		op: field of what you want to update
		value: update value for the field
		return code:
		0: success
		1: service name not found
		"""

		collection = self.db.service_info

		count = collection.count({'_id': ObjectId(_id)})
		if count == 0:
			print('[python error] service not exists in MongoDB.')
			return 1

		# check if update no difference return success
		result = collection.find({'_id': ObjectId(_id)})[0]
		if result[op] == value:
			return 0

		if op == 'name':
			count = collection.count({'name': value})
			if count != 0:
				print('[python error] Updated name already used')
				return 2

		if op == 'acronym':
			count = collection.count({'acronym': value})
			if count != 0:
				print('[python error] Updated acronym already used')
				return 3

		collection.update({'_id': ObjectId(_id)}, {'$set': {op: value }})
		return 0

	def delete_service(self, _id):
		"""
		return code:
		0: success
		1: service not exist
		"""

		collection = self.db.service_info

		# check if current service is in MongoDB
		count = collection.count({'_id': ObjectId(_id)})
		if count == 0:
			print('[python error] service not exists in MongoDB.')
			return 1

		collection.remove({'_id': ObjectId(_id)})
		return 0

	def get_workflows(self):
		dictReturn = []

		workflow_list = self.db["workflow_info"].find()
		count_workflow = workflow_list.count()
		for i in range(count_workflow):
			document = workflow_list[i]
			document['_id'] = str(document['_id'])
			dictReturn.append(document)
		
		return dictReturn

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
			print('[python error] workflow name already used.')
			return 1, ''

		# list the attributes for the interface
		post = {
			"name": name, # name of workflow
			"input": input_type, # allowed input type
			"classifier": classifier_path, # classifier data path
			"code": class_code # code for implementation of the workflow class
		}

		post_id = collection.insert_one(post).inserted_id
		return 0, str(post_id)

	def add_empty_workflow(self):

		collection = self.db.workflow_info

		name = ''
		input_type = []
		classifier_path = ''
		class_code = ''
		stategraph = ''

		post = {
			"name": name, # name of workflow
			"input": input_type, # allowed input type
			"classifier": classifier_path, # classifier data path
			"code": class_code, # code for implementation of the workflow class
			"stategraph": stategraph
		}

		post_id = collection.insert_one(post).inserted_id
		return 0, str(post_id)

	def update_workflow(self, _id, op, value):
		"""
		op: field of what you want to update
		value: update value for the field
		return code:
		0: success
		1: workflow name not found
		"""

		collection = self.db.workflow_info

		count = collection.count({'_id': ObjectId(_id)})
		if count == 0:
			print('[python error] workflow not exists')
			return 1

		# check if update no difference return success
		result = collection.find({'_id': ObjectId(_id)})[0]
		if result[op] == value:
			return 0

		if op == 'name':
			count = collection.count({'name': value})
			if count != 0:
				print('[python error] Updated name already used')
				return 2

		collection.update({'_id': ObjectId(_id)}, {'$set': {op: value }})
		return 0

	def delete_workflow(self, _id):
		"""
		return code:
		0: success
		1: workflow not exists
		"""
		
		collection = self.db.workflow_info

		# check if current workflow is in MongoDB
		count = collection.count({'_id': ObjectId(_id)})
		if count == 0:
			print('[python error] workflow not exists')
			return 1

		collection.remove({'_id': ObjectId(_id)})
		return 0

	def add_instance(self, _id, name, host, port):
		"""
		return code:
		0: success
		1: host/port not valid
		2: service name not exist
		3: host/port already used
		"""

		collection = self.db.service_info
		if host == 'localhost':
			host = '127.0.0.1'

		if not validate_ip_port(host, port):
			print('[python error] Host/port pair is not valid.')
			return 1, ''

		# check if current service is in MongoDB
		object_id = ObjectId(_id)
		count = collection.count({'_id': object_id})
		if count != 1:
			print('[python error] service not exists in MongoDB.')
			return 2, ''

		"""
		# check if name is used
		result = collection.find({'name': service_name, 'instance.name': name})
		if result.count() != 0:
			print('[python error] Instance name has already been used.')
			return 3
		"""

		# check if host and port is used
		result = collection.find({'instance' : { '$elemMatch': { 'host': host, 'port': port}}})
		if result.count() != 0:
			print('[python error] Host/port has already been used.')
			return 3, ''

		result = collection.find({'_id': object_id})
		instance_id = str(result[0]['num'])
		collection.update_one({'_id': object_id}, {'$inc': {'num': 1}})
		collection.update_one({'_id': object_id}, {'$push': {'instance': {
			'name': name,
			'host': host,
			'port': port,
			'id': instance_id
		}}})
		return 0, instance_id

	def add_empty_instance(self, _id):
		collection = self.db.service_info

		name = ''
		host = '127.0.0.1'
		port = 0
		object_id = ObjectId(_id)

		result = collection.find({'_id': object_id})
		instance_id = str(result[0]['num'])
		collection.update_one({'_id': object_id}, {'$inc': {'num': 1}})
		collection.update_one({'_id': object_id}, {'$push': {'instance': {
			'name': name,
			'host': host,
			'port': port,
			'id': instance_id
		}}})
		return 0, instance_id

	def update_instance(self, _id, instance_id, op, value):
		"""
		op: field of what you want to update
		value: update value for the field
		return code:
		0: success
		1: instance name not found
		"""

		collection = self.db.service_info
		if op == 'host':
			if value == 'localhost':
				value = '127.0.0.1'

		# check if current service is in MongoDB
		object_id = ObjectId(_id)
		result = collection.find({'_id': object_id, 'instance.id': instance_id})
		if result.count() != 1:
			print('[python error] Instance name not exists.')
			return 1

		# check update nothing
		cur_instance = {}
		instance_result = result[0]['instance']
		for instance in instance_result:
			if instance['id'] == instance_id:
				cur_instance = instance
				if instance[op] == value:
					return 0

		if op == 'host':
			old_port = cur_instance['port']

			if not validate_ip_port(value, old_port):
				print('[python error] Host/port pair is not valid.')
				return 2

			result = collection.find({'instance': {'$elemMatch': {'host': value, 'port': old_port}}})
			if result.count() != 0:
				print('[python error] Updated host/port has already been used')
				return 3

		if op == 'port':
			old_host = cur_instance['host']

			if not validate_ip_port(old_host, value):
				print('[python error] Host/port pair is not valid.')
				return 2

			result = collection.find({'instance': {'$elemMatch': {'host': old_host, 'port': value}}})
			if result.count() != 0:
				print('[python error] Updated host/port has already been used')
				return 3

		op = 'instance.$.'+op
		collection.update({'_id': object_id, 'instance.id': instance_id}, {'$set': {op: value}})
		return 0

	def delete_instance(self, _id, instance_id):
		"""
		return code:
		0: success
		1: instance name not exist
		"""
		collection = self.db.service_info

		# check if current service is in MongoDB
		object_id = ObjectId(_id)
		result = collection.find({'_id': object_id, 'instance.id': instance_id})
		if result.count() != 1:
			print('[python error] Instance name not exists.')
			return 1

		collection.update({'_id':object_id}, {'$pull': {'instance': {'id': instance_id}}})
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
	if s == 'localhost':
		s = '127.0.0.1'
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