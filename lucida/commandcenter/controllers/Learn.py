from flask import *
from Database import Database
from AccessManagement import login_required
from ThriftClient import ThriftClient


learn = Blueprint('learn', __name__, template_folder='templates')

# Returns true if the file name's extension is valid.
def file_extension_allowed(filename):
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
			if (not (upload_file and file_extension_allowed(upload_file.filename))):
				abort(404)
			# Get the label of the image.
			if not ('label' in request.form and request.form['label']):
				abort(404)
			# Add the new photo into the database by a thrift call.
			image_data = upload_file.read()
			upload_file.close()
			Database.add_picture(session['username'],
								request.form['label'],
								image_data)
			# Send the image to IMM.
			ThriftClient.learn_image(session['username'],
									request.form['label'],
									image_data)
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
	options = {
		'pictures': Database.get_pictures(session['username'])
	}
	# Display.
	return render_template('learn.html', **options)



