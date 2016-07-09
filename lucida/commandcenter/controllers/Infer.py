from flask import *
from AccessManagement import login_required
from ThriftClient import thrift_client
from QueryClassifier import query_classifier
from Utilities import log, check_image_extension
import os

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
				upload_file = request.files['file']
				if upload_file.filename != '':
					check_image_extension(upload_file)
				print '@@@@@@@@@@', form['speech_input']
				# Classify the query.
				services_needed = \
					query_classifier.predict(form['speech_input'], upload_file)
				options['result'] = thrift_client.infer(session['username'], 
					services_needed, form['speech_input'], upload_file.read())
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
