from __future__ import absolute_import
from __future__ import division
from __future__ import unicode_literals
 
import sys, glob, os 
sys.path.insert(0, glob.glob(os.path.abspath(os.path.dirname(__file__)) + 
	'/../../tools/thrift-0.9.3/lib/py/build/lib*')[0])
 
from controllers import *
from flask import *
from threading import Thread
import logging


# Initialize the Flask app with the template folder address.
app = Flask(__name__, template_folder='templates')

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
	
def flask_listener():
	app.run(host='0.0.0.0', port=3000, debug=True, use_reloader=False,
			threaded=True) 
	
def web_socket_listener():
	print 'Start web socket at 8081'
	logging.basicConfig(level=logging.DEBUG,
						format="%(levelname)8s %(asctime)s %(message)s ")
	logging.debug('Starting up server')
	WebSocket.tornado.options.parse_command_line()
	WebSocket.Application().listen(8081)
	WebSocket.tornado.ioloop.IOLoop.instance().start()
	 
if __name__ == '__main__':
	Thread(target = flask_listener).start()
	web_socket_listener()
