from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals
 
import sys, glob  
sys.path.insert(0, glob.glob('/home/yba/Documents/clarity/fbthrift/thrift/lib/py/build/lib*')[0]) # This needs to be more generic.
 
from lucidatypes.ttypes import *
from lucidaservice import LucidaService
 
from zope.interface import implements
from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.transport import TTwisted
from thrift.protocol import TBinaryProtocol
from thrift.server import TNonblockingServer 
from thrift.server import TServer
from twisted.internet import reactor 


 
class LucidaServiceHandler:
    implements(LucidaService.Iface)
 
    def __init__(self):
        self.log = {}
 
    def create(self, LUCID, spec):
        print(LUCID + ' Create')
        return
 
    def learn(self, LUCID, knowledge):
        print(LUCID + ' Learn')
        return      
     
    def infer(self, LUCID, query): 
        print(LUCID + ' Infer')
        return 'Lucida!!!!!!!'
     
if __name__ == '__main__':
#     pfactory = TBinaryProtocol.TBinaryProtocolFactory()
#     server = reactor.listenTCP(8080,
#         TTwisted.ThriftServerFactory(processor,
#             pfactory), interface="127.0.0.1")
#     print('CMD at port 8080')
#     reactor.run()
#     print('Here')
#     
# 
#      
    handler = LucidaServiceHandler()
    processor = LucidaService.Processor(handler)
    transport = TSocket.TServerSocket(port=8080)
    tfactory = TTransport.TBufferedTransportFactory()
    #tfactory = TTransport.TBufferedTransportFactory()
    pfactory = TBinaryProtocol.TBinaryProtocolFactory()
    # set server
    #server = TServer.TSimpleServer(processor, transport, tfactory, pfactory)
    #server = TServer.TThreadedServer(processor, transport, tfactory, pfactory)
    #server = TServer.TThreadPoolServer(processor, transport, tfactory, pfactory)
    server = TNonblockingServer.TNonblockingServer(processor, transport, pfactory, pfactory)
    print('CMD at ' + str(8080))
    server.serve()

