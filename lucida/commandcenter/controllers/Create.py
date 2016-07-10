from flask import *
from AccessManagement import login_required
from ThriftClient import thrift_client


create = Blueprint('create', __name__, template_folder='templates')

@create.route('/create', methods=['GET', 'POST'])
@login_required
def create_route():
	options = {}
	username = session['username']
	try:
		# Retrieve pre-configured services.
		services_list = []
		for service in thrift_client.SERVICES.values():
			host, port = service.get_host_port()
			services_list.append((service.name, host, port))
		
		# Deal with POST requests.
		if request.method == 'POST':
			# If the request does not contain an "op" field.
			if not 'op' in request.form:
				raise RuntimeError('Did you click the button?')
	except Exception as e:
		log(e)
		if str(e) == 'TSocket read 0 bytes':
			e = 'Back-end service encountered a problem'
		options['error'] = e			
	return render_template("create.html", service_list=
		sorted(services_list, key=lambda i: i[0]))
