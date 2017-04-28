import abc
from Decision import*

class WEDCM(Decision):
    """
    Decision class for WE decision making nodes.
    """

    def logic_method(self, response_data, service_graph, dcm_node):
        """
        Decision logic for WEDCM. Decides whether the response from WE
        is the weather or not (asks the user again) and eventually, gets
        the weather for the place.

        Arg:
            response_data: response text/image from previous services
            service_graph: current workflow
            dcm_node: current DCM node in workflow

        Return:
            lucida_response: response to send back to user
            next_node: next node to go to in workflow
        """
        # Check if weather was determined
        # If not, ask user for more specific information then call WE
        if 'No weather found' in response_data['text'][-1]:
            self.lucida_response = 'Please state the city and state you are asking the weather for.'
            self.next_node = service_graph.get_next_index(dcm_node, 'WE')
        else:
            self.lucida_response = ''
            self.next_node = None
        return
