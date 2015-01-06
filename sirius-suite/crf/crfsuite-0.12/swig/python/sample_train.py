#!/usr/bin/env python

import crfsuite
import sys

# Inherit crfsuite.Trainer to implement message() function, which receives
# progress messages from a training process.
class Trainer(crfsuite.Trainer):
    def message(self, s):
        # Simply output the progress messages to STDOUT.
        sys.stdout.write(s)

def instances(fi):
    xseq = crfsuite.ItemSequence()
    yseq = crfsuite.StringList()
    
    for line in fi:
        line = line.strip('\n')
        if not line:
        	# An empty line presents an end of a sequence.
            yield xseq, tuple(yseq)
            xseq = crfsuite.ItemSequence()
            yseq = crfsuite.StringList()
            continue

		# Split the line with TAB characters.
        fields = line.split('\t')
    	
    	# Append attributes to the item.
        item = crfsuite.Item()
        for field in fields[1:]:
            p = field.rfind(':')
            if p == -1:
            	# Unweighted (weight=1) attribute.
                item.append(crfsuite.Attribute(field))
            else:
            	# Weighted attribute
                item.append(crfsuite.Attribute(field[:p], float(field[p+1:])))
        
        # Append the item to the item sequence.
        xseq.append(item)
        # Append the label to the label sequence.
        yseq.append(fields[0])

if __name__ == '__main__':
	# This demonstrates how to obtain the version string of CRFsuite.
    print crfsuite.version()

	# Create a Trainer object.
    trainer = Trainer()
    
    # Read training instances from STDIN, and set them to trainer.
    for xseq, yseq in instances(sys.stdin):
        trainer.append(xseq, yseq, 0)

	# Use L2-regularized SGD and 1st-order dyad features.
    trainer.select('l2sgd', 'crf1d')
    
    # This demonstrates how to list parameters and obtain their values.
    for name in trainer.params():
        print name, trainer.get(name), trainer.help(name)
    
    # Set the coefficient for L2 regularization to 0.1
    trainer.set('c2', '0.1')
    
    # Start training; the training process will invoke trainer.message()
    # to report the progress.
    trainer.train(sys.argv[1], -1)
