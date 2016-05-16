from wtforms import Form, TextField, PasswordField


class LoginForm(Form):
	# Fields of the form.
	username = TextField('Username')
	
	password = PasswordField('Password')

