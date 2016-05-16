import requests, json
from flask import *


infer = Blueprint('infer', __name__, template_folder='templates')

@infer.route('/infer', methods=['GET', 'POST'])
def infer_route():
	# Display
	return render_template("infer.html")
