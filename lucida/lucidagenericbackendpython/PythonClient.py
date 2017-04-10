#!/usr/bin/env python

###################################################################
#  BEWARE: Synchronous client, won't work on Asynchronous server  #
#          PythonServer.py is asynchronous !!!                    #
###################################################################

import os, sys
sys.path.append('./gen-py')

from lucidaservice import LucidaService
from lucidaservice.ttypes import *
from lucidatypes.ttypes import *

from thrift import Thrift
from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol

try:

    # Make socket
    transport = TSocket.TSocket('localhost', 9092)

    # Buffering is critical. Raw sockets are very slow
    transport = TTransport.TBufferedTransport(transport)

    # Wrap in a protocol
    protocol = TBinaryProtocol.TBinaryProtocol(transport)

    # Create a client to use the protocol encoder
    client = LucidaService.Client(protocol)

    # Connect
    transport.open()

    exit = 0
    while not exit:
        # os.system('clear')
        print "Please enter a question:"
        input = raw_input(" >>  ")
        output = client.infer(input, QuerySpec())
        print 'Answer: %s' % (output)

    # Close
    transport.close()

except Thrift.TException, tx:
    print '%s' % (tx.message)