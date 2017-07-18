from flask import*
import threading
import os
import sys

from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol

from lucidatypes.ttypes import QueryInput, QuerySpec
from lucidaservice import LucidaService
from Config import WFList
from Utilities import log
from Database import database
import Config

reload(sys)
sys.setdefaultencoding('utf8') # to solve the unicode error

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

    def get_client_transport(self, host, port):
        print (host,port)
        transport = TTransport.TFramedTransport(TSocket.TSocket(host, port))
        protocol = TBinaryProtocol.TBinaryProtocol(transport)
        transport.open()
        return LucidaService.Client(protocol), transport

    def send_query(self, LUCID, service_name, query_input_list):
        query_spec = self.create_query_spec('query', query_input_list)
        service = self.SERVICES[service_name]
        host = query_input_list[0].tags[0]
        port = int(query_input_list[0].tags[1])
        client, transport = self.get_client_transport(host, port)
        log('Sending infer request to ' + service.name)
        result = client.infer(str(LUCID), query_spec)
        transport.close()
        return result

    def learn_image(self, LUCID, image_type, image_data, image_id, _id):
        knowledge_input = self.create_query_input(
            image_type, [image_data], [image_id])
        service = Config.get_service_withid(_id)
        if service.num == 0:
            raise RuntimeError('No available instance to learn knowledge')
        for obj in service.instance:
            instance_id = obj['id']
            host, port = service.get_host_port_withid(instance_id)
            client, transport = self.get_client_transport(host, port)
            log('Sending learn_image request to ' + service.name)
            client.learn(str(LUCID),
                self.create_query_spec('knowledge', [knowledge_input]))
            transport.close()

    def learn_text(self, LUCID, text_type, text_data, text_id, _id):
        knowledge_input = self.create_query_input(
            text_type, [text_data], [text_id])
        service = Config.get_service_withid(_id)
        if service.num == 0:
            raise RuntimeError('No available instance to learn knowledge')
        for obj in service.instance:
            instance_id = obj['id']
            host, port = service.get_host_port_withid(instance_id)
            client, transport = self.get_client_transport(host, port)
            log('Sending learn_text request to ' + service.name)
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
	
    def infer(self, LUCID, workflow, text_data, image_data):

        response_data = { 'text': text_data, 'image': image_data }
        self.threadResults = []

		# workflow_name contains the name of the workflow, NOT the microservice.
		# This acquires the workflow class.
        resultText = text_data
        resultImage = image_data

        passArgs = dict()
        pause = False
        while not workflow.isEnd and not pause:
			batchedDataReturn = dict()
			print "-------------NEXT ITERATION:STATE" + str(workflow.currentState)
			resultText = [ unicode(x) for x in resultText ]

            # Processes the current workflow state, and in the process finds if this is the final stage or if next stage exists.
			print("Acquiring Batch Request")
			workflow.processCurrentState(1,batchedDataReturn,passArgs,resultText,resultImage)

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
				
			print("Executed batch request")

			threadID = 0
			#This is where batched execution joins together
			for x in workflow.batchedData:
				threadList[threadID].join()
				print "============ThreadID" + str(threadID)
				print "Output:" + self.threadResults[threadID]
				batchedDataReturn[x.batchedDataName] = self.threadResults[threadID] 
				resultText.insert(threadID, self.threadResults[threadID])
				threadID+=1

			print("Do stuff after batch request")
			pause, ret= workflow.processCurrentState(0,batchedDataReturn,passArgs,resultText,resultImage)
			print("after processstate")
			
			# store info into session
			if pause:
				if LUCID not in Config.SESSION:
					Config.SESSION[LUCID] = {}
				Config.SESSION[LUCID]['workflow'] = workflow
				Config.SESSION[LUCID]['resulttext'] = resultText
			
			# pop session when workflow ends
			if workflow.isEnd:
				if LUCID in Config.SESSION:
					Config.SESSION.pop(LUCID, None)

        print("successful return")
        return ret

thrift_client = ThriftClient(Config.SERVICES)
