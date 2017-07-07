import os
from Utilities import log

class Service(object):
    # Constructor.
    def __init__(self, name, input_type, learn_type, num, instance):
        self.name = name
        self.count = 0
        if not (input_type == 'text' or input_type == 'image'):
            print 'Can only process text and image'
            exit()
        self.input_type = input_type
        self.learn_type = learn_type
        self.instance = instance
        self.num = len(self.instance)

    def get_host_port(self):
        try:
            if self.num <= 0:
                raise RuntimeError('No available instance for service ' + self.name)
            # check if uninitialized
            cur_name = ''
            for i in range(self.num):
                cur_name = self.instance[self.count]['name']
                if cur_name == '':
                    self.count = (self.count + 1)%self.num
                else:
                    break
            # all instance unintialized
            if cur_name == '':
                raise RuntimeError('No available instance for service ' + self.name)
            cur_host = self.instance[self.count]['host']
            cur_port = self.instance[self.count]['port']
            self.count = (self.count + 1)%self.num
            log('loop ' + str(self.count))
            return cur_host, cur_port
        except Exception:
            raise RuntimeError('Cannot access service ' + self.name)

    def get_host_port_withid(self, instance_id):
        for obj in self.instance:
            if obj['id'] == instance_id:
                return obj['host'], obj['port']


