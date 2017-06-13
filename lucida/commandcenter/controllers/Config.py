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
                     'class_MS': Graph([Node('MSWF')]),
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
