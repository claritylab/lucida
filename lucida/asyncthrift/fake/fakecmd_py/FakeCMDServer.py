from __future__ import absolute_import
from __future__ import division
from __future__ import unicode_literals
 
import sys, glob  
sys.path.insert(0, glob.glob('/home/yba/Documents/clarity/fbthrift/thrift/lib/py/build/lib*')[0]) # This needs to be more generic.
 
from lucidatypes.ttypes import *
from lucidaservice import LucidaService
 
from zope.interface import implements
from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol
from thrift.server import TNonblockingServer

from flask import *
from threading import Thread

# Initialize Flask app with the template folder address
app = Flask(__name__, template_folder='templates')

class LucidaServiceHandler:
    implements(LucidaService.Iface)
 
    def __init__(self):
        self.log = {}
 
    def create(self, LUCID, spec):
        print LUCID, 'Create: port', spec.name
        return
 
    def learn(self, LUCID, knowledge):
        print LUCID, 'Learn'
        return      
     
    def infer(self, LUCID, query): 
        print LUCID, 'Infer'
        return 'Lucida!!!!!!!'
    
@app.route("/")
def home():
    return "I am Lucida"

def threaded_function():
    handler = LucidaServiceHandler()
    processor = LucidaService.Processor(handler)
    transport = TSocket.TServerSocket(port=8080)
    tfactory = TTransport.TBufferedTransportFactory()
    pfactory = TBinaryProtocol.TBinaryProtocolFactory()
    server = TNonblockingServer.TNonblockingServer(processor, transport, pfactory, pfactory)
    print 'CMD at', str(8080)
    server.serve()
    
     
if __name__ == '__main__':
    thread = Thread(target = threaded_function)
    thread.start()
    app.run(host='0.0.0.0', port=3000, debug=True, use_reloader=False)


