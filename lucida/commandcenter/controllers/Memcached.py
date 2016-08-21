from memcache import Client
from Utilities import log
import os


class Memcached(object):
	# Constructor.
	def __init__(self):
		memcached_addr = os.environ.get('MEMCACHED_PORT_11211_TCP_ADDR')
		if memcached_addr:
			log('Memcached: ' + memcached_addr)
			self.client = Client([(memcached_addr, 11211)])
		else:
			log('Memcached: localhost')
			self.client = Client([('127.0.0.1', 11211)])

memcached = Memcached()
