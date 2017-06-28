from flask import *
from QueryClassifier import query_classifier
from AccessManagement import login_required
from ThriftClient import thrift_client
from Service import Service
from Utilities import log
import Config
import socket

create = Blueprint('create', __name__, template_folder='templates')

@create.route('/create', methods=['GET', 'POST'])
@login_required
def create_route():
    options = {}
    if request.method == 'POST':
        if 'request' in request.form:
            if request.form['request'] == 'Update':
                Config.load_config()
                query_classifier.__init__(Config.TRAIN_OR_LOAD, Config.CLASSIFIER_DESCRIPTIONS)
    
    try:
        services_list = []
        for service in thrift_client.SERVICES.values():
            if isinstance(service, Service):
                services_list.append(service.name)
        options['service_list'] = sorted(services_list, key=lambda i: i[0])
        # Retrieve pre-configured services.
        instances_list = []
        for service in thrift_client.SERVICES.values():
            if isinstance(service, Service):
                for i in range(service.num):
                    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                    name = service.instance[i]['name']
                    host = service.instance[i]['host']
                    port = service.instance[i]['port']
                    result = sock.connect_ex((host, port))
                    if result == 0:
                        instances_list.append((service.name, name, host, port, "running"))
                    else:
                        instances_list.append((service.name, name, host, port, "stop"))
                    sock.close()
        options['instance_list'] = sorted(instances_list, key=lambda i: i[0])
    except Exception as e:
        log(e)
        options['error'] = e
    return render_template("create.html", **options)
