#!/usr/bin/env python2

"""
Insert the micro service information into MongoDB
Zhexuan Chen 6/6/2017
"""

# Standard imports
import sys, os
from pymongo import *

def main():
	# check valid argument
	if len(sys.argv) <= 1:
		print('[python error] wrong number of argument.')
		exit(-1)

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

	# get the operation type
	op = sys.argv[1]

	if op == 'add':
		# check valid argument
		if len(sys.argv) != 9:
			print('[python error] wrong number of argument.')
			exit(-1)

		# check if current service is in MongoDB
		count = collection.count({'name': sys.argv[2]})
		if count != 0:
			#collection.delete_many({"name" : sys.argv[2]})
			print('[python error] service already in MongoDB.')
			exit(1)

		# list the attributes for the interface
		post = {
			"name": sys.argv[2],
			"acronym": sys.argv[3],
			"host": sys.argv[4],
			"port": sys.argv[5],
			"input": sys.argv[6],
			"learn": sys.argv[7],
			"location": sys.argv[8]
		}

		# insert the service information into MongoDB
		post_id = collection.insert_one(post).inserted_id

	elif op == 'check':
		# check valid argument
		if len(sys.argv) != 4:
			print('[python error] wrong number of argument.')
			exit(-1)

		# check if current service is in MongoDB
		count = collection.count({sys.argv[2]: sys.argv[3]})
		if count != 0:
			print('[python info] service already in MongoDB.')
			exit(1)
		else:
			print('[python info] service ' + sys.argv[2] + ' check pass.')

	elif op == 'check_host_port':
		# check valid argument
		if len(sys.argv) != 4:
			print('[python error] wrong number of argument.')
			exit(-1)

		# check if current service is in MongoDB
		count = collection.count({'host': sys.argv[2], 'port': sys.argv[3]})
		if count != 0:
			print('[python info] service already in MongoDB.')
			exit(1)
		else:
			print('[python info] service host/port check pass.')

	elif op == 'delete':
		# check valid argument
		if len(sys.argv) != 3:
			print('[python error] wrong number of argument.')
			exit(-1)

		# check if current service is in MongoDB
		count = collection.count({'name': sys.argv[2]})
		if count == 0:
			print('[python error] service not exists in MongoDB.')
			exit(1)

		collection.remove({'name': sys.argv[2]})

	elif op == 'delete_all':
		collection.remove()

	else:
		print('[python error] invalid operation for MongoDB.')
		exit(-1)
	
	return 0

if __name__ == '__main__':
	main()