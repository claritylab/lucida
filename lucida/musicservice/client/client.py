#!/usr/bin/env python

import sys
sys.path.append('../')

from MusicConfig import PORT
from lucidatypes.ttypes import QueryInput, QuerySpec
from lucidaservice import LucidaService

from thrift import Thrift
from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol

# Setup a template input query
LUCID = "Clinc"
query_input_data = "I want a happy song."
query_input = QueryInput(type="query", data=[query_input_data])
query_spec = QuerySpec(content=[query_input])

try:
  transport = TSocket.TSocket('localhost', PORT)
  transport = TTransport.TFramedTransport(transport)
  protocol = TBinaryProtocol.TBinaryProtocol(transport)
  client = LucidaService.Client(protocol)
  transport.open()

  # Test the server
  print query_input_data
  print "/////....Connecting to Musicservice.../////"
  msg = client.infer(LUCID, query_spec)
  print "/////Result/////\n", msg

  transport.close()

except Thrift.TException, ex:
  print "%s" % (ex.message)
