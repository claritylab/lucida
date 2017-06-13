from Service import Service
from Graph import Graph, Node
from pymongo import MongoClient
import os, sys
from dcm import*

def load_config():
    mongodb_addr = os.environ.get('MONGO_PORT_27017_TCP_ADDR')
    if mongodb_addr:
        db = MongoClient(mongodb_addr, 27017).lucida
    else:
        db = MongoClient().lucida
    service_list = db["service_info"].find()
    count = service_list.count()
    for i in range(count):
        service_obj = service_list[i]
        acn = service_obj['acronym']
        port = int(service_obj['port'])
        input_type = service_obj['input']
        learn_type = service_obj['learn']
        if learn_type == 'none':
            SERVICES[acn] = Service(acn, port, input_type, None)
        else:
            SERVICES[acn] = Service(acn, port, input_type, learn_type)
        CLASSIFIER_DESCRIPTIONS[input_type]['class_' + acn] = Graph([Node(acn)])
    return 0

# The maximum number of texts or images for each user.
# This is to prevent the server from over-loading.
MAX_DOC_NUM_PER_USER = 30 # non-negative inetegr

# Train or load the query classifier.
# If you set it to 'load', it is assumed that
# models are already saved in `../models`.
TRAIN_OR_LOAD = 'train' # either 'train' or 'load'



####################### How does a workflow work? Reference firstWorkFlow as a walkthrough example.

#Contains serviceName and data to pass. Needed for batch (and thereby parallel) processing.
class serviceRequestData(object):

	def __init__(self,nameOfService,argData):
		self.argumentData = argData
		self.serviceName = nameOfService
	

class workFlow(object):
		def __init__(self):
			self.currentState = 0; # What state on the state graph
			self.isEnd = False;
			self.batchedData = []
	
	
	
	

class firstWorkflow(workFlow):

	
	def processCurrentState(self,inputModifierText,inputModifierImage):
		print "Executing state logic";
		
		if(self.currentState==0):
			print "State 0";
			self.currentState = 1; # This decides what state to go to next
			# batchedData contains a list of service Requests. The function parameter is serviceRequestData(serviceName,dataToPassToService).
			# Eg. "QA",inputModifierText[0]) means to pass to QA microservice with whatever was in the inputModifierText[0] (The text from the Lucida prompt))
			self.batchedData = [serviceRequestData("QA",[unicode("How old is Johann")]),serviceRequestData("QA",inputModifierText[0])];
			return;
		
		if(self.currentState==1):
			print "State 1";
			# [1] is being passed as the input. This value came from: serviceRequestData("QA",inputModifierText[0])
			# It is based on the positioning of the previous serviceRequestData batch.
			# Eg. [0] = serviceRequestData("QA",[unicode("How old is Johann")], [1] = serviceRequestData("QA",inputModifierText[0])
			#That means the second entry from state0 is being passed to it.
			self.batchedData = [serviceRequestData("QA",inputModifierText[1])] 
			self.isEnd = True # This indicates the workflow is complete
			return;



class QAWF(workFlow):
	def processCurrentState(self,inputModifierText,inputModifierImage):
		if(self.currentState==0):
			self.batchedData = [serviceRequestData("QA",inputModifierText[0])];
			self.isEnd = True;
			return;

class IMMWF(workFlow):
	def processCurrentState(self,inputModifierText,inputModifierImage):
		if(self.currentState==0):
			self.batchedData = [serviceRequestData("IMM",inputModifierImage[0])];
			self.isEnd = True;
			return;

class CAWF(workFlow):
	def processCurrentState(self,inputModifierText,inputModifierImage):
		if(self.currentState==0):
			self.batchedData = [serviceRequestData("CA",inputModifierText[0])];
			self.isEnd = True;
			return;

class IMCWF(workFlow):
	def processCurrentState(self,inputModifierText,inputModifierImage):
		if(self.currentState==0):
			self.batchedData = [serviceRequestData("IMC",inputModifierImage[0])];
			self.isEnd = True;
			return;

class FACEWF(workFlow):
	def processCurrentState(self,inputModifierText,inputModifierImage):
		if(self.currentState==0):
			self.batchedData = [serviceRequestData("FACE",inputModifierImage[0])];
			self.isEnd = True;
			return;

class DIGWF(workFlow):
	def processCurrentState(self,inputModifierText,inputModifierImage):
		if(self.currentState==0):
			self.batchedData = [serviceRequestData("DIG",inputModifierImage[0])];
			self.isEnd = True;
			return;

class ENSEMBLEWF(workFlow):
	def processCurrentState(self,inputModifierText,inputModifierImage):
		if(self.currentState==0):
			self.batchedData = [serviceRequestData("ENSEMBLE",inputModifierText[0])];
			self.isEnd = True;
			return;


class MSWF(workFlow):
	def processCurrentState(self,inputModifierText,inputModifierImage):
		if(self.currentState==0):
			self.batchedData = [serviceRequestData("MS",inputModifierText[0])];
			self.isEnd = True;
			return;


WFList = {
	"IMMWF" : IMMWF(),
	"firstWorkFlow" : firstWorkflow(),
	"QAWF" : QAWF(),
	"CAWF" : CAWF(),
	"IMCWF" : IMCWF(),
	"FACEWF" : FACEWF(),
	"DIGWF" : DIGWF(),
	"ENSEMBLEWF" : ENSEMBLEWF(),
	"MSWF" : MSWF()

}




# Pre-configured services.
# The ThriftClient assumes that the following services are running.
# Host IP addresses are resolved dynamically:
# either set by Kubernetes or localhost.

SERVICES = {
    }

CLASSIFIER_DESCRIPTIONS = {
    'text' : { 'class_QA' :  Graph([Node('QAWF')]),
               'class_CA' : Graph([Node('CAWF')]),
               'class_WE' : Graph([Node('WEWF')]),
               'class_MS' : Graph([Node('MSWF')]) },
    'image' : { 'class_IMM' : Graph([Node('IMMWF')]),
                'class_IMC' : Graph([Node('IMCWF')]),
                'class_FACE' : Graph([Node('FACEWF')]),
                'class_DIG' : Graph([Node('DIGWF')]) },
    'text_image' : { 'class_QA': Graph([Node('QAWF')]),
                     'class_IMM' : Graph([Node('IMMWF')]),
                     'class_IMC' : Graph([Node('IMCWF')]),
                     'class_FACE' : Graph([Node('FACEWF')]),
                     'class_DIG' : Graph([Node('DIGWF')]), }
    }

load_config()

# TODO: Should I have this in its own Config file?
# Structure used to save the state/context across requests in a session
# example:
# SESSION = { <user>:
#                   'graph': <Graph>,
#                   'data': <response_data>
# }
SESSION = {}
