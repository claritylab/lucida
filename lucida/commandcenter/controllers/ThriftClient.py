from lucidatypes.ttypes import QueryInput, QuerySpec
from lucidaservice import LucidaService

from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol

from ConcurrencyManagement import services_lock, log

class ThriftClient(object):
	services = {}
	IMM = 'IMM'
	
	@staticmethod
	def add_service(name, host, port):
		# Check service name.
		if not name.isupper():
			raise RuntimeError('Service name must be in upper case')
		# Add (host, port) to services.
		services_lock.acquire()
		if not name in ThriftClient.services:
			ThriftClient.services[name] = [(host, int(port))]
		else:
			ThriftClient.services[name].append((host, int(port)))
		log('Registered services ' + str(ThriftClient.services))
		services_lock.release()
	
	@staticmethod		
	def get_service(name):
		services_lock.acquire()
		if not (name in ThriftClient.services and ThriftClient.services[name]):
			services_lock.release()
			raise RuntimeError(name + ' has not been registered yet')
		host, port = ThriftClient.services[name][0] # (host, port)
		# Rotate the list to balance the load of back-end servers.
		ThriftClient.services[name].pop(0)
		ThriftClient.services[name].append((host, port))
		services_lock.release()
		return host, port

	@staticmethod
	def get_client_transport(service_name):
		host, port = ThriftClient.get_service(service_name)
		transport = TTransport.TBufferedTransport(TSocket.TSocket(host ,port))
		protocol = TBinaryProtocol.TBinaryProtocol(transport)
		transport.open()
		return LucidaService.Client(protocol), transport

	@staticmethod    
	def learn_image(LUCID, label, data):
		knowledge_input = QueryInput()
		knowledge_input.tags = []
		knowledge_input.tags.append(str(label))
		knowledge_input.data = []
		knowledge_input.data.append(str(data))
		knowledge = QuerySpec()
		knowledge.content = []
		knowledge.content.append(knowledge_input)
		client, transport = ThriftClient.get_client_transport(ThriftClient.IMM)
		# Call learn.
		log('Sending learn_image request to IMM')
		client.learn(str(LUCID), knowledge)
		transport.close()
	
	@staticmethod    
	def infer_image(LUCID, data):
		query_input = QueryInput()
		query_input.data = []
		query_input.data.append(str(data))
		query = QuerySpec()
		query.content = []
		query.content.append(query_input)
		client, transport = ThriftClient.get_client_transport(ThriftClient.IMM)
		# Call learn.
		log('Sending infer_image request to IMM')
		result = client.infer(str(LUCID), query)
		transport.close()
		return result

		