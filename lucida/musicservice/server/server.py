#!/usr/bin/env python
from __future__ import print_function

import sys
sys.path.append('../')

from pygn import Pygn
from MusicConfig import *
from helper import *

from lucidatypes.ttypes import QuerySpec
from lucidaservice import LucidaService

from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol
from thrift.server import TServer

import socket
import json
import re

class MusicHandler:
	def create(self, LUCID, spec):
		'''
		Do nothing
		'''
		return 

	def learn(self, LUCID, knowledge):
		'''
		Do nothing
		'''
		return

	def infer(self, LUCID, query):
		# Error handling of bad input
		if len(query.content) == 0:
			return 'Sorry, try another question'
		if len(query.content[0].data) == 0:
			return 'Sorry, try another question'

		question = query.content[0].data[-1]
		# Partition the question into words list
		keyword = keyword_scan(question)
		moodid = ''
		output = ''
		ret = ''
		if keyword == "":
			ret = 'Sorry, cannot find suitable songs'
			return ret
		else:
			moodid = mood_dic[keyword]
		# Example how to create a radio playlist by artist and track
		tracks = Pygn.createRadio(clientID, userID, artist='', track='', mood=moodid, popularity ='1000', similarity = '1000', count = '2')
		if tracks == None:
			output = 'Sorry, cannot find suitable songs'
		for track in tracks:
			output += 'Track title: ' + track['track_title'] + '\nArtist: ' + track['album_artist_name']
		ret = output
		print(ret)
		return ret

# Set the handler to our implement
handler = MusicHandler()
processor = LucidaService.Processor(handler)
transport = TSocket.TServerSocket(port=PORT)
tfactory = TTransport.TFramedTransportFactory()
pfactory = TBinaryProtocol.TBinaryProtocolFactory()
server = TServer.TSimpleServer(processor, transport, tfactory, pfactory)

print("MUSIC at port %d" % PORT)
server.serve()
print("done!")
