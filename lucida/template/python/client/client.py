import sys
sys.path.append('../')

from lucidatypes.ttypes import QueryInput, QuerySpec
from lucidaservice import LucidaService

from thrift import Thrift
from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol

if len(sys.argv) != 2:
	print('Wrong arguments!')
	exit(1)
PORT = int(sys.argv[1])

# TODO: Adding your own sample query
LUCID = "Clinc"
query_input_data = "What is the sample query?"
query_input = QueryInput(type="query", data=[query_input_data])
query_spec = QuerySpec(content=[query_input])

# Initialize thrift objects
transport = TTransport.TFramedTransport(TSocket.TSocket("localhost", PORT))
protocol = TBinaryProtocol.TBinaryProtocol(transport)
client = LucidaService.Client(protocol)

transport.open()
print "///// Connecting to template... /////"
results = client.infer(LUCID, query_spec)
print "///// Result: /////"
print "%s" % results
transport.close()