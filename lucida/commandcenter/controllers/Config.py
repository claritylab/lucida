from Service import Service
from Graph import Graph, Node
from Parser import port_dic
from dcm import*

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
    'IMM' : Service('IMM', int(port_dic["imm_port"]), 'image', 'image'),
    'QA' : Service('QA', int(port_dic["qa_port"]), 'text', 'text'),
    'CA' : Service('CA', int(port_dic["ca_port"]), 'text', None),
    'IMC' : Service('IMC', int(port_dic["imc_port"]), 'image', None),
    'FACE' : Service('FACE', int(port_dic["face_port"]), 'image', None),
    'DIG' : Service('DIG', int(port_dic["dig_port"]), 'image', None),
    'WE' : Service('WE', int(port_dic["we_port"]), 'text', None),
    'MS' : Service('MS', int(port_dic["ms_port"]), 'text', None),
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

# TODO: Should I have this in its own Config file?
# Structure used to save the state/context across requests in a session
# example:
# SESSION = { <user>:
#                   'graph': <Graph>,
#                   'data': <response_data>
# }
SESSION = {}
