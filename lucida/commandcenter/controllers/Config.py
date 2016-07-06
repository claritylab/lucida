from Service import Service
from Graph import Graph, Node

MAX_DOC_NUM_PER_USER = 30 # maximum number of texts or images per user

TRAIN_OR_LOAD = 'train' # either 'train' or 'load'

# Pre-configured services.
# The ThriftClient assumes that the following services are running.
# Host IP addresses are resolved dynamically: 
# either set by Kubernetes or localhost.
SERVICES = { 
	'IMM' : Service('IMM', 8082, 'image', ['image']), 
	'QA' : Service('QA', 8083, 'text', ['text']),
	'CA' : Service('CA', 8084, 'text', None),
	'IMC' : Service('IMC', 8085, 'image', None),
	'FACE' : Service('FACE', 8086, 'image', None),
	'DIG' : Service('DIG', 8087, 'image', None),
	'ENSEMBLE' : Service('ENSEMBLE', 9090, 'text', None) 
	}

# Map from input type to query classes and services needed by each class.
CLASSIFIER_DESCRIPTIONS = { 
	'text' : { 'class_QA' :  Graph([Node('QA')]) ,
			   'class_CA' : Graph([Node('CA')]) },
	'image' : { 'class_IMM' : Graph([Node('IMM')]),
				'class_IMC' : Graph([Node('IMC')]),
				'class_FACE' : Graph([Node('FACE')]),
				'class_DIG' : Graph([Node('DIG')]) },
	'text_image' : { 'class_QA': Graph([Node('QA')]),
					 'class_IMM' : Graph([Node('IMM')]), 
					 'class_IMM_QA' : Graph([Node('IMM', [1]), Node('QA')]),
					 'class_IMC' : Graph([Node('IMC')]),
					 'class_FACE' : Graph([Node('FACE')]),
					 'class_DIG' : Graph([Node('DIG')]) } }

# Check the above configurations.
if MAX_DOC_NUM_PER_USER <= 0:
	print 'MAX_DOC_NUM_PER_USER must be non-negative'
	exit()
if not (TRAIN_OR_LOAD == 'train' or TRAIN_OR_LOAD == 'load'):
	print 'TRAIN_OR_LOAD must be either train or load'
	exit()
for service_name, service_obj in SERVICES.iteritems():
	if not service_name == service_obj.name:
		print service_name, 'must be the same as', service_obj.name
		exit()
for input_type in CLASSIFIER_DESCRIPTIONS:
	print '@@@@@ When query type is ' + input_type + ', there are ' + \
		str(len(CLASSIFIER_DESCRIPTIONS[input_type])) + ' possible classes:'
	i = 0
	for query_class_name, graph in \
		CLASSIFIER_DESCRIPTIONS[input_type].iteritems():
		print str(i) + '. ' + query_class_name + ' -- needs to invoke ' \
			+ graph.to_string()
		for node in graph.node_list:
			if not node.service_name in SERVICES:
				print 'Unrecognized service:', node.sercice_name
				exit()
		i += 1
