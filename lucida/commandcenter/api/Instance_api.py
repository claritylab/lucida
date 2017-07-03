from flask import *
from Database import db

instance_api = Blueprint('instance_api', __name__, template_folder='templates')

@instance_api.route('/api/v1/instance', methods = ['GET', 'POST'])
def instance_api_route():
	"""
	request json object:
	{
		'option': add/update/delete
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

		if option == 'add':
			if '_id' not in requestFields or 'name' not in requestFields or \
					'host' not in requestFields or 'port' not in requestFields:
				error = {'error': 'Field missing for adding instance'}
				return jsonify(error), 422

			ret, instance_id = db.add_instance(requestFields['_id'], requestFields['name'], 
				requestFields['host'], requestFields['port'])

			if ret == 1:
				error = {'error': 'Host/port pair not valid'}
				return jsonify(error), 422
			elif ret == 2:
				error = {'error': 'Service not exists'}
				return jsonify(error), 422
			elif ret == 3:
				error = {'error': 'Host/port pair already used'}
				return jsonify(error), 422
			elif ret == 0:
				result = {'success': 'Instance successfully added!', 'instance_id': instance_id}
				return jsonify(result), 200

		elif option == 'add_empty':
			if '_id' not in requestFields:
				error = {'error': 'Field missing for updating instance'}
				return jsonify(error), 422

			ret, instance_id = db.add_empty_instance(requestFields['_id'])

			if ret == 0:
				result = {'success': 'Instance successfully added!', 'instance_id': instance_id}
				return jsonify(result), 200

		elif option == 'update':
			if '_id' not in requestFields or 'instance_id' not in requestFields or \
					'op' not in requestFields or 'value' not in requestFields:
				error = {'error': 'Field missing for updating instance'}
				return jsonify(error), 422

			ret = db.update_instance(requestFields['_id'], requestFields['instance_id'], 
				requestFields['op'], requestFields['value'])

			if ret == 1:
				error = {'error': 'Instance not exists'}
				return jsonify(error), 422
			elif ret == 2:
				error = {'error': 'Host/port pair is not valid'}
				return jsonify(error), 422
			elif ret == 3:
				error = {'error': 'Updated host/port has already been used'}
				return jsonify(error), 422
			elif ret == 0:
				success = {'success': 'Instance successfully updated!'}
				return jsonify(success), 200

		elif option == 'delete':
			if '_id' not in requestFields or 'instance_id' not in requestFields:
				error = {'error': 'Field missing for deleting instance'}
				return jsonify(error), 422

			ret = db.delete_instance(requestFields['_id'], requestFields['instance_id'])

			if ret == 1:
				error = {'error': 'Instance not exists'}
				return jsonify(error), 422
			elif ret == 0:
				success = {'success': 'Instance successfully deleted!'}
				return jsonify(success), 200