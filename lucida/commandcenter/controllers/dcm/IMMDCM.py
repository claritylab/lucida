import abc
from Decision import*

not_monument = 'cow'

class IMMDCM(Decision):

    def logic_method(self, response_data, service_graph, dcm_node):
        # Check if image was recognized as a monument (not a cow)
        # If not, ask user for more specific information
        # Otherwise, send response from IMM (name of monument) to QA
        if not_monument in response_data['text'][-1]:
            self.lucida_response ='Cannot determine location of image. Please give me another image.'
            self.next_node = service_graph.get_next_index(dcm_node, 'IMM')
        else:
            response_data['text'][-1] = 'Where is the location of ' + response_data['text'][-1]
            self.lucida_response = ''
            self.next_node = service_graph.get_next_index(dcm_node, 'QA')
        return
