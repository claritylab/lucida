from flask import *
from Database import db

workflow_api = Blueprint('workflow_api', __name__, template_folder='templates')

@workflow_api.route('/api/v1/workflow', methods = ['GET', 'POST'])
def workflow_api_route():
	"""
	request json object (see detail in documents of API):
	{
		'option': add/update/delete
	}
	"""
	if request.method == 'GET':
		result = db.get_workflows()
		JSON_obj = {'workflow_list': result}
		return jsonify(JSON_obj), 200

	elif request.method == 'POST':
		requestFields = request.get_json()

		if 'option' not in requestFields:
			error = {'error': 'No option privided'}
			return jsonify(error), 422

		option = requestFields['option']

		if option == 'add':
			if 'name' not in requestFields or 'input' not in requestFields or \
					'classifier' not in requestFields or 'code' not in requestFields:
				error = {'error': 'Field missing for adding workflow'}
				return jsonify(error), 422

			ret, _id = db.add_workflow(requestFields['name'], requestFields['input'], 
				requestFields['classifier'], requestFields['code'])

			if ret == 1:
				error = {'error': 'Workflow name has already existed'}
				return jsonify(error), 422
			elif ret == 0:
				result = {'success': 'Workflow successfully added!', '_id': _id}
				return jsonify(result), 200

		elif option == 'add_empty':
			ret, _id = db.add_empty_workflow()

			if ret == 0:
				result = {'success': 'Workflow successfully added!', '_id': _id}
				return jsonify(result), 200

		elif option == 'update':
			if '_id' not in requestFields or 'op' not in requestFields or 'value' not in requestFields:
				error = {'error': 'Field missing for updating workflow'}
				return jsonify(error), 422

			ret = db.update_workflow(requestFields['_id'], requestFields['op'], requestFields['value'])

			if ret == 1:
				error = {'error': 'Workflow not exists'}
				return jsonify(error), 422
			elif ret == 2:
				error = {'error': 'Updated name already used'}
				return jsonify(error), 422
			elif ret == 0:
				success = {'success': 'Workflow successfully updated!'}
				return jsonify(success), 200

		elif option == 'delete':
			if '_id' not in requestFields:
				error = {'error': 'Field missing for deleting workflow'}
				return jsonify(error), 422

			ret = db.delete_workflow(requestFields['_id'])

			if ret == 1:
				error = {'error': 'Workflow not exists'}
				return jsonify(error), 422
			elif ret == 0:
				success = {'success': 'Workflow successfully deleted!'}
				return jsonify(success), 200