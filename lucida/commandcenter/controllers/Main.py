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


from flask.ext.login import login_user, logout_user, current_user, login_required
# from Auth import *

@main.route('/authorize/<provider>')
def oauth_authorize(provider):
    # Flask-Login function
#     if not current_user.is_anonymous():
#         return redirect(url_for('index'))
    oauth = OAuthSignIn.get_provider(provider)
    return oauth.authorize()

@main.route('/callback/<provider>')
def oauth_callback(provider):
    if not current_user.is_anonymous():
        return redirect(url_for('index'))
    oauth = OAuthSignIn.get_provider(provider)
    username, email = oauth.callback()
    if email is None:
        # I need a valid email address for my user identification
        flash('Authentication failed.')
        return redirect(url_for('index'))
    # Look if the user already exists
    user=User.query.filter_by(email=email).first()
    if not user:
        # Create the user. Try and use their name returned by Google,
        # but if it is not set, split the email address at the @.
        nickname = username
        if nickname is None or nickname == "":
            nickname = email.split('@')[0]

        # We can do more work here to ensure a unique nickname, if you 
        # require that.
        user=User(nickname=nickname, email=email)
        db.session.add(user)
        db.session.commit()
    # Log in the user, by default remembering them for their next visit
    # unless they log out.
    login_user(user, remember=True)
    return redirect(url_for('index'))
   
@main.route('/login_Googe', methods=['GET', 'POST'])
def login_Googe():
#     if g.user is not None and g.user.is_authenticated():
#         return redirect(url_for('index'))
    return render_template('login_Google.html',
                           title='Sign In')

