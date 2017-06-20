#!/usr/bin/env python2

# Standard import
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

	def add_service(self, name, acronym, host, port, input_type, learn_type):
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
			"host": host,
			"port": port,
			"input": input_type,
			"learn": learn_type
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


