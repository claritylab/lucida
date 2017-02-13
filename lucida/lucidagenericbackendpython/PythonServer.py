from lucidaservice import LucidaService
from lucidaservice.ttypes import *
from lucidatypes.ttypes import *

import sys, time
sys.path.append('../gen-py')

from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol
# For synchronous connections
# from thrift.server import TServer

from thrift.server import TNonblockingServer


class LucidaHandler:
    # Generic asynchronous job not following Lucida convention
    def asynchronousjob(self):
        print 'Async job started.'
        time.sleep(5)
        print 'Async job completed.'

    def create(self, LUCID, spec):
        print 'Create'

    def learn(self, LUCID, knowledge):
        print 'Learn'

    def infer(self, LUCID, query):
        print 'Received question: ' + LUCID
        return 'No.'


# Handler for Lucida requests
handler = LucidaHandler()

processor = LucidaService.Processor(handler)
transport = TSocket.TServerSocket(port=9092)
tfactory = TTransport.TBufferedTransportFactory()
pfactory = TBinaryProtocol.TBinaryProtocolFactory()

# For synchronous connections
# server = TServer.TThreadedServer(processor, transport, tfactory, pfactory)
server = TNonblockingServer.TNonblockingServer(processor, transport, pfactory, threads=1)

print 'Starting server...'
server.serve()