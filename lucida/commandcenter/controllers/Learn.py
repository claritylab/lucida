from flask import *
import Database, ThriftClient
from ConcurrencyManagement import log
from AccessManagement import login_required
from werkzeug import secure_filename
from Database import database 
from ThriftClient import thrift_client


learn = Blueprint('learn', __name__, template_folder='templates')

# Returns true if the extension of the file is valid.
def image_allowed(upload_file):
	filename = secure_filename(upload_file.filename)
	allowed_extensions = set(['png', 'PNG', 'jpg', 'JPG', 'bmp', 'BMP', \
		'gif', 'GIF'])
	return (('.' in filename) and \
		(filename.rsplit('.', 1)[1] in allowed_extensions))

@learn.route('/learn', methods=['GET', 'POST'])
@login_required
def learn_route():
	try:
		# Deal with POST requests.
		if request.method == 'POST':
			# If the request does not contain an "op" field.
			if not 'op' in request.form:
				raise RuntimeError('Did you click the Add button?')
			# Add image knowledge.
			elif request.form['op'] == 'add_image':
				# Get the uploaded image.
				upload_file = request.files['file']
				if (not (upload_file and image_allowed(upload_file))):
					raise RuntimeError('Invalid file')
				# Check the label of the image.
				if not 'label' in request.form or \
					request.form['label'].isspace():
					raise RuntimeError('Please enter a label for your image')
				# Send the image to IMM.
				image_data = upload_file.read()
				upload_file.close()
				thrift_client.learn_image(session['username'],
										  request.form['label'],
										  image_data)
				# Add the image into the database.
				database.add_image(session['username'],
								   request.form['label'],
								   image_data)
			# Add text knowledge.
			elif request.form['op'] == 'add_text':
				# CHeck the text knowledge.
				if not 'text_knowledge' in request.form \
					or request.form['text_knowledge'].isspace():
					raise RuntimeError(
						'Please enter a piece of text as knowledge')
				# Send the text to QA.
				thrift_client.learn_text(session['username'],
										 request.form['text_knowledge'])
				# Add the text knowledge into the database.
				database.add_text(session['username'],
								  request.form['text_knowledge'])
			elif request.form['op'] == 'secret':
				database.secret(session['username'])			
			else:
				raise RuntimeError('Did you click the Add button?')
	except Exception as e:
		log(e)
		return render_template('learn.html', error=e)
	# Display.
	return render_template('learn.html', 
		pictures=database.get_images(session['username']),
		text=database.get_text(session['username']))
