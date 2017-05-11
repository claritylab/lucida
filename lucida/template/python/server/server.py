#!/usr/bin/env python

import sys
sys.path.append('../')

from TemplateConfig import*

from lucidatypes.ttypes import QuerySpec
from lucidaservice import LucidaService

from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol
from thrift.server import TServer

# TODO: Adding modules your services needed

class TemplateHandler(LucidaService.Iface):
    def create(self, LUCID, spec):
        # TODO: Adding your own infer function. Check the top-level README to 
        # figure out each parameter represents.
        # For the template, do nothing
        return
        
    def learn(self, LUCID, knowledge):
        # TODO: Adding your own learn function. Check the top-level README to 
        # figure out each parameter represents.
        # For the template, do nothing
        return

    def infer(self, LUCID, query):
        # TODO: Adding your own infer function. Check the top-level README to 
        # figure out each parameter represents.
        # For the template, print the query info and return "Anwser is XXX"
        print("@@@@@ Infer; User: " + LUCID)
        if len(query.content) == 0 or len(query.content[0].data) == 0:
            return "error: incorrect query"
        query_data = query.content[0].data[-1]
        print("Asking: " + query_data)
        answer_data = "This is the sample answer"
        print("Result: " + answer_data)    
        return answer_data

# Set handler to our implementation and setup the server
handler = TemplateHandler()
processor = LucidaService.Processor(handler)
transport = TSocket.TServerSocket(port=PORT)
tfactory = TTransport.TFramedTransportFactory()
pfactory = TBinaryProtocol.TBinaryProtocolFactory()
server = TServer.TSimpleServer(processor, transport, tfactory, pfactory)

# Display useful information on the command center and start the server
# Change 'XXX' into your service's acronym
print 'XXX at port %d' % PORT
server.serve()
