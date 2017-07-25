from flask import *
from Database import db

blackbox_api = Blueprint('blackbox_api', __name__, template_folder='templates')

@blackbox_api.route('/api/v1/blackbox', methods = ['GET', 'POST'])
def blackbox_api_route():
	"""
	request json object (see detail in documents of API):
	{
		'option': start
	}
	"""
	if request.method == 'GET':
		pass

	elif request.method == 'POST':
		requestFields = request.get_json()

		if 'option' not in requestFields:
			error = {'error': 'No option privided'}
			return jsonify(error), 422

		option = requestFields['option']

		if option == 'start':
			if '_id' not in requestFields or 'instance_id' not in requestFields:
				error = {'error': 'Field missing for starting server'}
				return jsonify(error), 422

			ret = db.start_server(requestFields['_id'], requestFields['instance_id'])
			print("haha")
			if ret == 1:
				error = {'error': 'Instance not exists'}
				return jsonify(error), 422
			elif ret == 0:
				success = {'success': 'Server successfully started!'}
				return jsonify(success), 200