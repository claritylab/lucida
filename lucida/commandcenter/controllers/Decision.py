import abc

class Decision(object):
    __metaclass__ = abc.ABCMeta

    def __init__(self, next_node=None, lucida_response=''):
        self.next_node = next_node
        self.lucdia_repsonse = lucida_response

    @abc.abstractmethod
    def logic_method(self, response_data, service_graph, dcm_node):
        """Decision making logic"""
        return
