from Service import Service, WorkerService
from Graph import Graph, Node
from dcm import*

# The maximum number of texts or images for each user.
# This is to prevent the server from over-loading.
MAX_DOC_NUM_PER_USER = 30 # non-negative inetegr

# Train or load the query classifier.
# If you set it to 'load', it is assumed that
# models are already saved in `../models`.
TRAIN_OR_LOAD = 'train' # either 'train' or 'load'

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
    'WE' : Service('WE', 8088, 'text', None),
    'DCM_WE' : WorkerService('DCM', WEDCM.WEDCM()),
    'DCM_IMM' : WorkerService('DCM', IMMDCM.IMMDCM())
    }

# Map from input type to query classes and services needed by each class.
CLASSIFIER_DESCRIPTIONS = {
    'text' : { 'class_QA' :  Graph([Node('QA')]),
               'class_CA' : Graph([Node('CA')]),
               'class_WE' : Graph([Node('WE')]),
               'class_WE_DCM' : Graph([Node('WE', [1]), Node('DCM_WE', [0])]) },
    'image' : { 'class_IMM' : Graph([Node('IMM')]),
                'class_IMC' : Graph([Node('IMC')]),
                'class_FACE' : Graph([Node('FACE')]),
                'class_DIG' : Graph([Node('DIG')]) },
    'text_image' : { 'class_QA': Graph([Node('QA')]),
                     'class_IMM' : Graph([Node('IMM')]),
                     'class_IMM_QA' : Graph([Node('IMM', [1]), Node('QA')]),
                     'class_IMC' : Graph([Node('IMC')]),
                     'class_FACE' : Graph([Node('FACE')]),
                     'class_DIG' : Graph([Node('DIG')]),
                     'class_IMM_DCM_QA_WE' : Graph([Node('IMM', [1]), Node('DCM_IMM', [0,2]), Node('QA', [3]), Node('WE')]) }
    }

# TODO: Should I have this in its own Config file?
# Structure used to save the state/context across requests in a session
# example:
# SESSION = { <user>:
#                   'graph': <Graph>,
#                   'data': <response_data>
# }
SESSION = {}
