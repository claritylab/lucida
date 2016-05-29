from flask import *
from ConcurrencyManagement import log
from AccessManagement import login_required
from ThriftClient import thrift_client
from QueryClassifier import query_classifier

infer = Blueprint('infer', __name__, template_folder='templates')


@infer.route('/infer', methods=['GET', 'POST'])
@login_required
def infer_route():
	result = None
	try:
		# Deal with POST requests.
		if request.method == 'POST':
			form = request.form
			# If the request does not contain an "op" field.
			if not 'op' in form:
				raise RuntimeError('Did you click the Ask button?')
			# When the "op" field is equal to "add_image".
			elif form['op'] == 'infer':
				# Classify the query.
				services_needed = \
					query_classifier.predict(form['speech_input'],
											 request.files['file'])
				result = thrift_client.infer(session['username'], 
											 services_needed,
								   			 form['speech_input'],
								   			 request.files['file'].read())
				log('Result ' + result)
				if services_needed == ['CA']:
					return render_template('infer.html', dates=result)
			else:
				raise RuntimeError('Did you click the Ask button?')
	except Exception as e:
		log(e)
		return render_template('infer.html', error=e)
	# Display.
	return render_template('infer.html', result=result)
