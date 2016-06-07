from flask import *
import Database, ThriftClient
from ConcurrencyManagement import log
from AccessManagement import login_required
from Database import database 
from ThriftClient import thrift_client
from Utilities import check_image_extension, check_text_input


learn = Blueprint('learn', __name__, template_folder='templates')

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
				# Check the uploaded image.
				upload_file = request.files['file']
				if upload_file.filename == '':
					raise RuntimeError('Empty file is not allowed')
				check_image_extension(upload_file)
				# Check the label of the image.
				check_text_input(request.form['label'])
				# Send the image to IMM.
				image_data = upload_file.read()
				upload_file.close()
				thrift_client.learn_image(session['username'],
					request.form['label'], image_data)
				# Add the image into the database.
				database.add_image(session['username'],
					request.form['label'], image_data)
			# Add text knowledge.
			elif request.form['op'] == 'add_text':
				# Check the text knowledge.
				if not 'text_knowledge' in request.form:
					raise RuntimeError('Please enter a piece of text')
				check_text_input(request.form['text_knowledge'])
				# Send the text to QA.
				thrift_client.learn_text(session['username'],
					request.form['text_knowledge'])
				# Add the text knowledge into the database.
				database.add_text(session['username'],
					request.form['text_knowledge'])		
			else:
				raise RuntimeError('Did you click the Add button?')
	except Exception as e:
		log(e)
		return render_template('learn.html', error=e)
	# Display.
	return render_template('learn.html', 
		pictures=database.get_images(session['username']),
		text=database.get_text(session['username']))
