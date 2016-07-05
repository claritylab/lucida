from flask import *
import Database, ThriftClient
import hashlib, datetime
from ConcurrencyManagement import log
from AccessManagement import login_required
from Database import database 
from ThriftClient import thrift_client
from Utilities import check_image_extension, check_text_input


learn = Blueprint('learn', __name__, template_folder='templates')

@learn.route('/learn', methods=['GET', 'POST'])
@login_required
def learn_route():
	options = {}
	username = session['username']
	try:
		form = request.form
		# Deal with POST requests.
		if request.method == 'POST':
			# If the request does not contain an "op" field.
			if not 'op' in request.form:
				raise RuntimeError('Did you click the Add button?')
			# Add image knowledge.
			elif form['op'] == 'add_image':
				label = form['label']
				# Check the uploaded image.
				upload_file = request.files['file']
				if upload_file.filename == '':
					raise RuntimeError('Empty file is not allowed')
				check_image_extension(upload_file)
				# Check the label of the image.
				check_text_input(label)
				# Send the image to IMM.
				image_data = upload_file.read()
				upload_file.close()
				thrift_client.learn_image(username, label, image_data)
				# Add the image into the database.
				database.add_image(username, label, image_data)
			# Add text knowledge.
			elif form['op'] == 'add_text' or form['op'] == 'add_url':
				text_type = 'text' if form['op'] == 'add_text' else 'url'
				text_data = form['knowledge']
				# Check the text knowledge.
				check_text_input(text_data)
				# Generate the id.
				text_id = hashlib.md5(username + text_data
					+ str(datetime.datetime.now())).hexdigest()
				# Send the text to QA.
				thrift_client.learn_text(username, text_data,
						text_type, text_id)
				# Add the text knowledge into the database.
				database.add_text(username, text_data, text_type, text_id)	
			# Delete text knowledge.
			elif form['op'] == 'delete_text':
				text_type = 'unlearn'
				text_id = form['text_id']
				# Send the text to QA.
				thrift_client.learn_text(username, text_id, # id is the data
						text_type, text_id)
				# Delete the text from into the database.
				database.delete_text(username, text_id)			
			else:
				raise RuntimeError('Did you click the Add button?')
	except Exception as e:
		log(e)
		if str(e) == 'TSocket read 0 bytes':
			e = 'Back-end service encountered a problem'
		options['error'] = e
	# Display.
	options['pictures'] = database.get_images(username)
	options['text'] = database.get_text(username)
	return render_template('learn.html', **options)
