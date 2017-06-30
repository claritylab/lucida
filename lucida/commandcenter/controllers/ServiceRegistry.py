from flask import *
from AccessManagement import login_required

serviceregistry = Blueprint('serviceregistry', __name__, template_folder='templates')

@serviceregistry.route('/serviceregistry', methods=['GET', 'POST'])
@login_required
def serviceregistry_route():
	return render_template('serviceregistry.html')
