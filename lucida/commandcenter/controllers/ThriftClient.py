from lucidatypes.ttypes import QueryInput, QuerySpec
from lucidaservice import LucidaService

from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol

from ConcurrencyManagement import log
from Database import database
import Config
import os
import sys
reload(sys)  
sys.setdefaultencoding('utf8')


class ThriftClient(object):	
	# Constructor.
	def __init__(self, SERVICE_LIST_IN, LEARNERS_IN, SERVICES_IN):
		self.SERVICE_LIST = SERVICE_LIST_IN
		self.LEARNERS = LEARNERS_IN
		# Registered services.
		self.SERVICES = SERVICES_IN
		log('Pre-configured services: ' + str(SERVICES_IN))
	
	def create_query_input(self, type_in, data_in, tag_in):
		query_input = QueryInput()
		query_input.type = type_in
		query_input.data = []
		query_input.data.append(str(data_in))
		query_input.tags = []
		query_input.tags.append(str(tag_in))
		return query_input
	
	def create_query_spec(self, name_in, query_input_list):
		query_spec = QuerySpec()
		query_spec.name = name_in
		query_spec.content = query_input_list
		return query_spec	
		
	def get_service(self, name):
		try:
			host, port = self.SERVICES[name][0] # (host, port)
			# Rotate the list to balance the load of back-end servers.
			self.SERVICES[name].pop(0)
			self.SERVICES[name].append((host, port))
			tcp_addr = os.environ.get(name + '_PORT_' + str(port) + '_TCP_ADDR')
			if tcp_addr:
				log('TCP address is resolved to ' + tcp_addr)
				host = tcp_addr
			else:
				host = 'localhost'
			return host, port
		except Exception:
			raise RuntimeError('Cannot access service ' + name)
	
	def get_client_transport(self, service_name):
		host, port = self.get_service(service_name)
		transport = TTransport.TFramedTransport(TSocket.TSocket(host, port))
		protocol = TBinaryProtocol.TBinaryProtocol(transport)
		transport.open()
		return LucidaService.Client(protocol), transport

	def learn_image(self, LUCID, label, image_data):
		for service_name in self.LEARNERS['image']: # add concurrency?
			knowledge_input = self.create_query_input('image',
													  image_data, label)
			client, transport = self.get_client_transport(service_name)
			log('Sending learn_image request to IMM')
			client.learn(str(LUCID), 
						 self.create_query_spec('knowledge', [knowledge_input]))
			transport.close()
	
	def learn_text(self, LUCID, text_data):
		for service_name in self.LEARNERS['text']: # add concurrency?
			knowledge_input = self.create_query_input('text', text_data, '')
			client, transport = self.get_client_transport(service_name)
			log('Sending learn_text request to QA')
			client.learn(str(LUCID), 
				self.create_query_spec('knowledge', [knowledge_input]))
			transport.close()
		
	def ask_ensemble(self, text_data):
		# I know hard-coding is not good, but this is due to the fact that
		# Falk implements his QA Ensemble differently.
		log('Asking ensemble')
		transport = TSocket.TSocket('ensemble', 9090)
		transport = TTransport.TBufferedTransport(transport)
		protocol = TBinaryProtocol.TBinaryProtocol(transport)
		client = LucidaService.Client(protocol)
		transport.open()
		result = client.infer(text_data, QuerySpec())
		transport.close()
		return result

	def infer(self, LUCID, services_needed, text_data, image_data):
		query_input_list = []
		i = 0
		for service_name in services_needed:
			input_type = self.SERVICE_LIST[service_name]
			data_in = ''
			tag_in = ''
			if input_type == 'text':
				data_in = text_data
			elif input_type == 'image':
				data_in = image_data
			else:
				raise RuntimeError('Can only process text and image data')
			if i != len(services_needed) - 1:
				host, port = self.get_service(services_needed[i + 1])
				tag_in = host + ', ' + str(port)	
			query_input_list.append(self.create_query_input(
				input_type, data_in, tag_in))
			i += 1
		# Check IMM.
		if services_needed[0] == 'IMM' and \
			database.count_images(str(LUCID)) == 0:
				raise RuntimeError('Cannot match in empty photo collection')
		client, transport = self.get_client_transport(services_needed[0])
		log('Sending infer request to ' + services_needed[0])
		result = client.infer(str(LUCID), self.create_query_spec(
			'query', query_input_list))
		transport.close()
		if 'Factoid not found in knowledge base.' in result:
			result = self.ask_ensemble(text_data)
		return result


thrift_client = ThriftClient(Config.SERVICE_LIST, Config.LEARNERS,
		Config.REGISTRERED_SERVICES)	
