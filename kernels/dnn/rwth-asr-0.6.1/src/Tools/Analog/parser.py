"""
This is the XML parser for Sprint log-files.

Please try to keep it reasonably backward compatible!  Suggestion: If
you need additional code to handle older files, make a comment with
the date.  Such parts should be removed after, say, 6 months.
"""


__version__   = '$Revision: 9621 $'
__date__      = '$Date: 2014-05-13 19:35:55 +0200 (Tue, 13 May 2014) $'

import string, re, sys
from miscLib import zopen, zclose, uopen, uclose
from xmlparser import *


class SprintParser(XmlSchemaParser):
    def __init__(self):
	super(SprintParser, self).__init__()
	root = XmlMixedElement('sprint')
	root.flattenUnknownElements()
	self.setRoot(root)

    def parseFile(self, fname):
	fd = zopen(fname, 'r')
	data = fd.read()
	zclose(fd)
	self.reset()
	if data.startswith('<?xml'):
	    self.parser.feed(data)
	else:
	    self.parser.feed('<?xml version="1.0" encoding="ISO-8859-1"?>')
	    self.parser.feed('<sprint>')
	    self.parser.feed(data)
	    self.parser.feed('</sprint>')
	self.parser.close()

# ===========================================================================
class StatisticElement(XmlMixedElement):
    def __init__(self, attrName, handler, dataElementList=['min', 'avg', 'max'] ):
	super(StatisticElement, self).__init__('statistic')
	self.attrName = attrName
	self.handler = handler
	self.dataElementList = dataElementList
	for name in dataElementList:
	    def handler(atts, data, key=name):
		self.collect(key, float(data))
	    self.addChild(XmlDataOnlyElement(name, handler))
	self.ignoreUnknownElements()
	self.statistic = {}

    def matches(self, name, attr):
	attrName = ' '.join(attr.get('name', '').split())
	return name == self.name and attrName == self.attrName

    def collect(self, key, data):
	self.statistic[key] = data

    def start(self, attr):
	self.statistic = {}

    def end(self):
	self.handler(self.attrName, self.statistic)


class StatisticContainerElement(XmlMixedElement):
    def __init__(self, name, handler, statElementList):
	super(StatisticContainerElement, self).__init__(name)
	self.handler = handler
	self.statElementList = statElementList
	for statElement in self.statElementList:
	    self.addChild(StatisticElement(statElement, self.collect))
	self.ignoreUnknownElements()
	self.statistics = {}

    def collect(self, key, statistic):
	self.statistics[key] = statistic

    def start(self, attr):
	self.statistics = {}

    def end(self):
	self.handler(self.statistics)


class OrthElement(XmlDataOnlyElement):
    def __init__(self, source, handler):
	super(OrthElement, self).__init__('orth', handler)
	self.sourceAttr = source

    def matches(self, name, atts):
	return name == self.name and \
	       atts.get('source') == self.sourceAttr

    def end(self):
	orth = ''.join(self.data)
	orth = ' '.join(orth.split())
	self.handler(orth)

class SpeakerElement(XmlEmptyElement):
    def __init__(self, handler):
	super(SpeakerElement, self).__init__('speaker', handler)

    def start(self, atts):
	self.handler(atts['name'])

class TracebackElement(XmlDataOnlyElement):
    def __init__(self, handler):
	super(TracebackElement, self).__init__('traceback', handler)

    parseTracebackElement = re.compile("""
    ^  \s*  t=\s*(\d+)                # time
       \s+  s=\s*([\d\.e\+]+)         # score
    (?:\s+  (.+?)\s+/([^/]*)/  )?     # orth  /phon/  (optional)
       \s+  (\S+\|\S+|[0-9]+)         # cross-word boundary phonemes (or transit index - obsolete)
    \s* $
    """, re.MULTILINE | re.VERBOSE)

    def end(self):
	items = []
	data = string.join(self.data, '')
	for match in self.parseTracebackElement.finditer(data):
	    time, score, orth, phon, transit = match.groups()
	    items += [(time, score, orth)]
	self.handler(items)


class EditDistanceStatisticsElement(XmlMixedElement):
    def __init__(self, handler):
	super(EditDistanceStatisticsElement, self).__init__('statistic')
	self.handler = handler
	self.addChild(XmlDataOnlyElement('count', self.addCount))

    def matches(self, name, atts):
	return name == self.name and \
	       atts.get('type') == 'edit-distance'

    def start(self, atts):
	self.counts = {}

    def addCount(self, atts, cdata):
	event = atts['event']
	self.counts[event] = self.counts.get(event, 0) + int(cdata)

    def end(self):
	self.handler(self.counts)

class ObsoleteEditDistanceStatisticsElement(EditDistanceStatisticsElement):
    " obsolete since January 2005 "
    def __init__(self, name, handler):
	super(ObsoleteEditDistanceStatisticsElement, self).__init__(handler)
	self.nameAttr = name

    def matches(self, name, atts):
	return name == self.name and \
	       atts.get('type') == 'edit-distance' and \
	       atts.get('name') == self.nameAttr


class EditDistanceAlignmentElement(XmlDataOnlyElement):
    def __init__(self, handler):
	super(EditDistanceAlignmentElement, self).__init__('alignment')
	self.handler = handler

    oldParseAlignmentElement = re.compile("\s*a=(.+)\tb=(.+)\s*") # obsolete since January 2005
    parseAlignmentElement = re.compile("^\s*(.+)\s+[={}#]\s+(.+)\s*$")

    def matches(self, name, atts):
	return name == self.name and \
	       atts.get('type') == 'edit-distance'

    def start(self, atts):
	super(EditDistanceAlignmentElement, self).start(atts)
	if atts.has_key('name'): # obsolete since January 2005
	    self.name = atts['name']

    class BadAlignmentLine(ValueError): pass

    def parseLine(self, line):
	# Note: word tokens in Sprint contain a trailing blank per definition
	match = self.parseAlignmentElement.match(line)
	if match:
	    return tuple([ string.strip(g) for g in match.groups() ])

	match = self.oldParseAlignmentElement.match(line)
	if match:
	    return tuple([ string.strip(g) for g in match.groups() ])

	raise self.BadAlignmentLine

    def end(self):
	matches = []
	lines = string.split(string.join(self.data, ''), '\n')
	for line in lines:
	    try:
		matches.append(self.parseLine(line))
	    except self.BadAlignmentLine:
		if len(string.strip(line)):
		    print >> sys.stderr, 'could not parse line:', repr(line)
	self.handler(matches)

class ObsoleteEditDistanceAlignmentElement(EditDistanceAlignmentElement):
    " obsolete since January 2005 "
    def __init__(self, name, handler):
	super(ObsoleteEditDistanceAlignmentElement, self).__init__(handler)
	self.nameAttr = name

    def matches(self, name, atts):
	return name == self.name and \
	       atts.get('type') == 'edit-distance' and \
	       atts.get('name') == self.nameAttr


class EvaluationElement(XmlMixedElement):
    def __init__(self, handler):
	super(EvaluationElement, self).__init__('evaluation')
	self.ignoreUnknownElements()
	self.handler = handler
	self.addChild(EditDistanceAlignmentElement(self.addAlignment))
	self.addChild(EditDistanceStatisticsElement(self.addErrors))

    class EvaluationData:
	pass

    def start(self, atts):
	self.data = self.EvaluationData()
	self.data.name = atts['name']
	self.data.type = atts['type']

    def addAlignment(self, alignment):
	self.data.alignment = alignment

    def addErrors(self, errors):
	self.data.errors = errors

    def end(self):
	self.handler(self.data)


class TimerElement(XmlMixedElement):
    def __init__(self, handler):
	super(TimerElement, self).__init__('timer')
	self.ignoreUnknownElements()
	self.handler = handler
	self.addChild(XmlDataOnlyElement('user', self.addTime))

    def addTime(self, atts, cdata):
	time = float(cdata)
	self.handler(time)


class SegmentLayerElement(XmlMixedElement):
    def __init__(self, name, handler):
	super(SegmentLayerElement, self).__init__(name)
	self.flattenUnknownElements()
	self.handler = handler

	self.addChild(OrthElement(
	    'recognized',
	    lambda orth: self.addData('recognized', orth)))
	self.addChild(TracebackElement(
	    lambda traceback: self.addData('traceback', traceback)))

	self.addChild(ObsoleteEditDistanceAlignmentElement(
	    'word errors',
	    lambda alignment: self.addData('edit distance alignment', alignment)))
	self.addChild(ObsoleteEditDistanceStatisticsElement(
	    'word errors',
	    lambda counts: self.addData('word errors', counts)))
	self.addChild(ObsoleteEditDistanceStatisticsElement(
	    'lattice word errors',
	    lambda counts: self.addData('lattice word errors', counts)))

	self.addChild(EvaluationElement(self.addEvaluationData))

	self.addChild(XmlDataOnlyElement(
	    'word-lattice-density',
	    lambda atts, cdata: self.addData('lattice density', float(cdata if cdata != "-nan" and cdata != "inf" else 0))))
	self.addChild(TimerElement(
	    lambda seconds: self.addData('user time', seconds)))
	self.addChild(XmlDataOnlyElement(
	    'real-time',
	    lambda atts, cdata: self.addData('real time', float(cdata))))
	self.addChild(StatisticContainerElement(
	    'search-space-statistics',
	    lambda statistics: self.addData('search-space-statistics', statistics),
	    ['states before pruning', 'states after pruning',
	     'trees before pruning', 'trees after pruning',
	     'ending words before pruning', 'ending words after pruning', 'ending words after 2nd pruning',
	     'ending words after recombi',
	     'hmm states before pruning', 'hmm states after pruning',
	     'active network states before pruning', 'active network states after pruning',
	     'active network arcs before pruning', 'active network arcs after pruning' ]))

    def addEvaluationData(self, evaluationData):
	if evaluationData.type == 'eval' and evaluationData.name == 'single best':
	    self.addData('edit distance alignment', evaluationData.alignment)
	type = {
	    'eval' : 'word',
	    'orth' : 'letter',
	    'phon' : 'phoneme'} [evaluationData.type]
	if (evaluationData.name == 'single best' or evaluationData.name == 'best-in-lattice'):
	    aspect = '%s errors' % type
	elif evaluationData.name == 'lattice':
	    aspect = 'lattice %s errors' % type
	else:
	    aspect = None
	if aspect:
	    self.addData(aspect, evaluationData.errors)


class LayerElement(SegmentLayerElement):
    def __init__(self, handler):
	super(LayerElement, self).__init__('layer', handler)

    def start(self, atts):
	self.layerName = atts['name']
	self.data = {}

    def addData(self, aspect, data):
	self.data[aspect] = data

    def end(self):
	self.handler(self.layerName, self.data)


class SegmentElement(SegmentLayerElement):
    def __init__(self, handler):
	super(SegmentElement, self).__init__('segment', handler)
	self.addChild(OrthElement(
	    'reference',
	    lambda orth: self.addData('reference', orth)))
	self.addChild(SpeakerElement(lambda speaker: self.addData('speaker', speaker)))
	self.addChild(LayerElement(self.addLayer))

    def start(self, atts):
	try:
	    self.segmentName = atts['full-name']
	except KeyError:
	    self.segmentName = atts['name']
	self.layerData = { None: {} }
	self.addData('start', float(atts['start']))
	self.addData('end', float(atts['end']))
	self.addData('track', int(atts['track']))
	self.addData('name', self.segmentName)

    def addData(self, aspect, data):
	self.layerData[None][aspect] = data

    def addLayer(self, layer, data):
	self.layerData[layer] = data

    def end(self):
	self.handler(self.segmentName, self.layerData)


class RecordingElement(XmlMixedElement):
    def __init__(self, handler):
	super(RecordingElement, self).__init__('recording')
	self.handler = handler
	self.addChild(SegmentElement(self.addSegment))

    def start(self, atts):
	self.recordingName = atts['name']
	if atts.has_key('audio'):
	    self.audio = atts['audio']
	else:
	    self.audio = None

    def addSegment(self, name, data):
	for layerKey in data.keys():
	    data[layerKey]['recording'] = self.recordingName
	    data[layerKey]['audio'] = self.audio
	self.handler(name, data)

class SegmentData(dict):
    def __init__(self, data, id):
	self.id = id
	self.update(data)

    def selectLayer(self, layer):
	return SegmentData({None: self[layer]}, self.id)

class LogFileParser(SprintParser):
    def __init__(self):
	super(LogFileParser, self).__init__()
	self.root.addChild(RecordingElement(self.addSegment))
	self.segments = {}

    def addSegment(self, name, data):
	if name in self.segments:
	    segmentData = self.segments[name]
	    for layer in data:
		if layer in segmentData:
		    for aspect in data[layer]:
			if aspect in segmentData[layer]:
			    if segmentData[layer][aspect] != data[layer][aspect]:
				print >> sys.stderr, \
				      'warning: ignoring repeated segment/layer/aspect "%s"/"%s"/"%s"'\
				      % (name, layer, aspect)
			else:
			    segmentData[layer][aspect] = data[layer][aspect]
		else:
		    segmentData[layer] = data[layer]
	else:
	    self.segments[name] = SegmentData(data, name)

    def parseFile(self, fname, segments = None):
	"""
	The rturn value is a rank three mapping
	segments[segment][layer][aspect]
	"""

	if segments is None:
	    self.segments = {}
	else:
	    self.segments = segments
	super(LogFileParser, self).parseFile(fname)
	return self.segments
