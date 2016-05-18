from flask import *
from Database import Database
from AccessManagement import login_required
from Learn import file_extension_allowed
from ThriftClient import ThriftClient
from base64 import b64encode

infer = Blueprint('infer', __name__, template_folder='templates')

@infer.route('/infer', methods=['GET', 'POST'])
@login_required
def infer_route():
	options = {
		'uploaded_picture': None,
		'result_pictures': []
	}
	# Deal with POST requests.
	if request.method == 'POST':
		# If the request does not contain an "op" field.
		if not 'op' in request.form:
			abort(404)
		# When the "op" field is equal to "add_image".
		elif request.form['op'] == 'add_image':
			# Get the uploaded image.
			upload_file = request.files['file']
			if (not (upload_file and file_extension_allowed(upload_file.filename))):
				return 'Invalid file'
			# Send the image to IMM.
			image_data = upload_file.read()
			upload_file.close()
			result = ThriftClient.infer_image(session['username'], image_data)
			# Filter all the images owned by the user with label equal to result.
			options['uploaded_picture'] = b64encode(image_data)
			for image in Database.get_pictures(session['username']):
				if image['label'] == result:
						options['result_pictures'].append(image)
		# Abort.
		else:
			abort(404)
	# Display.
	return render_template('infer.html', **options)

@infer.route('/infer_ASR', methods=['GET', 'POST'])
@login_required
def infer_ASR_route():
	# Display.
	return render_template('infer_ASR.html')
