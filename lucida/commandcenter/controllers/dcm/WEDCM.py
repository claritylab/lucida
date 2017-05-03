import abc
from Decision import*

class WEDCM(Decision):

    def logic_method(self, response_data, service_graph, dcm_node):
        # Check if weather was determined
        # If not, ask user for more specific information then call WE
        if 'No weather found' in response_data['text'][-1]:
            self.lucida_response = 'Please state the city and state you are asking the weather for.'
            self.next_node = service_graph.get_next_index(dcm_node, 'WE')
        else:
            self.lucida_response = ''
            self.next_node = None
        return
