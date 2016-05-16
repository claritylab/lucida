from lucidatypes.ttypes import *
from lucidaservice import LucidaService

# from twisted.internet.defer import inlineCallbacks
# from twisted.internet import reactor
# from twisted.internet.protocol import ClientCreator  

# from thrift import Thrift
# from thrift.transport import TTwisted  
# from thrift.protocol import TBinaryProtocol

from thrift import TTornado
from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol

from ConcurrencyManagement import *



from tornado import gen
from tornado import ioloop



class ThriftClient(object):
	services = {}
	IMM = 'IMM'
	
	@staticmethod
	def add_service(name, host, port):
		if not name.isupper():
			raise ValueError('Service name must be in upper case')
		services_lock.acquire()
		if not name in ThriftClient.services:
			ThriftClient.services[name] = [(host, int(port))]
		else:
			ThriftClient.services[name].append((host, int(port)))
		services_lock.release()
		log('Registered services ' + str(ThriftClient.services))
	
	@staticmethod		
	def get_service(name):
		services_lock.acquire()
		if not name in ThriftClient.services:
			services_lock.release()
			raise KeyError(name + ' has not been registered yet')
		if not ThriftClient.services[name]:
			services_lock.release()
			raise KeyError(name + ' has been registered but no working server')
		rtn = ThriftClient.services[name][0] # (host, port)
		# Rotate the list to balance the load of back-end servers.
		ThriftClient.services[name].pop(0)
		ThriftClient.services[name].append(rtn)
		services_lock.release()
		return rtn
	
# 	@staticmethod
# 	@inlineCallbacks  
# 	def learn_image_callback(client, LUCID, knowledge):
# 		try:
# 			yield client.learn(LUCID, knowledge)
# 		except Exception as e:
# 			log(e)
# 		log("should return!!!!!!!!!!")
		
	@staticmethod
	@gen.coroutine
	def learn_image_callback2(LUCID, knowledge):
		log('$$$$$$$$$$')
		transport = TTornado.TTornadoStreamTransport('localhost', 8082)
		pfactory = TBinaryProtocol.TBinaryProtocolFactory()
		client = LucidaService.Client(transport, pfactory)
 		
		# open the transport, bail on error
		try:
			yield transport.open()
		except TTransport.TTransportException as ex:
			log(ex)
			raise gen.Return()
		
		yield client.learn(str(LUCID), knowledge)
 		
		# close the transport
		client._transport.close()
 		
		# close the transport
		client._transport.close()
		raise gen.Return()

	@staticmethod    
	def learn_image(LUCID, label, data):
# 		service_inst = ThriftClient.get_service(ThriftClient.IMM)
# 		log('CMD sending learn_image request to IMM at ' + str(service_inst))
# 		d = ClientCreator(reactor,  
# 		TTwisted.ThriftClientProtocol,  
# 		LucidaService.Client,  
# 		TBinaryProtocol.TBinaryProtocolFactory(),  
# 		).connectTCP(service_inst[0], service_inst[1])
		knowledge_input = QueryInput()
		knowledge_input.tags = []
		knowledge_input.tags.append(str(label))
		knowledge_input.data = []
		knowledge_input.data.append(str(data))
		knowledge = QuerySpec()
		knowledge.content = []
		knowledge.content.append(knowledge_input)
		ioloop.IOLoop.current().run_sync(lambda : ThriftClient.learn_image_callback2(LUCID, knowledge))
		