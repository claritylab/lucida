#!/usr/bin/env python2

from Service import Service
from Graph import Graph, Node
from pymongo import MongoClient
from Database import database
import os, sys, re
from dcm import*

MAX_DOC_NUM_PER_USER = 30 # non-negative integer
"""
The maximum number of texts or images for each user.
This is to prevent the server from over-loading.
"""

TRAIN_OR_LOAD = 'train' # either 'train' or 'load'
"""
Train or load the query classifier.
If you set it to 'load', it is assumed that
models are already saved in `../models`.
"""

SERVICES = {}
"""
Service dict that store all available services dynamically
Host IP addresses are resolved dynamically:
either set by Kubernetes or localhost.
"""

WFList = {}
"""
Workflow dict that store all available workflows dynamically
"""

CLASSIFIER_DESCRIPTIONS = {
    'text': {},
    'image': {},
    'text_image': {}
}
"""
Classifier stored for each workflow
"""

CLASSIFIER_PATH = {}
"""
Classifier data path for each workflow
"""

SESSION = {}
"""
Structure used to save the state/context across requests in a session
example:
SESSION = { <user>:
                  'graph': <Graph>,
                  'data': <response_data>
}
"""

class serviceRequestData(object):
    """
    Contains serviceName and data to pass. Needed for batch (and thereby parallel) processing.
    """

    def __init__(self,nameOfService,argData):
        self.argumentData = argData
        self.serviceName = nameOfService
    
class workFlow(object): 
    
    def __init__(self):
        self.currentState = 0 # What state on the state graph
        self.isEnd = False
        self.batchedData = []
    
class firstWorkflow(workFlow):
    """
    How does a workflow work? Reference firstWorkFlow as a walkthrough example.
    """

    def processCurrentState(self,inputModifierText,inputModifierImage):
        print "Executing state logic"
        
        if(self.currentState==0):
            print "State 0"
            self.currentState = 1; # This decides what state to go to next
            # batchedData contains a list of service Requests. The function parameter is serviceRequestData(serviceName,dataToPassToService).
            # Eg. "QA",inputModifierText[0]) means to pass to QA microservice with whatever was in the inputModifierText[0] (The text from the Lucida prompt))
            self.batchedData = [serviceRequestData("QA",[unicode("How old is Johann")]),serviceRequestData("QA",inputModifierText[0])]
            return
        
        if(self.currentState==1):
            print "State 1"
            # [1] is being passed as the input. This value came from: serviceRequestData("QA",inputModifierText[0])
            # It is based on the positioning of the previous serviceRequestData batch.
            # Eg. [0] = serviceRequestData("QA",[unicode("How old is Johann")], [1] = serviceRequestData("QA",inputModifierText[0])
            #That means the second entry from state0 is being passed to it.
            self.batchedData = [serviceRequestData("QA",inputModifierText[1])] 
            self.isEnd = True # This indicates the workflow is complete
            return

def load_config():
    """
    Update the config needed for Lucida
    """

    mongodb_addr = os.environ.get('MONGO_PORT_27017_TCP_ADDR')
    if mongodb_addr:
        db = MongoClient(mongodb_addr, 27017).lucida
    else:
        db = MongoClient().lucida

    # Update service list
    SERVICES.clear()
    service_list = db["service_info"].find()
    count_service = service_list.count()
    for i in range(count_service):
        service_obj = service_list[i]
        acn = service_obj['acronym']
        num = int(service_obj['num'])
        instance = service_obj['instance']
        input_type = service_obj['input']
        learn_type = service_obj['learn']
        if learn_type == 'none':
            SERVICES[acn] = Service(acn, input_type, None, num, instance)
        else:
            SERVICES[acn] = Service(acn, input_type, learn_type, num, instance)
    
    # Update workflow list, current only support single service workflow
    for input_t in CLASSIFIER_DESCRIPTIONS:
    	CLASSIFIER_DESCRIPTIONS[input_t].clear()
    WFList.clear()
    workflow_list = db["workflow_info"].find()
    count_workflow = workflow_list.count()
    for i in range(count_workflow):
    	workflow_obj = workflow_list[i]
    	name = workflow_obj['name']
    	input_list = workflow_obj['input']
    	classifier = workflow_obj['classifier']
        CLASSIFIER_PATH['class_'+name] = classifier
        code = workflow_obj['code']
    	exec(code)
    	for input_t in input_list:
    		CLASSIFIER_DESCRIPTIONS[input_t]['class_'+name] = Graph([Node(name)])
    	WFList[name] = eval(name+"()")
    return 0

load_config()
