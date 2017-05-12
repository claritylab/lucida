from lucidatypes.ttypes import QueryInput, QuerySpec
from lucidaservice import LucidaService
from dcm import*
from flask import*

from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol

from Utilities import log
from Database import database
import Config
import os
import sys
reload(sys)
sys.setdefaultencoding('utf8') # to solve the unicode error

class ThriftClient(object):
    # Constructor.
    def __init__(self, SERVICES):
        self.SERVICES = SERVICES
        log('Pre-configured services: ' + str(SERVICES))

    def create_query_input(self, type, data, tag_list):
        query_input = QueryInput()
        query_input.type = type
        query_input.data = data
        query_input.tags = tag_list
        return query_input

    def create_query_spec(self, name, query_input_list):
        query_spec = QuerySpec()
        query_spec.name = name
        query_spec.content = query_input_list
        return query_spec

    def get_client_transport(self, service):
        host, port = service.get_host_port()
        transport = TTransport.TFramedTransport(TSocket.TSocket(host, port))
        protocol = TBinaryProtocol.TBinaryProtocol(transport)
        transport.open()
        return LucidaService.Client(protocol), transport

    def send_query(self, LUCID, service_graph, start_index, query_input_list):
        query_spec = self.create_query_spec('query', query_input_list)
        service = self.SERVICES[service_graph.get_node(start_index).service_name]
        client, transport = self.get_client_transport(service)
        log('Sending infer request to ' + service.name)
        result = client.infer(str(LUCID), query_spec)
        transport.close()
        return result

# TODO: If SESSION is within its own file move this function there
    def clear_context(self, LUCID, service_graph):
        if LUCID in Config.SESSION:
            # Initialize workflow back to the beginning
            service_graph.start_index = 0
            del Config.SESSION[LUCID]

    def learn_image(self, LUCID, image_type, image_data, image_id):
        for service in Config.Service.LEARNERS['image']: # add concurrency?
            knowledge_input = self.create_query_input(
                image_type, [image_data], [image_id])
            client, transport = self.get_client_transport(service)
            log('Sending learn_image request to IMM')
            client.learn(str(LUCID),
                self.create_query_spec('knowledge', [knowledge_input]))
            transport.close()

    def learn_text(self, LUCID, text_type, text_data, text_id):
        for service in Config.Service.LEARNERS['text']: # add concurrency?
            knowledge_input = self.create_query_input(
                text_type, [text_data], [text_id])
            client, transport = self.get_client_transport(service)
            log('Sending learn_text request to QA')
            client.learn(str(LUCID),
                self.create_query_spec('knowledge', [knowledge_input]))
            transport.close()

# TODO: split function into separate functions (DCM, creating QuerySpec)
    def infer(self, LUCID, service_graph, text_data, image_data):
        # Create the list of QueryInput.
        query_input_list = []
        response_data = { 'text': text_data, 'image': image_data }
        end_of_workflow = False
        start_index = service_graph.start_index
        node = service_graph.get_node(start_index)
        while not end_of_workflow:
            service = self.SERVICES[node.service_name]
            # Worker Service nodes
            if service.name == "DCM":
                # Call DAG until DCM node
                result = self.send_query(LUCID, service_graph, start_index, query_input_list)
                response_data['text'].append(result)
                # Make a decision using the DCM logic method
                service.decision.logic_method(response_data, service_graph, node)
                start_index = service.decision.next_node
                # If no start/next index, display latest response to user
                if start_index == None:
                    # Remove saved state/context for user
                    self.clear_context(LUCID, service_graph)
                    return response_data['text'][-1]
                # If lucida response, gather more info from user
                elif service.decision.lucida_response:
                    Config.SESSION[LUCID] = {'graph': service_graph, 'data': response_data }
                    service_graph.start_index = start_index
                    return service.decision.lucida_response
                # Set up to go to the next service
                else:
                    node = service_graph.get_node(start_index)
                    query_input_list = []
            # Service nodes
            else:
                # Create QueryInput list
                data = response_data['text'] if service.input_type == 'text' \
                        else response_data['image']
                host, port = service.get_host_port()
                tag_list = [host, str(port)]
                # Only Worker Service nodes can have multiple to_indices
                if len(node.to_indices) > 1:
                   print 'Invalid to_indices for Node ' + node.service_name
                   exit()
                elif len(node.to_indices) == 1:
                    to_index = node.to_indices[0]
                    node = service_graph.get_node(to_index)
                    # If node points to DCM, end the path
                    if 'DCM' in node.service_name:
                        tag_list.append('0')
                    else:
                        tag_list.append('1')
                        tag_list.append(str(to_index))
                else:
                    tag_list.append('0')
                    end_of_workflow = True
                query_input_list.append(self.create_query_input(
                    service.input_type, data, tag_list))
        # Send last QuerySpec
        # Remove saved state/context for user
        self.clear_context(LUCID, service_graph)
        result = self.send_query(LUCID, service_graph, start_index, query_input_list)
        return result

thrift_client = ThriftClient(Config.SERVICES)
