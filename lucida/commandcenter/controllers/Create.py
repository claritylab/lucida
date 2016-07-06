from flask import *
from AccessManagement import login_required
from ThriftClient import thrift_client


create = Blueprint('create', __name__, template_folder='templates')

@create.route('/create', methods=['GET', 'POST'])
@login_required
def create_route():
	# Display.
	services_list = []
	for service in thrift_client.SERVICES.values():
		host, port = service.get_host_port()
		services_list.append((service.name, host, port))
	return render_template("create.html", service_list=
		sorted(services_list, key=lambda i: i[0]))
