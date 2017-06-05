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
			self.currentState = 1;
			self.batchedData = [serviceRequestData("QA",[unicode("How old is Johann")]),serviceRequestData("QA",[unicode("What color is a pug?")])];
			return;
		
		if(self.currentState==1):
			self.currentState = 2
			print "State 1";
			self.batchedData = [serviceRequestData("QA",[unicode("How old is Johann")])]
			self.isEnd = True
			return;
		if(self.currentState==2):
			self.currentState = 3;
			print "State 2";
			self.batchedData = [serviceRequestData("DU",inputModifierText[0])]
			return;
						
		if(self.currentState==3):
			print "State 3";
			self.isEnd = True;
			self.batchedData = [serviceRequestData("QA",inputModifierText[0]),serviceRequestData("DU",inputModifierText[0])]
			return;
				



class QAWF(workFlow):
	def processCurrentState(self,inputModifierText,inputModifierImage):
		if(self.currentState==0):
			self.batchedData = [serviceRequestData("QA",inputModifierText[0])];
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
			self.batchedData = [serviceRequestData("IMC",inputModifierText[0])];
			self.isEnd = True;
			return;

class FACEWF(workFlow):
	def processCurrentState(self,inputModifierText,inputModifierImage):
		if(self.currentState==0):
			self.batchedData = [serviceRequestData("FACE",inputModifierText[0])];
			self.isEnd = True;
			return;

class DIGWF(workFlow):
	def processCurrentState(self,inputModifierText,inputModifierImage):
		if(self.currentState==0):
			self.batchedData = [serviceRequestData("DIG",inputModifierText[0])];
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
	'IMM' : Service('IMM', 8082, 'image', 'image'), 
	'QA' : Service('QA', 8083, 'text', 'text'),
	'CA' : Service('CA', 8084, 'text', None),
	'IMC' : Service('IMC', 8085, 'image', None),
	'FACE' : Service('FACE', 8086, 'image', None),
	'DIG' : Service('DIG', 8087, 'image', None),
	'ENSEMBLE' : Service('ENSEMBLE', 9090, 'text', None),
	'MS' : Service('MS', 8089, 'text', None),
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
