from flask import *
from Database import db

service_api = Blueprint('service_api', __name__, template_folder='templates')

@service_api.route('/api/v1/service', methods = ['GET', 'POST'])
def service_api_route():
	"""
	request json object:
	{
		'option': add/update/delete
	}
	"""
	if request.method == 'GET':
		result = db.get_services()
		JSON_obj = {'service_list': result}
		return jsonify(JSON_obj), 200

	elif request.method == 'POST':
		requestFields = request.get_json()

		if 'option' not in requestFields:
			error = {'error': 'No option privided'}
			return jsonify(error), 422

		option = requestFields['option']

		if option == 'add':
			if 'name' not in requestFields or 'acronym' not in requestFields or \
					'input' not in requestFields or 'learn' not in requestFields:
				error = {'error': 'Field missing for adding service'}
				return jsonify(error), 422

			ret, _id = db.add_service(requestFields['name'], requestFields['acronym'], 
				requestFields['input'], requestFields['learn'])

			if ret == 1:
				error = {'error': 'Service name has already existed'}
				return jsonify(error), 422
			elif ret == 2:
				error = {'error': 'Service acronym has already used'}
				return jsonify(error), 422
			elif ret == 0:
				result = {'success': 'Service successfully added!', '_id': _id}
				return jsonify(result), 200

		elif option == 'add_empty':
			ret, _id = db.add_empty_service()

			if ret == 0:
				result = {'success': 'Service successfully added!', '_id': _id}
				return jsonify(result), 200

		elif option == 'update':
			if '_id' not in requestFields or 'option' not in requestFields or 'value' not in requestFields:
				error = {'error': 'Field missing for updating service'}
				return jsonify(error), 422

			ret = db.update_service(requestFields['_id'], requestFields['op'], requestFields['value'])

			if ret == 1:
				error = {'error': 'Service not exists'}
				return jsonify(error), 422
			elif ret == 2:
				error = {'error': 'Updated name already used'}
				return jsonify(error), 422
			elif ret == 3:
				error = {'error': 'Updated acronym already used'}
				return jsonify(error), 422
			elif ret == 0:
				success = {'success': 'Service successfully updated!'}
				return jsonify(success), 200

		elif option == 'delete':
			if '_id' not in requestFields:
				error = {'error': 'Field missing for deleting service'}
				return jsonify(error), 422

			ret = db.delete_service(requestFields['_id'])

			if ret == 1:
				error = {'error': 'Service not exists'}
				return jsonify(error), 422
			elif ret == 0:
				success = {'success': 'Service successfully deleted!'}
				return jsonify(success), 200