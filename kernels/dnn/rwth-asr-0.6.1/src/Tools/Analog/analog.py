"""
Analog: Sprint Log-file Analyser

This supersedes a previous program called analog which lived in the
Tools/Speech-Recognizer directory.  This version features a
modularised XML parser, customizable statistics output, and extraction
of recognition results in different formats.

"""

__version__   = '$Revision: 8349 $'
__date__      = '$Date: 2011-08-15 12:59:09 +0200 (Mon, 15 Aug 2011) $'


import sys, traceback, copy, os, string, re
if sys.version_info[:2] < (2, 4):
    from sets import Set; set = Set
    def sorted(l):
	l = list(l)
	l.sort()
	return l

from format import TableFormatter, PrettyFormatter, QtFormatter
from miscLib import uopen, uclose
from parser import LogFileParser

# ===========================================================================
class Processor:
    def begin(self):
	pass

    def logFilename(self, filename, error, prefix):
	pass

    def sep(self):
	pass

    def end(self):
	pass

    def work(self, segments, groupBy):
	raise NotImplementedError

    def isActive(self):
	return True

def groupSegments(segments, groupBy):
    groups = {}
    keys = set()
    for id in segments:
	g = groupBy(segments[id])
	if g not in groups:
	    groups[g] = []
	groups[g].append(id)
	for k in segments[id].keys():
	    keys.add(k)

    return dict(
	[ (g, dict(
	    [ (k, [ segments[id][k]
		    for id in groups[g] ])
	      for k in keys
	    ]))
	  for g in groups
	])

# ===========================================================================
class MutatorClass(type):
    def __init__(cls, name, bases, dictionary):
	if cls.id:
	    Mutator.allMutators += [ cls ]

class Mutator(object):
    __metaclass__ = MutatorClass
    allMutators = []
    id = None

    def __init__(self, options):
	self.options = options

    def work(self, segment):
	raise NotImplementedError

class Manipulator(Processor):
    """
    Manipulate data after reading from log file before they are passed
    to the Extractor and StatisticsReporter.
    """
    def __init__(self, options):
	self.options = options
	mmap = dict([(mutator.id, mutator) for mutator in Mutator.allMutators])
	self.mutators = []
	for id in self.options.mutations:
	    if id in mmap:
		self.mutators.append(mmap[id](self.options))
	    else:
		print >> sys.stderr, 'error: undefined mutator "%s"' % id

    def work(self, segments, groupBy):
	for segment in segments.itervalues():
	    for layer in segment.itervalues():
		for mutator in self.mutators:
		    mutator.work(layer)

    def isActive(self):
	return len(self.mutators) > 0

# ===========================================================================
class WriterClass(type):
    def __init__(cls, name, bases, dictionary):
	if cls.id:
	    Writer.allWriters += [ cls ]

class Writer(object):
    __metaclass__ = WriterClass
    allWriters = []

    id = None
    defaultPostfix = None

    def __init__(self, options):
	self.postfix = getattr(options, str(self.id) + "Postfix");

    def __call__(self, file, data):
	raise NotImplementedError

class Extractor(Processor):
    """
    Write data from log file in various formats.
    """

    def begin(self):
	print "converting"

    def logFilename(self, filename, error, prefix):
	printout = prefix + filename
	if error:
	    printout = '\033[01;31m' + printout + '\033[0;0m'
	print printout

    def work(self, segments, groupBy):
	groups = groupSegments(segments, groupBy)
	groups = sorted(groups.iteritems())
	for groupName, group in groups:
	    self.writeGroup(groupName, group)

    def writeGroup(self, groupName, layers):
	for layer in layers.keys():
	    baseFilename = groupName.replace(' ', '_')
	    if layer:
		baseFilename += "." + layer
	    self.writeGroupLayer(baseFilename, layers[layer])

    def writeGroupLayer(self, baseFilename, data):
	for writer in self.writers:
	    filename = baseFilename + writer.postfix
	    f = self.openOutputFile(filename)
	    if not f:
		continue
	    try:
		writer(f, data)
		print "%s created." % filename
	    except:
		print >> sys.stderr, "Failed to create %s." % (filename)
		os.remove(filename)
		traceback.print_exc()

    def openOutputFile(self, filename):
	if os.path.dirname(filename) and not os.path.exists(os.path.dirname(filename)):
	    os.makedirs(os.path.dirname(filename))
	if os.path.exists(filename):
	    if self.options.force_overwrite:
		print >> sys.stderr, 'File "%s" already exists.  Overwriting as requested.' % (filename)
	    else:
		print >> sys.stderr, 'Failed to create "%s". File already exists.' % filename
		return None
	f = uopen(filename, self.options.encoding, 'w')
	return f

    def __init__(self, options):
	self.options = options
	self.writers = []
	if options.extractionFormats:
	    for extractionFormatId in options.extractionFormats:
		writers = filter(lambda w: w.id == extractionFormatId, Writer.allWriters)
		if len(writers) == 1:
		    self.writers += [ writers[0](options) ]
		else:
		    print >> sys.stderr, 'error selecting format:', extractionFormatId

    def isActive(self):
	return len(self.writers) > 0

# ===========================================================================
class Field:
    def __init__(self, name, width=5, format='%s', unit=''):
	self.name   = name
	self.width  = max(len(name), width)+1
	self.format = format
	self.unit   = unit

    def __repr__(self):
	return 'Field(%s)' % self.name

class CollectorClass(type):
    def __init__(cls, name, bases, dictionary):
	collector = cls
	if collector.id:
	    fields = [ (collector, field) for field in collector.fields ]
	    Collector.allFields += fields

class Collector(object):
    __metaclass__ = CollectorClass
    allFields = []

    id     = None
    fields = None

def pivot(listOfDicts):
    "Convert a list of dictionaries into a dictionary of lists."""
    keys = set([ k for d in listOfDicts for k in d ])
    dictOfLists = dict([
	(k, [ d.get(k, None)  for d in listOfDicts ])
	for k in keys ])
    return dictOfLists

class StatisticsReporter(Processor):
    """
    Report statistics about log file data.
    """

    def begin(self):
	self.format.header()
	self.format.sep()

    def logFilename(self, filename, error, prefix):
	self.format.file(filename, error, prefix)

    def sep(self):
	self.format.sep()

    def work(self, segments, groupBy):
	groups = groupSegments(segments, groupBy)
	groupList = groups.keys()
	groupList.sort()
	for g in groupList:
	    self.reportGroup(g, groups[g])
	if self.format.shouldShowTotal and (len(groups) > 1):
	    self.format.sep()
	    self.work(segments, lambda id: 'total')

    def reportGroup(self, groupName, layers):
	layerList = layers.keys()
	layerList.sort()

	for layer in layerList:
	    data = pivot(layers[layer])
	    def evalStat(stat):
		try:
		    return stat(data)
		except KeyError:
		    return []
	    values = dict([
		((stat, field), value)
		for stat in self.statistics
		for field, value in evalStat(stat)])
	    self.format.row(groupName, layer, values)

    reFieldSpec = re.compile('^([^()]+)(?:\(([^)]*)\))?$')

    def analyseFieldSpec(self, fieldSpec):
	m = self.reFieldSpec.match(fieldSpec)
	if not m:
	    raise ValueError("illegal field format", fieldSpec)

	f = m.group(1).split('.')
	if len(f) == 1:
	    cand = filter(lambda cf: cf[0].id == f[0] or cf[1].name == f[0],
			  Collector.allFields)
	elif len(f) == 2:
	    cand = filter(lambda cf: cf[0].id == f[0] and cf[1].name == f[1],
			  Collector.allFields)
	else:
	    raise ValueError("illegal field format", fieldSpec)
	if not cand:
	    raise ValueError("not matching field", fieldSpec)

	args = m.group(2)
	if args is not None:
	    args = tuple(args.split(','))

	return cand, args

    def selectFields(self, selection):
	self.fields = list()
	statistics = dict()
	for fieldSpec in selection:
	    fields, args = self.analyseFieldSpec(fieldSpec)
	    for stat, field in fields:
		if (stat, args) in statistics:
		    collector = statistics[(stat, args)]
		else:
		    if args is None:
			collector = stat()
		    else:
			collector = stat(*args)
		    statistics[(stat, args)] = collector
		self.fields.append((collector, field))
	self.statistics = statistics.values()

    def __init__(self, options):
	self.options = options
	try:
	    self.selectFields(options.fields)
	except:
	    print >> sys.stderr, 'error selecting fields:', options.fields
	    raise
	if self.isActive():
	    resultFile = uopen(options.resultFile, options.encoding, 'w')
	    if options.bootlog:
		self.format = TableFormatter(resultFile, self.fields)
	    elif options.gui:
		self.format = QtFormatter(resultFile, self.fields)
	    else:
		self.format = PrettyFormatter(resultFile, self.fields)

    def isActive(self):
	return len(self.fields) > 0

# ===========================================================================
def composeLayers(segments):
    for id in segments:
	generalData = segments[id][None]
	for layer in segments[id]:
	    if layer is None: continue
	    data = generalData.copy()
	    data.update(segments[id][layer])
	    segments[id][layer] = data

def selectLayer(segments, layer):
    result = {}
    for id in segments:
	result[id] = segments[id].selectLayer(layer)
    return result


class Analog:
    def logFilename(self, filename, wasParseSuccessful, multiple):
	for processor in self.processors:
	    processor.logFilename(
		filename, not wasParseSuccessful,
		{ True: ' + ', False: ''}[multiple])

    def processFiles(self, files):
	segments = {}
	for fname in files:
	    try:
		self.parser.parseFile(fname, segments)
	    except KeyboardInterrupt:
		print >> sys.stderr, 'keyboard interrupt'
		sys.exit(2)
	    except:
		if self.options.verbose_errors:
		    print >> sys.stderr, '%s FAILED' % fname
		    import traceback
		    traceback.print_exc()
		wasParseSuccessful = False
	    else:
		wasParseSuccessful = True
	    self.logFilename(fname, wasParseSuccessful, (len(files) > 1))

	composeLayers(segments)
	if self.options.layer:
	    segments = selectLayer(segments, self.options.layer)

	for processor in self.processors:
	    processor.work(segments, self.groupBy)
	    processor.sep()

    def __init__(self, options):
	self.options = options
	self.processors = [
	    Manipulator(options),
	    Extractor(options),
	    StatisticsReporter(options) ]
	self.processors = [ proc for proc in self.processors if proc.isActive() ]

	if options.groupBySpeaker:
	    self.groupBy = lambda segment: segment[None]["speaker"]
	elif options.subcorpusDepth > 0:
	    s = options.subcorpusDepth
	    self.groupBy = lambda segment: '/'.join(segment.id.split('/')[:s])
	else:
	    self.groupBy = lambda segment: segment.id

    def main(self, args):
	self.parser = LogFileParser()

	for processor in self.processors:
	    processor.begin()
	if self.options.merge:
	    i = 0
	    for j in range(len(args)):
		if args[j] == '/':
		    self.processFiles(args[i:j])
		    i = j + 1
	    self.processFiles(args[i:])
	else:
	    for fname in args:
		self.processFiles([fname])
	for processor in self.processors:
	    processor.end()

# ===========================================================================
def run():
    try:
	import optparse
    except ImportError:
	print 'Ask your system administrator to install, either a reasonably recent version of Python or "python-optik"!'
	sys.exit(1)

    optparser = optparse.OptionParser(
    usage   = '%prog [options] <recognition log file(s)>...\n' + __doc__,
    version = '%prog ' + __version__)

    optparser.add_option(
	'--verbose-errors', action='store_true',
	help='show a detailed report for parse errors')
    optparser.add_option(
	'-f', '--field', action='append', dest='fields',
	help='select field F (use --list-fields to et a list)', metavar='F')
    optparser.add_option(
	'-F', '--list-fields', action='store_true',
	help='show list of available fields')
    optparser.add_option(
	'-b', '--table-format', action='store_true', dest='bootlog',
	help='print output in bootlog format')
    optparser.add_option(
	'-g', '--gui', action='store_true', dest='gui',
	help='show fields in a Qt dialog')
    optparser.add_option(
	'-m', '--merge', action='store_true', dest='merge',
	help="""merge log files -
By default each log file is processed separately.  If merging is
enabled all log files are combined before computing statistics.  This
is useful e.g. when you split up a recognition task in a job array.
However you can specify several groups to be merged by separating them
with a single slash "/".
e.g.  --merge baseline-20k-recog.out.[123] / baseline-64k-recog.out.[123]
	""")
    optparser.add_option(
	'-u', '--mutate', action='append', dest='mutations', default=[],
	help='apply mutator M to log file data before further processing (order may important)', metavar='M')
    optparser.add_option(
	'-U', '--list-mutators', action='store_true',
	help='show list of available mutators')
    optparser.add_option(
	'-r', '--group-by-speaker', action='store_true', dest='groupBySpeaker',
	help='group results by speaker')
    optparser.add_option(
	'-s', '--group-by-level', metavar='N', type='int', dest='subcorpusDepth', default=1,
	help='group results by subcorpus level N')
    optparser.add_option(
	'-l', '--layer', metavar='L',
	help='use only data from layer L')
    optparser.add_option(
	'--full-name', action='store_true', default=False, dest='fullName',
	help='use full segment id in nist ctm output')
    optparser.add_option(
	'-e', '--encoding', metavar='ENCODING', dest='encoding', default='ascii',
	help='use encoding for files (see below); default is ascii')
    optparser.add_option(
	'--result', metavar='FILE', dest='resultFile', default='-',
	help='write results to FILE (default is standard out)')

    optparser.add_option(
	'-c', '--convert', action='append', dest='extractionFormats',
	help='select extraction in format C (use --list-coversion-formats to et a list)', metavar='C')
    optparser.add_option(
	'-C', '--list-coversion-formats', action='store_true',
	help='show list of available extraction formats')
    optparser.add_option(
	'--force-overwrite', action='store_true',
	help='do not check if files to be written exist already.')
    for writer in Writer.allWriters:
	optparser.add_option(
	    '--%s-postfix' % (writer.id), dest='%sPostfix' % (writer.id),
	    metavar='POSTFIX', help='%s files will have the postfix POSTFIX' % (writer.id),
	    default=writer.defaultPostfix)
    optparser.add_option(
	'--frame-shift', type='float', dest='frameShift', default=0.01,
	help='frame shift in seconds')
    optparser.add_option(
	'--silence-lemma', dest='silenceLemma', default='[SILENCE]',
	help='orthographic form of the silence lemma')

    optparser.add_option(
	'--karaoke-suppress-deletions', dest='karaokeSuppressDeletions', action='store_true',
	help='deletions are marked by underscore')
    optparser.add_option(
	'--karaoke-compare-postfix', dest='karaokeComparePostfix', help='postfix for compare files')
    options, args = optparser.parse_args()

    if options.list_mutators:
	for s in sorted([ mutator.id for mutator in Mutator.allMutators ]):
	    print s
	sys.exit()
    if options.list_fields:
	for s in sorted([ '%s.%s' % (collector.id, field.name)
			  for collector, field in Collector.allFields ]):
	    print s
	sys.exit()
    if options.list_coversion_formats:
	for s in sorted([ writer.id for writer in Writer.allWriters ]):
	    print s
	sys.exit()
    if not args:
	optparser.print_help()
	sys.exit(1)

    if options.fields is None:
	if options.extractionFormats:
	    options.fields = []
	else:
	    if options.gui:
		options.fields = []
		for collector, field in Collector.allFields:
		    if not collector.id in options.fields:
			options.fields.append(collector.id)
	    else:
		options.fields = ['sb-wer', 'rtf']

    analog = Analog(options)
    analog.main(args)
