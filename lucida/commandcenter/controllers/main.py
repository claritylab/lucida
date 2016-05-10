import requests, json
from flask import *


main = Blueprint('main', __name__, template_folder='templates')

@main.route('/', methods=['GET',])
def main_route():
	# Display
	return render_template("index.html")

@main.route('/contact', methods=['GET',])
def contact_route():
	# Display
	return render_template("contact.html")
