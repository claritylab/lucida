from flask import *
from AccessManagement import login_required
from ThriftClient import thrift_client


create = Blueprint('create', __name__, template_folder='templates')

@create.route('/create', methods=['GET', 'POST'])
@login_required
def create_route():
	# Display.
	services_list = []
	for service_name in thrift_client.SERVICES:
		host, port = thrift_client.get_service(service_name)
		services_list.append((service_name, host, port))
	return render_template("create.html", service_list=
		sorted(services_list, key=lambda i: i[0]))
