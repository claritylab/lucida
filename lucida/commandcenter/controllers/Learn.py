from flask import *
import Database, ThriftClient
import hashlib, datetime
from AccessManagement import login_required
from Database import database
from ThriftClient import thrift_client
from Utilities import log, check_image_extension, check_text_input
import Config
import re

learn = Blueprint('learn', __name__, template_folder='templates')

def generic_learn_route(op, form, upload_file):
	options = {}
	username = session['username']
	try:
		# Add image knowledge.
		if op == 'add_image':
			image_type = 'image'
			label = form['label']
			_id = form['_id']
			# Check the uploaded image.
			if upload_file.filename == '':
				raise RuntimeError('Empty file is not allowed')
			check_image_extension(upload_file)
			# Check the label of the image.
			check_text_input(label)
			# Check whether the user can add one more image.
			database.check_add_image(username)
			# Generate the id.
			image_data = upload_file.read()
			image_id = hashlib.md5(username +
				str(datetime.datetime.now())).hexdigest()
			# Send the image to IMM.
			upload_file.close()
			thrift_client.learn_image(username, image_type, image_data,
				image_id, _id)
			# Add the image into the database.
			database.add_image(username, image_data, label, image_id, _id)
		# Delete image knowledge.
		elif op == 'delete_image':
			image_type = 'unlearn'
			image_id = form['image_id']
			_id = form['_id']
			# Send the unlearn request to IMM.
			thrift_client.learn_image(username, image_type, '', image_id, _id)
			# Delete the image from the database.
			database.delete_image(username, image_id, _id)
		# Add text knowledge.
		elif op == 'add_text' or op == 'add_url':
			text_type = 'text' if op == 'add_text' else 'url'
			text_data = form['knowledge']
			_id = form['_id']
			# Check the text knowledge.
			check_text_input(text_data)
			# Check whether the user can add one more piece of text.
			database.check_add_text(username)
			# Generate the id.
			text_id = hashlib.md5(username + text_data +
				str(datetime.datetime.now())).hexdigest()
			# Send the text to QA.
			thrift_client.learn_text(username, text_type,
					text_data, text_id, _id)
			# Add the text knowledge into the database.
			database.add_text(username, text_type, text_data, text_id, _id)
		# Delete text knowledge.
		elif op == 'delete_text':
			text_type = 'unlearn'
			text_id = form['text_id']
			_id = form['_id']
			# Send the unlearn request to QA.
			thrift_client.learn_text(username, text_type, '', text_id, _id)
			# Delete the text from into the database.
			database.delete_text(username, text_id, _id)
		else:
			raise RuntimeError('Did you click the button?')
	except Exception as e:
		log(e)
		options['errno'] = 500
		options['error'] = str(e)
		if 'code' in e and re.match("^4\d\d$", str(e.code)):
			options['errno'] = e.code
		if str(e) == 'TSocket read 0 bytes':
			options['error'] = 'Back-end service encountered a problem'
		if str(e).startswith('Could not connect to'):
			options['error'] = 'Back-end service is not running'
	return options

@learn.route('/learn', methods=['GET', 'POST'])
@login_required
def learn_route():
	options = {}
	
	# Deal with POST requests.
	if request.method == 'POST':
		options = generic_learn_route(request.form['op'], request.form, request.files['file'] if 'file' in request.files else None)
	
	# home page
	if '_id' not in request.args:
		options['text_list'] = []
		options['image_list'] = []
		for _id in Config.LEARNERS['text']:
			service = Config.get_service_withid(_id)
			options['text_list'].append({'name': service.name, '_id': _id})
		for _id in Config.LEARNERS['image']:
			service = Config.get_service_withid(_id)
			options['image_list'].append({'name': service.name, '_id': _id})
		return render_template('learn_home.html', **options)	
	
	try:
		_id = request.args['_id']
		service = Config.get_service_withid(_id)

		options['_id'] = _id
		options['service_name'] = service.name
				
		if _id in Config.LEARNERS['text']:
			options['text_able'] = 1
		else:
			options['text_able'] = 0

		# check if text field is necessary
		if _id in Config.LEARNERS['image']:
			options['image_able'] = 1
		else:
			options['image_able'] = 0

		if options['text_able'] == 0 and options['image_able'] == 0:
			abort(404)

		if options['text_able'] == 0:
			options['pictures'] = database.get_images(session['username'], _id)
		else:
			options['text'] = database.get_text(session['username'], _id)
	except Exception as e:
		log(e)
		options['errno'] = 500
		options['error'] = str(e)
	return render_template('learn.html', **options)

allowed_endpoints = ['add_image','delete_image','add_text','add_url','delete_text','query']
@learn.route('/api/learn/<string:op>', methods=['POST'])
def api_learn_add_del_route(op):
	if not op in allowed_endpoints:
		abort(404)

	options = {}
	if not op == 'query':
		options = generic_learn_route(op, request.form, request.files['file'] if 'file' in request.files else None)
	else:
		try:
			# Retrieve knowledge.	
			if 'type' in request.form and request.form['type'] == 'text':
				options['text'] = database.get_text(session['username'], request.form['_id'])
			elif 'type' in request.form and request.form['type'] == 'image':
				options['pictures'] = database.get_images(session['username'], request.form['_id'])
			else:
				options['pictures'] = database.get_images(session['username'], request.form['_id'])
				options['text'] = database.get_text(session['username'], request.form['_id'])
		except Exception as e:
			log(e)
			options['errno'] = 500
			options['error'] = str(e)
	if 'errno' in options:
		return json.dumps(options), options['errno']
	return json.dumps(options), 200
