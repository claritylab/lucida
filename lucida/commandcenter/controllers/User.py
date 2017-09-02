from flask import *
from Database import database
from RegistrationForm import RegistrationForm
from LoginForm import LoginForm
from AccessManagement import login_required
from itsdangerous import (TimedJSONWebSignatureSerializer as Serializer, BadSignature, SignatureExpired)

user = Blueprint('user', __name__, template_folder='templates')
secret = 'A0Zr98j/3yX R~XHH!jmN]LWX/,?RT'

@user.route('/signup', methods=['GET', 'POST'])
def signup_route():
	# Check if the viewer already has a session.
	if 'logged_in' in session:
		# Display.
		return redirect(url_for('main.main_route')) # cannot register again		
	# Retrieve the registration form.
	form = RegistrationForm(request.form)
	options = {
		'error': []
	}
	# Deal with POST requests.
	if request.method == "POST":
		if form.validate():
			# Insert the user into database.
			database.add_user(form.username.data, form.firstname.data,
				form.lastname.data, form.password1.data,
				form.email.data)
			# Display.
			return redirect(url_for('user.login_route')) # still need to log in
		else:
			options['error'] = []
			for error_list in form.errors.items():
				options['error'] += error_list[1]
			# note: form.errors.items() is like:
			#       [('username', ['error message', 'error message']),
			#       ('password', ['error message'])]
			# Display.
			return render_template("signup.html", form=form, **options)
	# Display.
	return render_template("signup.html", form=form, **options)

@user.route('/user', methods=['GET'])
@login_required
def profile_route():
	s = Serializer(secret, expires_in = 300)
	options = {
		'token': s.dumps({ 'username': session['username'] }),
		'interfaces': database.list_interfaces(session['username'])
	}
	return render_template("profile.html", **options)

@user.route('/login', methods=['GET', 'POST'])
def login_route():
	# Retrieve the registration form.
	form = LoginForm(request.form)
	options = {
		'error': []
	}
	# Deal with POST requests.
	if request.method == "POST":
		if form.validate():
			if not database.username_exists(form.username.data):
				options['error'] = ['Username does not exist']
				# Display.
				return render_template("login.html", form=form, **options)
			# Check if the password is correct.
			password_is_correct = database.check_password(form.username.data, 
				form.password.data)
			if not password_is_correct:
				options['error'] = \
					['Password is incorrect for the specified username']
				# Display.
				return render_template("login.html", form=form, **options)
			# Update session.
			session['logged_in'] = True
			session['username'] = form.username.data
			# Get the url argument.
			url = request.args.get('url')
			if not url is None: # requested a private page and passed the check
				return redirect(url)
			# Display.
			return redirect(url_for('main.main_route'))
	# Display.
	return render_template("login.html", form=form, **options)

@user.route('/api/add_interface', methods=['POST'])
def api_add_interface_route():
	if not request.form or not 'interface' in request.form or not 'token' in request.form or not 'username' in request.form:
		abort(400)
	s = Serializer(secret)
	try:
		data = s.loads(request.form['token'])
	except SignatureExpired:
		abort(401)
	except BadSignature:
		abort(403)
	database.add_interface(data['username'], request.form['interface'], request.form['username'])
	return 'Successfully added user ' + data['username'] + ' to interface ' + request.form['interface'] + '.', 200

@user.route('/logout', methods=['GET', 'POST'])
@login_required
def logout_route():
	# Remove contents of cookie.
	session.clear()
	# Redirect to the default home page.
	return redirect(url_for('main.main_route'))
