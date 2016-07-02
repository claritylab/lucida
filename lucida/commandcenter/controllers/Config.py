TRAIN_OR_LOAD = 'train' # either 'train' or 'load'

class Service(object):
	LEARNERS = { 'audio' : [], 'image' : [], 'text' : [] } 
	# Constructor.
	def __init__(self, name, port, input_type, learn_type):
		self.name = name
		self.port = port
		self.input_type = input_type
		if learn_type:
			if not learn_type in Service.LEARNERS:
				print 'learn_type must be one of audio, image, or text'
				exit()
			Service.LEARNERS[learn_type].append(name)

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
	'ENSEMBLE' : Service('ENSEMBLE', 9090, 'text', None) 
	}

# Map from input type to query classes and services needed by each class.
CLASSIFIER_DESCRIPTIONS = { 
	'text' : { 'class_QA' :  [ 'QA' ] ,
			   'class_CA' : [ 'CA' ] },
	'image' : { 'class_IMM' : [ 'IMM' ],
				'class_IMC' : [ 'IMC' ],
				'class_FACE' : [ 'FACE' ],
				'class_DIG' : [ 'DIG' ] },
	'text_image' : { 'class_QA': [ 'QA' ],
					 'class_IMM' : [ 'IMM' ], 
					 'class_IMM_QA' : [ 'IMM', 'QA' ],
					 'class_IMC' : [ 'IMC' ],
					 'class_FACE' : [ 'FACE' ],
					 'class_DIG' : [ 'DIG' ] } }

# Check the above configurations.
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
	for query_class_name, services in \
		CLASSIFIER_DESCRIPTIONS[input_type].iteritems():
		print str(i) + '. ' + query_class_name + ' -- needs to invoke ' \
			+ str(services)
		for service in services:
			if not service in SERVICES:
				print 'CLASSIFIER_DESCRIPTIONS', service, \
					'is not in SERVICE_LIST'
				exit()
		i += 1
