#!/usr/bin/python

"""
Save this file as server.py
>>> python server.py 0.0.0.0 8001
serving on 0.0.0.0:8001

or simply

>>> python server.py
Serving on localhost:8000

You can use this to test GET and POST methods.

"""

import SimpleHTTPServer
import SocketServer
import logging
import cgi

import sys, os
from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol

# service specifics
sys.path.append(os.environ.get('LUCIDAROOT')+'/learn/gen-py')
from parser_service import ParserService
from parser_service.ttypes import *

INDRI_PATH=os.getenv('INDRI_INDEX', '/opt/indri_db')
LEARN_PORT=int(os.getenv('DOCKER_LEARN', 8083))
LEARN_SERVER=int(os.getenv('DOCKER_LEARN_SERVER', 8080))

def create_learn(addr='localhost', port=LEARN_PORT):
  learn = {}
  # Make socket
  socket = TSocket.TSocket(addr, port)

  # Buffering is critical. Raw sockets are very slow
  learn['transport'] = TTransport.TFramedTransport(socket)
  learn['transport'].open()

  # Wrap in a protocol
  protocol = TBinaryProtocol.TBinaryProtocol(learn['transport'])

  # Create a client to use the protocol encoder
  learn['client'] = ParserService.Client(protocol)

  return learn

class ServerHandler(SimpleHTTPServer.SimpleHTTPRequestHandler):

    def do_GET(self):
        logging.warning("======= GET STARTED =======")
        logging.warning(self.headers)
        SimpleHTTPServer.SimpleHTTPRequestHandler.do_GET(self)

    def do_POST(self):
        logging.warning("======= POST STARTED =======")
        logging.warning(self.headers)
        form = cgi.FieldStorage(
            fp=self.rfile,
            headers=self.headers,
            environ={'REQUEST_METHOD':'POST',
                     'CONTENT_TYPE':self.headers['Content-Type'],
                     })
        logging.warning("======= POST VALUES =======")
        for item in form.list:
            logging.warning(item)
            # If it is the form we want
            if item.name == "facts":
                learn = create_learn()
                with open("fact.txt", "w") as fp:
                  fp.write(item.value)
                cur_dir=os.environ.get('LUCIDAROOT')+'/learn/html_server/'
                learn['client'].parseThrift(INDRI_PATH, [], [cur_dir+'fact.txt'])
                self.send_response(301)
                self.send_header('Location','success.html')
                self.end_headers()
        logging.warning("\n")
        SimpleHTTPServer.SimpleHTTPRequestHandler.do_GET("success.html")

Handler = ServerHandler

httpd = SocketServer.TCPServer(("", LEARN_SERVER), Handler)

httpd.serve_forever()
