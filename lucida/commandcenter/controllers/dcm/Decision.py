import abc

class Decision(object):
    __metaclass__ = abc.ABCMeta


    def __init__(self, next_node=None, lucida_response=''):
        self.next_node = next_node
        self.lucdia_repsonse = lucida_response

    @abc.abstractmethod
    def logic_method(self, response_data, service_graph, dcm_node):
        """
        Decision logic method.

        Arg:
            response_data: response text/image from previous services
            service_graph: current workflow
            dcm_node: current DCM node in workflow

        Return:
            lucida_response: response to send back to user
            next_node: next node to go to in workflow
        """

        return
