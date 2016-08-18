from flask import *
from Database import database
from RegistrationForm import RegistrationForm
from LoginForm import LoginForm
from AccessManagement import login_required


user = Blueprint('user', __name__, template_folder='templates')

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

@user.route('/logout', methods=['GET', 'POST'])
@login_required
def logout_route():
	# Remove contents of cookie.
	session.clear()
	# Redirect to the default home page.
	return redirect(url_for('main.main_route'))
