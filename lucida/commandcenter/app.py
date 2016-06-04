from __future__ import absolute_import
from __future__ import division
from __future__ import unicode_literals
 
import sys, glob  
#sys.path.insert(0, glob.glob('/home/yba/Documents/clarity/fbthrift/thrift/lib/py/build/lib*')[0]) # This needs to be more generic.
sys.path.insert(0, glob.glob('../../tools/thrift-0.9.3/lib/py/build/lib*')[0])
 
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

def thrift_listener():
    handler = ThriftServer.LucidaServiceHandler()
    processor = ThriftServer.LucidaService.Processor(handler)
    transport = ThriftServer.TSocket.TServerSocket(port=8080)
    pfactory = ThriftServer.TBinaryProtocol.TBinaryProtocolFactory()
    server = ThriftServer.TNonblockingServer.TNonblockingServer(processor,
        transport, pfactory, pfactory)
    print 'CMD at ' + str(8080)
    server.serve()
    
def flask_listener():
    app.run(host='0.0.0.0', port=3000, debug=True, use_reloader=False,
            threaded=True) 
    
def web_socket_listener():
    logging.basicConfig(level=logging.DEBUG,
                        format="%(levelname)8s %(asctime)s %(message)s ")
    logging.debug('Starting up server')
    WebSocket.tornado.options.parse_command_line()
    WebSocket.Application().listen(8888)
    WebSocket.tornado.ioloop.IOLoop.instance().start()
     
if __name__ == '__main__':
    Thread(target = thrift_listener).start()
    Thread(target = flask_listener).start()
    web_socket_listener()
