import os

TRAIN_OR_LOAD = 'train' # either 'train' or 'load'

# If you modify the input types: 'audio', 'image', 'text', 'text_image', 
# you need to change QueryClassifier, ThriftClient, and Infer.
# If you modify the service names:'ASR', 'IMM', 'QA', 'CA',
# you need to change the back-end service so that it reports itself correctly to
# the command center (i.e. QueryInput's name matches the name here).

# Map from service to its input type.
SERVICE_LIST = { 'ASR' : 'audio', 'IMM' : 'image' , 'QA' : 'text',
				 'CA' : 'text' }
 
# Map from knowledge type to services that can learn this type of knowledge.
LEARNERS = { 'audio' : [], 'image' : [ 'IMM' ], 'text' : [ 'QA' ] }

# Map from input type to query classes and services needed by each class.
CLASSIFIER_DESCRIPTIONS = { 'text' : { 'class_QA' :  [ 'QA' ] ,
							'class_CA' : [ 'CA' ] },
							'image' : { 'class_IMM' : [ 'IMM' ] },
							'text_image' : { 'class_QA': [ 'QA' ],
							'class_IMM' : [ 'IMM' ], 
							'class_IMM_QA' : [ 'IMM', 'QA' ] } }

# Pre-configured services.
# The ThriftClient assumes that the following services are running.
# Only set when Lucida is running in Docker containers.
REGISTRERED_SERVICES = {}
if not os.environ.get('DOCKER') is None:
	REGISTRERED_SERVICES = {'IMM': [('IMM', 8082)], 'QA': [('QA', 8083)], \
		 'CA': [('CA', 8084)]}

for input_type, services in LEARNERS.iteritems():
	for service in services:
		if not service in SERVICE_LIST:
			print 'LEARNERS:', service, 'is not in SERVICE_LIST'       

for input_type in CLASSIFIER_DESCRIPTIONS:
	print '@@@@@ When query type is ' + input_type + ', there are ' + \
		str(len(CLASSIFIER_DESCRIPTIONS[input_type])) + ' possible classes:'
	i = 0
	for query_class_name, services in \
		CLASSIFIER_DESCRIPTIONS[input_type].iteritems():
		print str(i) + '. ' + query_class_name + ' -- needs to invoke ' \
			+ str(services)
		for service in services:
			if not service in SERVICE_LIST:
				print 'CLASSIFIER_DESCRIPTIONS', service, \
					'is not in SERVICE_LIST'
		i += 1
