import os, time, hashlib, uuid
from pymongo import MongoClient
from base64 import b64encode
from ThriftClient import ThriftClient
from ConcurrencyManagement import *


class Database(object):
	db = MongoClient().lucida
	users = db.users
	images = db.images_yba2
	
	# Name of the algorithm to use for password encryption.
	ENCRYPT_ALGORITHM = 'sha512'
	
	@staticmethod
	# Adds a new user.
	def add_user(username, firstname, lastname, password, email):
		salt = uuid.uuid4().hex # thwart rainbow attack
		hashed_password = Database.hash_password(Database.ENCRYPT_ALGORITHM,
												salt, password)
		Database.users.insert_one({'username' : username,
								'firstname': firstname,
								'lastname': lastname,
								'password': hashed_password,
								'email': email})
	
	@staticmethod
	# Returns true if password of the user is correct
	def check_password(username, input_password):
		correct_password_in_db = (Database.users.find_one
		({'username': username}))['password']
		salt = correct_password_in_db.split('$')[1]
		generated_password = Database.hash_password(Database.ENCRYPT_ALGORITHM,
												    salt, input_password)
		return correct_password_in_db == generated_password

	@staticmethod
	# Generates a hashed password from the raw password.
	def hash_password(algorithm, salt, password):
		m = hashlib.new(algorithm)
		password = password.encode('utf-8')
		s = salt + password
		m.update(s)
		password_hash = m.hexdigest()
		return "$".join([algorithm, salt, password_hash])
	
	@staticmethod
	#Returns true if the username already exists.
	def username_exists(username):
		return not Database.users.find_one({'username': username}) is None
	
	@staticmethod
	# Adds the uploaded image.
	def add_picture(username, label, upload_file):
		Database.images.insert_one({'label': label, 'data': b64encode(upload_file)})
		ThriftClient.learn_image(username, label, upload_file)
		
	@staticmethod
	# Returns the images by username.
	def get_pictures(username):
		images_db = "images_" + username
		log('Retrieving images from ' + images_db)
		return [image for image in Database.images.find()]

