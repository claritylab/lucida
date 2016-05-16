from lucidatypes.ttypes import *
from lucidaservice import LucidaService
from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol
from thrift.server import TNonblockingServer

from ThriftClient import ThriftClient
from ConcurrencyManagement import log

class LucidaServiceHandler(LucidaService.Iface):
 
    def __init__(self):
        self.log = {}
 
    def create(self, LUCID, spec):
        log('Create ' + spec.content[0].type
             + ' at host ' + spec.content[0].data[0]
             + ' port ' + spec.content[0].tags[0])
        ThriftClient.add_service(spec.content[0].type, spec.content[0].data[0], spec.content[0].tags[0])
        return
 
    def learn(self, LUCID, knowledge):
        return      
     
    def infer(self, LUCID, query): 
        return 'CMD can only infer'