from flask import *
from ConcurrencyManagement import log
from AccessManagement import login_required
from ThriftClient import thrift_client
from QueryClassifier import query_classifier
from Utilities import check_image_extension, check_text_input
import Config
import os

infer = Blueprint('infer', __name__, template_folder='templates')


@infer.route('/infer', methods=['GET', 'POST'])
@login_required
def infer_route():
	options = {
		'result': None,
		'asr_addr_port': request.url_root[7 : request.url_root.find('infer')]
	}
	if os.environ.get('DOCKER'):
		options['asr_addr_port'] = options['asr_addr_port'][:-1] + '2'
	else:
		options['asr_addr_port'] += '8081'
	try:
		# Deal with POST requests.
		if request.method == 'POST':
			form = request.form
			# If the request does not contain an "op" field.
			if not 'op' in form:
				raise RuntimeError('Did you click the Ask button?')
			# When the "op" field is equal to "add_image".
			elif form['op'] == 'infer':
				# Check input file.
				if request.files['file'].filename != '':
					check_image_extension(request.files['file'])
				# Classify the query.
				services_needed = \
					query_classifier.predict(form['speech_input'],
						request.files['file'])
				options['result'] = thrift_client.infer(session['username'], 
					services_needed, form['speech_input'],
					request.files['file'].read())
				log('Result ' + options['result'])
				if services_needed == ['CA']:
					options['dates'] = options['result']
					options['result'] = None
					return render_template('infer.html', **options)
			else:
				raise RuntimeError('Did you click the Ask button?')
	except Exception as e:
		log(e)
		options['error'] = e
		return render_template('infer.html', **options)
	# Display.
	return render_template('infer.html', **options)
