TRAIN_OR_LOAD = 'train' # either 'train' or 'load'

CLASSIFIER_PIPELINES = { 'speech' : [ ( 'QA',  [ 'QA' ] ), ( 'CA', [ 'CA' ] ) ],
                         'image' : [ ( 'IMM', [ 'IMM' ] ) ],
                         'speech_image' : [ ( 'IMM_QA', [ 'IMM', 'QA' ] ) ] }

for input_type in CLASSIFIER_PIPELINES:
    print '@@@@@ When query type is ' + input_type + ', there are ' + str(len(CLASSIFIER_PIPELINES[input_type])) + ' possible query classes:'
    i = 0
    for query_class in CLASSIFIER_PIPELINES[input_type]:
        print str(i) + '. ' + query_class[0] + ' -- needs to invoke ' + str(query_class[1])
        i += 1

