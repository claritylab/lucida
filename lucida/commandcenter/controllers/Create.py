from flask import *
from AccessManagement import login_required
from ThriftClient import thrift_client


create = Blueprint('create', __name__, template_folder='templates')

@create.route('/create', methods=['GET', 'POST'])
@login_required
def create_route():
	# Display.
	
	return render_template("create.html")

