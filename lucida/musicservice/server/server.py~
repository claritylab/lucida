#!/usr/bin/env python
from __future__ import print_function

import socket
import sys
import json
import re
sys.path.append('../')

from MusicConfig import *

from lucidatypes.ttypes import QuerySpec
from lucidaservice import LucidaService


from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol
from thrift.server import TServer

# import pygn library
from pygn import Pygn


class MusicHandler:
	def create(self, LUCID, spec):
		# Do nothing
		return 

	def learn(self, LUCID, knowledge):
		# Do nothing
		return

	def infer(self, LUCID, query):
		question = query.content[0].data[-1]
		# Partition the question into words list
		keyword = keyword_scan(question)
		moodid = ''
		output = ''
		ret = ''
		if keyword == "":
			ret = 'Received: No available keywords in databases'
			return ret
		else:
			moodid = mood_dic[keyword]
		# Example how to create a radio playlist by artist and track
		results = Pygn.createRadio(clientID, userID, artist='', track='', mood=moodid, popularity ='1000', similarity = '1000', count = '2')
		if results == None:
			output = 'No available tracks in databases'
		for result in results:
			output += 'Track title: ' + result['track_title'] + '\nArtist: '+result['album_artist_name']
		ret = output
		print(ret)
		return ret

handler = MusicHandler()
processor = LucidaService.Processor(handler)
transport = TSocket.TServerSocket(port=PORT)
tfactory = TTransport.TFramedTransportFactory()
pfactory = TBinaryProtocol.TBinaryProtocolFactory()

server = TServer.TSimpleServer(processor, transport, tfactory, pfactory)

print("MUSIC at port %d" % PORT)
server.serve()
print("done!")
