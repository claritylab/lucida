class Decision(object):
    def class_WE_DCM (self, response_data, service_graph, dcm_node):
        next_node = None
        lucida_response = ''
        # Check if weather was determined
        # If not, ask user for more specific information then call WE
        if 'No weather found' in response_data['text'][-1]:
            lucida_response = 'Please state the city and state you are asking the weather for.'
            next_node = service_graph.get_next_index(dcm_node, 'WE')
        return next_node, lucida_response

    def class_IMM_DCM (self, response_data, service_graph, dcm_node):
        next_node = None
        lucida_response = ''
        # Check if image was recognized as a monument (not a cow)
        # If not, ask user for more specific information
        # Otherwise, send response from IMM (name of monument) to QA
        if 'cow' in response_data['text'][-1]:
            lucida_response ='Cannot determine location of image. Please give me another image.'
            next_node = service_graph.get_next_index(dcm_node, 'IMM')
        else:
            response_data['text'][-1] = 'Where is the location of ' + response_data['text'][-1]
            next_node = service_graph.get_next_index(dcm_node, 'QA')
        return next_node, lucida_response
