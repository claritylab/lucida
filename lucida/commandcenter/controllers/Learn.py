from flask import *
from Database import Database
from ConcurrencyManagement import log
from AccessManagement import login_required
from ThriftClient import ThriftClient
from werkzeug import secure_filename


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
	# Deal with POST requests.
	if request.method == 'POST':
		# If the request does not contain an "op" field.
		if not 'op' in request.form:
			abort(404)
		# When the "op" field is equal to "add_image".
		elif request.form['op'] == 'add_image':
			# Get the uploaded image.
			upload_file = request.files['file']
			if (not (upload_file and image_allowed(upload_file))):
				return render_template('learn.html', errors='Invalid file')
			# Get the label of the image.
			if not ('label' in request.form and request.form['label']):
				return render_template('learn.html',
					errors='Please enter a label for your image')
			# Add the new photo into the database by a thrift call.
			image_data = upload_file.read()
			upload_file.close()
			try:
				Database.add_picture(session['username'],
									request.form['label'],
									image_data)
			except Exception as e:
				log(e)
				return render_template('learn.html', errors=e)
			# Send the image to IMM.
			try:
				ThriftClient.learn_image(session['username'],
										request.form['label'],
										image_data)
			except Exception as e:
				log(e)
				Database.delete_picture(session['username'],
									request.form['label'])
				return render_template('learn.html', errors=e)
# 		# When the "op" field is equal to "delete"
# 		elif request.form["op"] == "delete":
# 			# If the request does not have an "albumid" or a "picid" field,
# 			# abort
# 			if (not "albumid" in request.form) or (not "picid" in request.form):
# 				abort(404)
# 			# If the "albumid" field does not contain a valid albumid, abort
# 			if not is_valid_album_id(request.form["albumid"]):
# 				abort(404)
# 			# If the "picid" field does not contain a valid picid, abort
# 			if not is_valid_picid(request.form["picid"]):
# 				abort(404)
# 			# Now we can delete the specific photo
# 			delete_picture(get_picture(request.form["picid"]))
		# Abort
		else:
			abort(404)
	# Display.
	return render_template('learn.html', 
		pictures=Database.get_pictures(session['username']))
