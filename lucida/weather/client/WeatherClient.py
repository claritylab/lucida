#!/usr/bin/env python

import sys
sys.path.append('../')

from WeatherConfig import PORT
from lucidatypes.ttypes import QueryInput, QuerySpec
from lucidaservice import LucidaService

from thrift import Thrift
from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol

LUCID = "Clinc"
query_input_data = "What's the weather in Ann Arbor, MI?"
query_input = QueryInput(type="query", data=[query_input_data])
query_spec = QuerySpec(content=[query_input])

# Initialize thrift objects
transport = TTransport.TFramedTransport(TSocket.TSocket("localhost", PORT))
protocol = TBinaryProtocol.TBinaryProtocol(transport)
client = LucidaService.Client(protocol)

transport.open()
print "/////....Connecting to Weather..../////"
results = client.infer(LUCID, query_spec)
print "/////Result/////"
print "%s" % results
transport.close()
