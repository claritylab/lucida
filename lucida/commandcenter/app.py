from __future__ import absolute_import
from __future__ import division
from __future__ import unicode_literals
 
import sys, glob  
#sys.path.insert(0, glob.glob('/home/yba/Documents/clarity/fbthrift/thrift/lib/py/build/lib*')[0]) # This needs to be more generic.
sys.path.insert(0, glob.glob('/home/yba/Documents/clarity/thrift-0.9.3/lib/py/build/lib*')[0])
 
 
 
from controllers import *

from flask import *
from threading import Thread
# from twisted.internet import reactor
import config


# Initialize the Flask app with the template folder address.
app = Flask(__name__, template_folder='templates')

app.config.from_object('config')

# Register the controllers.
app.register_blueprint(Main.main)
app.register_blueprint(Create.create)
app.register_blueprint(Learn.learn)
app.register_blueprint(Infer.infer)

# Session.
app.secret_key = 'A0Zr98j/3yX R~XHH!jmN]LWX/,?RT'

def thrift_server():
    handler = ThriftServer.LucidaServiceHandler()
    processor = ThriftServer.LucidaService.Processor(handler)
    transport = ThriftServer.TSocket.TServerSocket(port=8080)
    pfactory = ThriftServer.TBinaryProtocol.TBinaryProtocolFactory()
    server = ThriftServer.TNonblockingServer.TNonblockingServer(processor, transport, pfactory, pfactory)
    print 'CMD at ' + str(8080)
    server.serve()
    
def flask_server():
    app.run(host='0.0.0.0', port=3000, debug=True, use_reloader=False) 
     
if __name__ == '__main__':
    Thread(target = thrift_server).start()
    Thread(target = flask_server).start()
    


