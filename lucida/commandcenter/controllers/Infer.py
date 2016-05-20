from flask import *
from Database import Database
from ConcurrencyManagement import log
from AccessManagement import login_required
from Learn import image_allowed
from ThriftClient import ThriftClient

infer = Blueprint('infer', __name__, template_folder='templates')


@infer.route('/infer', methods=['GET', 'POST'])
@login_required
def infer_route():
	options = {}
	# Deal with POST requests.
	if request.method == 'POST':
		# If the request does not contain an "op" field.
		if not 'op' in request.form:
			abort(404)
		# When the "op" field is equal to "add_image".
		elif request.form['op'] == 'infer':
			# Get the uploaded image.
			upload_file = request.files['file']
			if (not (upload_file and image_allowed(upload_file))):
				return render_template('infer.html', errors='Invalid file')
			# Send the image to IMM.
			image_data = upload_file.read()
			upload_file.close()
			try:
				result = ThriftClient.infer_image(session['username'],
												image_data)
			except Exception as e:
				log(e)
				options['errors'] = [e]
				return render_template('infer.html', errors=e)
			# Filter all the images owned by the user with label equal to result.
			options['result_pictures'] = []
			for image in Database.get_pictures(session['username']):
				if image['label'] == result:
						options['result_pictures'].append(image)
		# Abort.
		else:
			abort(404)
	# Display.
	return render_template('infer.html', **options)
