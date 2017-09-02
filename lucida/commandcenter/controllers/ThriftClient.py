from lucidatypes.ttypes import QueryInput, QuerySpec
from lucidaservice import LucidaService
from dcm import*
from flask import*

from Config import  WFList

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


import threading





#This is basically the thread starter function
class FuncThread(threading.Thread):
    def __init__(self, target, *args):
        self._target = target
        self._args = args
        threading.Thread.__init__(self)
    def run(self):
        self._target(*self._args)
 


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
        print (host,port)
        transport = TTransport.TFramedTransport(TSocket.TSocket(host, port))
        protocol = TBinaryProtocol.TBinaryProtocol(transport)
        transport.open()
        return LucidaService.Client(protocol), transport

    def send_query(self, LUCID, service_name, query_input_list):
        query_spec = self.create_query_spec('query', query_input_list)
        service = self.SERVICES[service_name]
        client, transport = self.get_client_transport(service)
        log('Sending infer request to ' + service.name)
        result = client.infer(str(LUCID), query_spec)
        transport.close()
        return result


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
            
            
            # Example usage
    def executeThreadServiceRequest(self,service_name, inputData, LUCID, threadIDValue):
		print("Thread ", threadIDValue, "executing", service_name, "with input", inputData)
		service = self.SERVICES[service_name]
		host, port = service.get_host_port()
		tag_list = [host, str(port)]
		query_input_list = [self.create_query_input(service.input_type, inputData, tag_list)]
		resultText = self.send_query(LUCID, service_name, query_input_list)
		self.threadResults.insert(threadIDValue, resultText)
		
 


# TODO: split function into separate functions (DCM, creating QuerySpec)
    def infer(self, LUCID, workflow_name, text_data, image_data):


        response_data = { 'text': text_data, 'image': image_data }
        self.threadResults = []

		# workflow_name contains the name of the workflow, NOT the microservice.
		# This acquires the workflow class.
        workflow = WFList[workflow_name]
        workflow.__init__()
        resultText = response_data['text']
        resultImage = [response_data['image']]


        while not workflow.isEnd:
			
			i = 0
			for x in resultText:
				resultText[i] = [unicode(resultText)] # Text information must be unicode'd and array'd to be properly passed. IMAGE DATA DOES NOT HAVE THIS DONE TO IT.
				i+= 1
				
			# Processes the current workflow state, and in the process finds if this is the final stage or if next stage exists.
			workflow.processCurrentState(resultText,resultImage)

			resultText = []
			resultImage = []
			self.threadResults = []
			threadList = []
			threadID = 0
			#This is where batched execution initalizes and begins
			for x in workflow.batchedData:
				print "_____Thread" + str(threadID) + "," +  str(x.serviceName) + "," + str(x.argumentData)
				#Execute the desired microservice
				threadList.append(FuncThread(self.executeThreadServiceRequest, x.serviceName, x.argumentData, LUCID,threadID))
				threadList[threadID].start()
				threadID+=1

			threadID = 0
			#This is where batched execution joins together
			for x in workflow.batchedData:
				threadList[threadID].join()
				print "============ThreadID" + str(threadID)
				print "Output:" + self.threadResults[threadID]
				resultText.insert(threadID, self.threadResults[threadID])
				threadID+=1



                
        return resultText[0]

thrift_client = ThriftClient(Config.SERVICES)
