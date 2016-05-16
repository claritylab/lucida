from lucidatypes.ttypes import QueryInput, QuerySpec
from lucidaservice import LucidaService

from thrift import TTornado
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol

from tornado import gen
from tornado import ioloop

from ConcurrencyManagement import services_lock, log


class ThriftClient(object):
	services = {}
	IMM = 'IMM'
	
	@staticmethod
	def add_service(name, host, port):
		# Check service name.
		if not name.isupper():
			raise ValueError('Service name must be in upper case')
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
			raise KeyError(name + ' has not been registered yet')
		host, port = ThriftClient.services[name][0] # (host, port)
		# Rotate the list to balance the load of back-end servers.
		ThriftClient.services[name].pop(0)
		ThriftClient.services[name].append((host, port))
		services_lock.release()
		return host, port

	@staticmethod
	@gen.coroutine
	def learn_image_callback(LUCID, knowledge):
		host, port = ThriftClient.get_service(ThriftClient.IMM)
		# Create a client based on host and port.
		transport = TTornado.TTornadoStreamTransport(host, port)
		pfactory = TBinaryProtocol.TBinaryProtocolFactory()
		client = LucidaService.Client(transport, pfactory)
		# Open the transport.
		try:
			yield transport.open()
		except TTransport.TTransportException as ex:
			log(ex)
			raise gen.Return()
		# Call learn.
		log('Sending learn_image request to IMM')
		yield client.learn(str(LUCID), knowledge)
		# Close the transport.
		client._transport.close()
		raise gen.Return()
	
	@staticmethod
	@gen.coroutine
	def infer_image_callback(LUCID, query):
		host, port = ThriftClient.get_service(ThriftClient.IMM)
		# Create a client based on host and port.
		transport = TTornado.TTornadoStreamTransport(host, port)
		pfactory = TBinaryProtocol.TBinaryProtocolFactory()
		client = LucidaService.Client(transport, pfactory)
		# Open the transport.
		try:
			yield transport.open()
		except TTransport.TTransportException as ex:
			log(ex)
			raise gen.Return()		
		# Call infer.
		log('Sending infer_image request to IMM')
		result = yield client.infer(str(LUCID), query)
		# Close the transport.
		client._transport.close()
		raise gen.Return(result)

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
		ioloop.IOLoop.current().run_sync(
		lambda : ThriftClient.learn_image_callback(LUCID, knowledge))
	
	@staticmethod    
	def infer_image(LUCID, data):
		query_input = QueryInput()
		query_input.data = []
		query_input.data.append(str(data))
		query = QuerySpec()
		query.content = []
		query.content.append(query_input)
		result = ioloop.IOLoop.current().run_sync(
		lambda : ThriftClient.infer_image_callback(LUCID, query))
		return result
		
		