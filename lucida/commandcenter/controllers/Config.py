#!/usr/bin/env python2

from Service import Service
from pymongo import MongoClient
from Database import database
import os, sys, re

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
    'text': [],
    'image': [],
    'text_image': []
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
}
"""

LEARNERS = { 'audio' : [], 'image' : [], 'text' : [] }
"""
Store all service supporting learn
"""

def appendServiceRequest(data,arg1,arg2,arg3):
	data.append(serviceRequestData(arg1,arg2,[arg3]))
	return data

class serviceRequestData(object):
#Contains serviceName and data to pass. Needed for batch (and thereby parallel) processing.

	def __init__(self,batchedDataName,nameOfService,argData):
		self.argumentData = argData
		self.serviceName = nameOfService
		self.batchedDataName = batchedDataName
    
class workFlow(object): 
    
    def __init__(self):
        self.currentState = 0 # What state on the state graph
        self.isEnd = False
        self.pause = False
        self.batchedData = []

def load_config():
    """
    Update the config needed for Lucida
    """

    # Load mongodb
    db = database.db
    for input_t in LEARNERS:
        del LEARNERS[input_t][:]
    SESSION.clear()
    # Update service list
    SERVICES.clear()
    service_list = db["service_info"].find()
    count_service = service_list.count()
    for i in range(count_service):
        service_obj = service_list[i]
        acn = service_obj['acronym']
        instance = service_obj['instance']
        input_type = service_obj['input']
        learn_type = service_obj['learn']
        _id = str(service_obj['_id'])
        # check if uninitialized
        if acn == '':
            if acn not in SERVICES:
                SERVICES[acn] = 1
            else:
                SERVICES[acn] += 1
            continue
        # get num of available and uninitialized instance
        num = len(instance)
        avail_instance = [x for x in instance if x['name'] != '']
        avail = len(avail_instance)
        unini = num-avail
        SERVICES[acn] = Service(acn, input_type, learn_type, unini, avail, avail_instance, _id)
        # update learners
        if learn_type == 'none':
            pass
        else:
            LEARNERS[learn_type].append(_id)
    
    # Update workflow list, current only support single service workflow
    for input_t in CLASSIFIER_DESCRIPTIONS:
        del CLASSIFIER_DESCRIPTIONS[input_t][:]
    WFList.clear()
    workflow_list = db["workflow_info"].find()
    count_workflow = workflow_list.count()
    for i in range(count_workflow):
    	workflow_obj = workflow_list[i]
    	name = workflow_obj['name']
    	input_list = workflow_obj['input']
    	classifier = workflow_obj['classifier']
        # check if uninitialized
        if name == '':
            continue
        CLASSIFIER_PATH[name] = classifier
        code = workflow_obj['code']
    	exec(code)
    	for input_t in input_list:
    		CLASSIFIER_DESCRIPTIONS[input_t].append(name)
    	WFList[name] = eval(name+"()")
    return 0

def get_service_withid(_id):
    for service in SERVICES:
        if service == '':
            continue
        if SERVICES[service]._id == _id:
            return SERVICES[service]

load_config()
