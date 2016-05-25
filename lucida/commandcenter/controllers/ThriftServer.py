from lucidatypes.ttypes import *
from lucidaservice import LucidaService
from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol
from thrift.server import TNonblockingServer

from ConcurrencyManagement import log
from ThriftClient import thrift_client

class LucidaServiceHandler(LucidaService.Iface):
 
    def __init__(self):
        self.log = {}
 
    def create(self, LUCID, spec):
        try:
            log('Create ' + spec.content[0].type
                 + ' at host ' + spec.content[0].data[0]
                 + ' port ' + spec.content[0].tags[0])
            thrift_client.add_service(spec.content[0].type,
                                     spec.content[0].data[0],
                                    spec.content[0].tags[0])
        except Exception as e:
            log(e)
        return
 
    def learn(self, LUCID, knowledge):
        log('Only create should be invoked')
        return
     
    def infer(self, LUCID, query): 
        return 'Only create should be invoked'
