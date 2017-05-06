from flask import *
from Database import database
from AccessManagement import login_required
from ThriftClient import thrift_client
from QueryClassifier import query_classifier
from Utilities import log, check_image_extension
import os
import json

infer = Blueprint('infer', __name__, template_folder='templates')

@infer.route('/infer', methods=['GET', 'POST'])
@login_required
def infer_route():
	options = {}
	if os.environ.get('ASR_ADDR_PORT'):
		options['asr_addr_port'] = os.environ.get('ASR_ADDR_PORT')
	else:
		options['asr_addr_port'] = 'ws://localhost:8081'
	try:
		form = request.form
		# Deal with POST requests.
		if request.method == 'POST':
			# If the request does not contain an "op" field.
			if not 'op' in form:
				raise RuntimeError('Did you click the Ask button?')
			# When the "op" field is equal to "add_image".
			elif form['op'] == 'infer':
				# Check input file.
				upload_file = request.files['file'] if 'file' in request.files \
					else None
				if not upload_file is None and upload_file.filename != '':
					check_image_extension(upload_file)
				# Classify the query.
				speech_input = form['speech_input'] if 'speech_input' in form \
					else ''
				print '@@@@@@@@@@', speech_input
				services_needed = \
					query_classifier.predict(speech_input, upload_file)
				options['result'] = thrift_client.infer(session['username'], 
					services_needed, speech_input, upload_file.read()
					if upload_file else None)
				log('Result ' + options['result'])
				# Check if Calendar service is needed.
				# If so, JavaScript needs to receive the parsed dates.
				if services_needed.has_service('CA'):
					options['dates'] = options['result']
					options['result'] = None
					return render_template('infer.html', **options)
			else:
				raise RuntimeError('Did you click the Ask button?')
	except Exception as e:
		log(e)
		if str(e) == 'TSocket read 0 bytes':
			e = 'Back-end service encountered a problem'
		options['error'] = e
		return render_template('infer.html', **options)
	# Display.
	return render_template('infer.html', **options)

@infer.route('/api/infer', methods=['POST'])
def api_infer_route():
	options = {}

	# Verify request.
	if not 'username' in request.form or not 'interface' in request.form or not 'text_input' in request.form :
		abort (400)

	session['username'] = database.get_username(request.form['interface'], request.form['username'])
	if session['username'] == None:
		abort (403)

	session['logged_in'] = True
	print '@@@@@@@@', session['username']

	try:
		# Check input file.
		upload_file = request.files['file'] if 'file' in request.files \
			else None
		if not upload_file is None and upload_file.filename != '':
			check_image_extension(upload_file)
		# Classify the query.
		text_input = request.form['text_input'] if 'text_input' in request.form \
			else ''
		print '@@@@@@@@@@', text_input
		services_needed = \
			query_classifier.predict(text_input, upload_file)
		options['result'] = thrift_client.infer(session['username'], 
			services_needed, text_input, upload_file.read()
			if upload_file else None)
		log('Result ' + options['result'])
		# Check if Calendar service is needed.
		# If so, JavaScript needs to receive the parsed dates.
		if services_needed.has_service('CA'):
			options['dates'] = options['result']
			options['result'] = None
	except Exception as e:
		log(e)
		if str(e) == 'TSocket read 0 bytes':
			e = 'Back-end service encountered a problem'
		options['error'] = e
		abort(500)
	return json.dumps(options), 200
