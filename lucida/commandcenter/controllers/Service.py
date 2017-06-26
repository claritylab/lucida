import os
from Utilities import log

class Service(object):
    LEARNERS = { 'audio' : [], 'image' : [], 'text' : [] }
    # Constructor.
    def __init__(self, name, input_type, learn_type, num, host_port):
        self.name = name
        self.num = num
        self.count = 0
        if not (input_type == 'text' or input_type == 'image'):
            print 'Can only process text and image'
            exit()
        self.input_type = input_type
        if not learn_type is None:
            if not learn_type in Service.LEARNERS:
                print 'Unrecognized learn_type'
                exit()
            Service.LEARNERS[learn_type].append(self)
        self.host_port = host_port

    def get_host_port(self):
        try:
            if self.num <= 0:
                raise RuntimeError('No available instance for service ' + self.name)
            cur_host = self.host_port[self.count]['host']
            cur_port = self.host_port[self.count]['port']
            self.count = (self.count + 1)%self.num
            log('loop ' + str(self.count))
            return cur_host, cur_port
        except Exception:
            raise RuntimeError('Cannot access service ' + self.name)

