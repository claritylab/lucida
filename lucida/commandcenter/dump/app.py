from __future__ import absolute_import
from __future__ import division
from __future__ import unicode_literals

import sys, glob, os
sys.path.insert(0, glob.glob(os.path.abspath(os.path.dirname(__file__)) +
    '/../../tools/thrift-0.9.3/lib/py/build/lib*')[0])

from controllers import *
from controllers.Parser import cmd_port
from flask import *
from flask_socketio import SocketIO, emit
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
app.register_blueprint(Speech.speech)

# Session.
app.secret_key = 'A0Zr98j/3yX R~XHH!jmN]LWX/,?RT'

socketio.init_app(app, async_mode='eventlet', allow_upgrades='true')

def flask_listener():
    print 'Starting non-secure flask'
#    socketio.run(app, host='0.0.0.0', port=3000, debug=True, use_reloader=False, certfile='certs/server.crt', keyfile='certs/server.key')
    socketio.run(app, host='0.0.0.0', port=3000, debug=True, use_reloader=False)

def web_socket_listener():
    print 'Start web socket at ' + str(cmd_port)
    logging.basicConfig(level=logging.DEBUG,
            format="%(levelname)8s %(asctime)s %(message)s ")
    logging.debug('Starting up server')
    WebSocket.tornado.options.parse_command_line()

    # For wss (with ASR capability)
    if os.environ.get('SECURE_HOST'):
        print 'Starting secure web socket'
        WebSocket.Application().listen(cmd_port, ssl_options={
            "certfile":"certs/server.crt",
            "keyfile":"certs/server.key"})
    # For ws (without ASR capability)
    else:
        print 'Starting non-secure web socket'
        WebSocket.Application().listen(cmd_port)

    WebSocket.tornado.ioloop.IOLoop.instance().start()

if __name__ == '__main__':
#    Thread(target = flask_listener).start()
#    web_socket_listener()
    flask_listener()
