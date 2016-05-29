import hashlib, uuid, glob
from pymongo import MongoClient
from base64 import b64encode
from ConcurrencyManagement import log
from ThriftClient import thrift_client


class Database():
	# Constructor.
	def __init__(self):
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
	
	# Name of the algorithm to use for password encryption.
	ENCRYPT_ALGORITHM = 'sha512'
	
	# Adds a new user.
	def add_user(self, username, firstname, lastname, password, email):
		salt = uuid.uuid4().hex # thwart rainbow attack
		hashed_password = self.hash_password(self.ENCRYPT_ALGORITHM,
			salt, password)
		self.users.insert_one({'username' : username,
						  	   'firstname': firstname,
						       'lastname': lastname,
						       'password': hashed_password,
						       'email': email})
	
	# Returns true if password of the user is correct
	def check_password(self, username, input_password):
		correct_password_in_db = (self.users.find_one
			({'username': username}))['password']
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
	
	# Adds the uploaded image.
	def add_image(self, username, label, upload_file):
		if not self.get_image_collection(username).find_one(
			{'label': label}) is None:
			raise RuntimeError('Image ' + label + ' already exists')
		self.get_image_collection(username).insert_one(
			{'label': label, 'data': b64encode(upload_file)})
		
	# Returns the images by username.
	def get_images(self, username):
		log('Retrieving images from images_' + username)
		return [image for image in self.get_image_collection(username).find()]
	
	# Adds the knowledge text.
	def add_text(self, username, text_knowledge):
		self.get_text_collection(username).insert_one(
			{'text_knowledge': text_knowledge})
		
	# Returns the knowledge text by username.
	def get_text(self, username):
		log('Retrieving text from text_' + username)
		return [text for text in self.get_text_collection(username).find()]
	
	# Deletes the specified image.
	def delete_image(self, username, label):
		self.get_image_collection(username).remove({'label': label})
		
	# Load knowledge from a given directory. Only for testing. 
	def secret(self, username, directory):
		if directory[-1] != '/':
			directory += '/'
		for image_path in glob.glob(directory + '*.jpg') \
			+ glob.glob(directory + '*.JPG'):
			image_label = image_path.split('/')[-1]
			with open(image_path) as f:
				log('Adding image ' + image_label)
				image_data = f.read()
				f.close()
				thrift_client.learn_image('yba', image_label, image_data)
				self.add_image(username, image_label, image_data)
		with open(directory + 'knowledge.txt') as f:
			lines = f.readlines()
			for line in lines:
				log('Adding text ' + line)
				thrift_client.learn_text('yba', line)
				self.add_text(username, line)
	
database = Database()
	