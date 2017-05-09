import hashlib, uuid
from pymongo import MongoClient
from base64 import b64encode
from Utilities import log
import os
import Config
from Memcached import memcached


class Database(object):
	# Name of the algorithm to use for password encryption.
	ENCRYPT_ALGORITHM = 'sha512'

	# Constructor.
	def __init__(self):
		mongodb_addr = os.environ.get('MONGO_PORT_27017_TCP_ADDR')
		if mongodb_addr:
			log('MongoDB: ' + mongodb_addr)
			self.db = MongoClient(mongodb_addr, 27017).lucida
		else:
			log('MongoDB: localhost')
			self.db = MongoClient().lucida
		self.users = self.db.users

	# Returns the image collection of the user.
	def get_image_collection(self, username):
		images_collection = 'images_' + username
		return self.db[images_collection]

	# Returns the text collection of the user.
	def get_text_collection(self, username):
		text_collection = 'text_' + username
		return self.db[text_collection]

	# Adds a new user.
	def add_user(self, username, firstname, lastname, password, email):
		salt = uuid.uuid4().hex # thwart rainbow attack
		hashed_password = self.hash_password(self.ENCRYPT_ALGORITHM,
			salt, password)
		self.users.insert_one({'username' : username,
			'firstname': firstname, 'lastname': lastname,
			'password': hashed_password, 'email': email})
		# Add the password entry to memcached,
		# which auto-expire after 60 seconds.
		memcached.client.set(username, hashed_password, time=60)

	# Adds a new interface for a user.
	def add_interface(self, username, interface, interface_uid):
		interface += "_interface"
		self.users.update({'username' : username},
			{'$set': {interface: interface_uid}}, upsert=False)

	# List interface for a user.
	def list_interfaces(self, username):
		if not self.username_exists(username):
			return []
		row = self.users.find_one({'username': username})
		return [key for key,value in row.items() if key.endswith("_interface")]

	# Returns true if password of the user is correct
	def check_password(self, username, input_password):
		# Try memcached first.
		correct_password_in_db = memcached.client.get(username)
		if not correct_password_in_db:
			correct_password_in_db = (self.users.find_one
				({'username': username}))['password']
			memcached.client.set(username, correct_password_in_db, time=60)
		salt = correct_password_in_db.split('$')[1]
		generated_password = self.hash_password(self.ENCRYPT_ALGORITHM,
			salt, input_password)
		return correct_password_in_db == generated_password

	# Generates a hashed password from the raw password.
	def hash_password(self, algorithm, salt, password):
		m = hashlib.new(algorithm)
		password = password.encode('utf-8')
		s = salt + password
		m.update(s)
		password_hash = m.hexdigest()
		return "$".join([algorithm, salt, password_hash])

	#Returns true if the username already exists.
	def username_exists(self, username):
		return not self.users.find_one({'username': username}) is None

	#Returns true if the username already exists.
	def get_username(self, interface, interface_uid):
		interface += "_interface"
		row = self.users.find_one({interface: interface_uid});
		if not row is None:
			return row['username']
		return None

	# Adds the uploaded image.
	def add_image(self, username, image_data, label, image_id):
		self.get_image_collection(username).insert_one(
			{'label': label, 'data': b64encode(image_data), # encoded
			 'image_id': image_id})

	# Deletes the specified image.
	def delete_image(self, username, image_id):
		self.get_image_collection(username).remove({'image_id': image_id})

	# Returns all the images by username.
	def get_images(self, username):
		log('Retrieving all images from images_' + username)
		# Notice image['data'] was encoded using Base64.
		return [image for image in self.get_image_collection(username).find({}, { '_id': 0 })]

	# Checks whether the user can add one more image.
	def check_add_image(self, username):
		if self.get_image_collection(username).count() >= \
			Config.MAX_DOC_NUM_PER_USER:
			raise RuntimeError('Sorry. You can only add ' + 
				str(Config.MAX_DOC_NUM_PER_USER) + \
				' images at most')
	# Returns the number of images by username.
	def count_images(self, username):
		log('Retrieving the number of images from images_' + username)
		return self.get_image_collection(username).count()

	# Adds the knowledge text.
	def add_text(self, username, text_type, text_data, text_id):
		self.get_text_collection(username).insert_one(
			{'type': text_type, 'text_data': text_data,
			 'text_id': text_id})

	# Deletes the knowledge text.
	def delete_text(self, username, text_id):
		self.get_text_collection(username).delete_one(
			{'text_id': text_id})

	# Returns the knowledge text by username.
	def get_text(self, username):
		log('Retrieving text from text_' + username)
		return [text for text in self.get_text_collection(username).find({}, { '_id': 0 })]

	# Checks whether the user can add one more piece of text.
	def check_add_text(self, username):
		if self.get_text_collection(username).count() >= \
			Config.MAX_DOC_NUM_PER_USER:
			raise RuntimeError('Sorry. You can only add ' + 
				str(Config.MAX_DOC_NUM_PER_USER) + \
				' pieces of text at most')

database = Database()
