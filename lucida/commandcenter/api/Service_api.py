from flask import *
from Database import db

service_api = Blueprint('service_api', __name__, template_folder='templates')

@service_api.route('/api/v1/service', methods = ['GET', 'POST'])
def service_api_route():
	"""
	request json object:
	{
		'op': add/update/delete
	}
	"""

	requestFields = request.get_json()

	if 'op' not in requestFields:
		error = {'error': 'No option privided'}
		return jsonify(error), 422

	op = requestFields['op']

	if op == 'add':
		if 'name' not in requestFields or 'acronym' not in requestFields or \
				'input' not in requestFields or 'learn' not in requestFields:
			error = {'error': 'Field missing for adding service'}
			return jsonify(error), 422

		ret, _id = db.add_service(requestFields['name'], requestFields['acronym'], 
			requestFields['input'], requestFields['learn'])

		if ret == 1:
			error = {'error': 'Service name has already existed'}
			return jsonify(error), 4




