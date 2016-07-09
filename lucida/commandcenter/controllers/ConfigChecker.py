from Config import *

# Check Config.py.
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
				print 'CLASSIFIER_DESCRIPTIONS misconfigured'
				print 'Unrecognized service:', node.sercice_name
				exit()
			if input_type == 'text':
				if SERVICES[node.service_name].input_type != 'text':
					print 'CLASSIFIER_DESCRIPTIONS misconfigured'
					print node.service_name, 'does not receive text'
					exit()
			elif input_type == 'image':
				if SERVICES[node.service_name].input_type != 'image':
					print 'CLASSIFIER_DESCRIPTIONS misconfigured'
					print node.service_name, 'does not receive image'
					exit()
			elif input_type == 'text_image':
				pass
			else:
				print 'CLASSIFIER_DESCRIPTIONS misconfigured'
				print 'input type must be either text, image, or text_image'
				exit()
		i += 1
