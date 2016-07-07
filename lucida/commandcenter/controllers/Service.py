import os
from Utilities import log

class Service(object):
	LEARNERS = { 'audio' : [], 'image' : [], 'text' : [] } 
	# Constructor.
	def __init__(self, name, port, input_type, learn_type):
		self.name = name
		self.port = port
		if not (input_type == 'text' or input_type == 'image'):
			print 'Can only process text and image'
			exit()
		self.input_type = input_type
		if not learn_type is None:
			if not learn_type in Service.LEARNERS:
				print 'Unrecognized learn_type'
				exit()
			Service.LEARNERS[learn_type].append(self)
			
	def get_host_port(self):
		try:
			host = 'localhost'
			tcp_addr = os.environ.get(
				self.name + '_PORT_' + str(self.port) + '_TCP_ADDR')
			if tcp_addr:
				log('TCP address is resolved to ' + tcp_addr)
				host = tcp_addr
			return host, self.port
		except Exception:
			raise RuntimeError('Cannot access service ' + self.name)		
			
			