import sys, glob  
sys.path.insert(0, glob.glob('/home/yba/Documents/clarity/fbthrift/thrift/lib/py/build/lib*')[0]) # This needs to be more generic.

from lucidatypes.ttypes import *
from lucidaservice import LucidaService

from twisted.internet.defer import inlineCallbacks  
from twisted.internet import reactor  
from twisted.internet.protocol import ClientCreator  

from thrift import Thrift
from thrift.transport import TTwisted  
from thrift.protocol import TBinaryProtocol  

@inlineCallbacks  
def main(client):
    try:
        result = yield client.infer('Johann', QuerySpec())
        print result 
    except Exception as e:
        print e
    
if __name__ == '__main__':
    print 'CMD sending 7 requests to IMM at 8082'
    for x in xrange(0, 7):
        d = ClientCreator(reactor,  
        TTwisted.ThriftClientProtocol,  
        LucidaService.Client,  
        TBinaryProtocol.TBinaryProtocolFactory(),  
        ).connectTCP("127.0.0.1", 8082)  
        print x, 'Sending request to IMM'
        d.addCallback(lambda conn: conn.client)  
        d.addCallback(main)  
    
    reactor.run()  
