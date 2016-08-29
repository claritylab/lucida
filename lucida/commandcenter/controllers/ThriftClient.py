from lucidatypes.ttypes import QueryInput, QuerySpec
from lucidaservice import LucidaService

from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol

from Utilities import log
from Database import database
import Config
import os
import sys
reload(sys)  
sys.setdefaultencoding('utf8') # to solve the unicode error


class ThriftClient(object):	
	# Constructor.
	def __init__(self, SERVICES):
		self.SERVICES = SERVICES
		log('Pre-configured services: ' + str(SERVICES))
	
	def create_query_input(self, type, data, tag_list):
		query_input = QueryInput()
		query_input.type = type
		query_input.data = []
		query_input.data.append(str(data))
		query_input.tags = tag_list
		return query_input
	
	def create_query_spec(self, name, query_input_list):
		query_spec = QuerySpec()
		query_spec.name = name
		query_spec.content = query_input_list
		return query_spec	
	
	def get_client_transport(self, service):
		host, port = service.get_host_port()
		transport = TTransport.TFramedTransport(TSocket.TSocket(host, port))
		protocol = TBinaryProtocol.TBinaryProtocol(transport)
		transport.open()
		return LucidaService.Client(protocol), transport

	def learn_image(self, LUCID, image_type, image_data, image_id):
		for service in Config.Service.LEARNERS['image']: # add concurrency?
			knowledge_input = self.create_query_input(
				image_type, image_data, [image_id])
			client, transport = self.get_client_transport(service)
			log('Sending learn_image request to IMM')
			client.learn(str(LUCID), 
				self.create_query_spec('knowledge', [knowledge_input]))
			transport.close()
	
	def learn_text(self, LUCID, text_type, text_data, text_id):
		for service in Config.Service.LEARNERS['text']: # add concurrency?
			knowledge_input = self.create_query_input(
				text_type, text_data, [text_id])
			client, transport = self.get_client_transport(service)
			log('Sending learn_text request to QA')
			client.learn(str(LUCID), 
				self.create_query_spec('knowledge', [knowledge_input]))
			transport.close()

	def infer(self, LUCID, service_graph, text_data, image_data):
		# Create the list of QueryInput.
		query_input_list = []
		for node in service_graph.node_list:
			service = self.SERVICES[node.service_name]
			data = text_data if service.input_type == 'text' else image_data
			host, port = service.get_host_port()
			tag_list = [host, str(port), str(len(node.to_indices))]
			for to_index in node.to_indices:
				tag_list.append(str(to_index))
			query_input_list.append(self.create_query_input(
				service.input_type, data, tag_list))
		query_spec = self.create_query_spec('query', query_input_list)
		# Go through all starting indices and send requests.
		result = []
		for start_index in service_graph.starting_indices:
			service = self.SERVICES[service_graph.get_node(
				start_index).service_name]
			client, transport = self.get_client_transport(service)
			log('Sending infer request to ' + service.name)
			result.append(client.infer(str(LUCID), query_spec))
			transport.close()
		return ' '.join(result)


thrift_client = ThriftClient(Config.SERVICES)	
