"""
Analog Plug-in for reating simple formats such as recognized sentences.
"""

__version__   = '$Revision: 6911 $'
__date__      = '$Date: 2008-10-30 15:35:27 +0100 (Thu, 30 Oct 2008) $'


import string
from analog import Writer

class WriteList(Writer):
    def __init__(self, options, fieldName):
	super(WriteList, self).__init__(options)
	self.fieldName = fieldName

    def __call__(self, file, data):
	for record in data:
	    print >> file, record[self.fieldName]

class RecognizedSentences(WriteList):
    id = 'recognized'
    defaultPostfix = '.rec'

    def __init__(self, options):
	super(RecognizedSentences, self).__init__(options, 'recognized')

class ReferenceSentences(WriteList):
    id = 'reference'
    defaultPostfix = '.ref'

    def __init__(self, options):
	super(ReferenceSentences, self).__init__(options, 'reference')
