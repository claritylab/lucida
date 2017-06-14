"""
This a template for how to add your own microservice into Lucida interface
"""

# Server Port number (necessary for every microservice server)
import ConfigParser, sys

class FakeSecHead(object):
    def __init__(self, fp):
        self.fp = fp
        self.sechead = '[asection]\n'

    def readline(self):
        if self.sechead:
            try: 
                return self.sechead
            finally: 
                self.sechead = None
        else: 
            return self.fp.readline()

cp = ConfigParser.SafeConfigParser()
cp.readfp(FakeSecHead(open("../../config.properties")))
port_dic = dict(cp.items('asection'))
PORT = int(port_dic['xxx_port'])

# TODO: Other configuration for your own service