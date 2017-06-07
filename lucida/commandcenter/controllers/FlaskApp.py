from __future__ import absolute_import
from __future__ import division
from __future__ import unicode_literals

import sys, glob, os
sys.path.insert(0, glob.glob(os.path.abspath(os.path.dirname(__file__)) +
    '/../../../tools/thrift-0.9.3/lib/py/build/lib*')[0])

from . import Main
from . import User
from . import Create
from . import Learn
from . import Infer
from .Parser import cmd_port
from flask import *
import logging


# Initialize the Flask app with the template folder address.
app = Flask(__name__, template_folder='../templates')

# app.config.from_object('config')
app.config['MAX_CONTENT_LENGTH'] = 16 * 1024 * 1024 # 16 MB due to MongoDB

# Register the controllers.
app.register_blueprint(Main.main)
app.register_blueprint(User.user)
app.register_blueprint(Create.create)
app.register_blueprint(Learn.learn)
app.register_blueprint(Infer.infer)

# Session.
app.secret_key = 'A0Zr98j/3yX R~XHH!jmN]LWX/,?RT'
