from wtforms import Form, TextField, PasswordField, validators, ValidationError
from Database import database
import re


# A custom validator to check if an email input is valid.
def email_check(form, field):
	if not re.match(r'[^@]+@[^@]+\.[^@]+', form.email.data):
		raise ValidationError('Email address must be valid')

# A custom validator to check if a username input already exists.
def username_existence_check(form, field):
	if database.username_exists(form.username.data): # already exists
		raise ValidationError('This username is taken')

# Returns an error message when the field has length less than min_length.
def too_short_error_msg(field, min_length):
	return field + ' must be at least ' + str(min_length) + ' characters long'

# Returns an error message when the field has length greater than max_length.
def too_long_error_msg(field, max_length):
	return field + ' must be no longer than ' + str(max_length) + ' characters long'

# Returns an error message when the field has characters
# other than letters, digits, and underscores.
def special_char_error_msg(field):
	return field + ' may only contain letters, digits, and underscores'	

class RegistrationForm(Form):	
	# Fields of the form.
	username = TextField('Username',
						 [username_existence_check,
						 validators.Length(
						 min=3, message=too_short_error_msg('Usernames', 3)),
						 validators.Length(
						 max=20, message=too_long_error_msg('Username', 20)),
						 validators.Regexp(
						 re.compile(r'^$|^[\w]+$'),
						 message=special_char_error_msg('Usernames'))])

	firstname = TextField('First name',
						  [validators.Length(
						  max=20, message=too_long_error_msg('Firstname', 20))])

	lastname = TextField('Last name',
						 [validators.Length(
						 max=20, message=too_long_error_msg('Lastname', 20))])

	password1 = PasswordField('New Password',
							 [validators.Length(
							 min=8, message=too_short_error_msg('Passwords', 8)),
							 validators.Regexp(
							 re.compile(r'\d.*([A-Z]|[a-z])|([A-Z]|[a-z]).*\d'),
							 message='Passwords must contain at least one letter and one number'),
							 validators.Regexp(
							 re.compile(r'^$|^[\w]+$'),
							 message=special_char_error_msg('Passwords'))])

	password2 = PasswordField('Repeat Password', 
							[validators.EqualTo('password1', message='Passwords do not match')])

	email = TextField('Email Address', 
					  [validators.Length(
					  max=255,
					  message=too_long_error_msg('Email', 255)),
					  email_check])
