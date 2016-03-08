import sys, os
from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol

# service specifics
sys.path.append(os.environ.get('LUCIDAROOT')+'/learn/gen-py')
from parser_service import ParserService
from parser_service.ttypes import *

def create_learn(addr='localhost', port=8083):
  learn = {}
  # Make socket
  socket = TSocket.TSocket(addr, port)

  # Buffering is critical. Raw sockets are very slow
  learn['transport'] = TTransport.TFramedTransport(socket)
  learn['transport'].open()

  # Wrap in a protocol
  protocol = TBinaryProtocol.TBinaryProtocol(learn['transport'])

  # Create a client to use the protocol encoder
  learn['client'] = ParserService.Client(protocol)

  return learn

INDRI_PATH=os.getenv('INDRI_INDEX', '/opt/indri_db')
urls  = []
myfiles = ['facts.txt']
learn = create_learn()
print learn['client'].parseThrift(INDRI_PATH, urls, myfiles)
