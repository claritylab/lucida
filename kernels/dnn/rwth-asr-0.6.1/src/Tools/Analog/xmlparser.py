"""
A structured XML parsing framework - Python edition
"""

__version__   = '$Revision: 6911 $'
__date__      = '$Date: 2008-10-30 15:35:27 +0100 (Thu, 30 Oct 2008) $'


import xml.sax, xml.sax.saxutils, xml.sax.handler
import inspect
try:
    from xml.sax.saxutils import DefaultHandler
except ImportError:
    from xml.sax.handler import ContentHandler as DefaultHandler


class XmlError(ValueError): pass
class XmlValidityError(XmlError): pass
class XmlWellFormednessError(XmlError): pass


class XmlElement(object):
    def __init__(self, name):
	self.name = name

    def matches(self, name, atts):
	return self.name == '*' or self.name == name

    def start(self, atts): pass
    def end(self): pass
    def characters(self, cdata): raise NotImplementedError
    def element(self, name, atts): raise NotImplementedError


class XmlEmptyElement(XmlElement):
    allowWhitespace = True

    def __init__(self, name, handler = None):
	super(XmlEmptyElement, self).__init__(name)
	self.handler = handler

    def start(self, atts):
	if self.handler:
	    self.handler(atts)

    def characters(self, cdata):
	if not self.allowWhitespace or cdata.strip() != '':
	    raise XmlValidityError('Element "%s" is declared to have empty content' % self.name)

    def element(self, name, atts):
	raise XmlValidityError('Element "%s" is declared to have empty content' % self.name)


class XmlIgnoreElement(XmlElement):
    def characters(self, cdata):
	pass

    def element(self, name, atts):
	return XmlIgnoreElement('*')


class XmlFlattenElement(XmlElement):
    def __init__(self, name, parent):
	super(XmlFlattenElement, self).__init__(name)
	self.parent = parent

    def characters(self, cdata):
	self.parent.characters(cdata)

    def element(self, name, atts):
	return self.parent.element(name, atts)


class XmlParentElement(XmlElement):
    unexpectedElement = None

    def flattenUnknownElements(self):
	self.unexpectedElement = XmlFlattenElement('*', self)

    def ignoreUnknownElements(self):
	self.unexpectedElement = XmlIgnoreElement('*')


class XmlMixedElement(XmlParentElement):
    """
    XML element declaration for the mixed content model.

    XmlMixedElement allows character data interspersed with the
    elements declared in children.  Neither order nor number of
    elements and character data is restricted.
    """

    startHandler = None
    endHandler = None
    charactersHandler = None

    def __init__(self, name, startHandler=None, endHandler=None, charactersHandler=None):
	super(XmlMixedElement, self).__init__(name)
	self.children = []
	if startHandler:
	    self.startHandler = startHandler
	if endHandler:
	    self.endHandler = endHandler
	if charactersHandler:
	    self.charactersHandler = charactersHandler

    def addChild(self, child):
	self.children.append(child)

    def element(self, name, atts):
	matched = None
	for child in self.children:
	    if child.matches(name, atts):
		if matched:
		    raise 'ambiguous child elements'
		matched  = child
	if matched:
	    return matched
	else:
#           parser()->warnAboutUnexpectedElement(_name, name());
	    return self.unexpectedElement

    def start(self, atts):
	if self.startHandler:
	    self.startHandler(atts)

    def end(self):
	if self.endHandler:
	    self.endHandler()

    def characters(self, cdata):
	if self.charactersHandler:
	    self.charactersHandler(cdata)


class XmlRegularElement(XmlParentElement):
    """
    XML element declaration for the regular content model.

    XML elements following the regular content model have child
    elements according to a regular language.  (Character data is not
    allowed.)  In the DTD this language is specified by a regular
    expression.  XmlRegularElement does (currently) not provide regular
    expression parsing.  Instead the language must be specified as a
    deterministic, epsilon-free finite state automaton.  The
    construction of this automaton is left to the user.  States are
    identified with integers.  The initial state is initial, any
    state may be marked as final with addFinalState(). Transitions can
    be defined using addTransition().  Remember that the automaton must
    be deterministic, since this is not checked for and can lead to
    unexpected results!
    """

    initial = '__initial__'
    final = '__final__'
    startHandler = None
    endHandler = None

    def __init__(self, name, startHandler=None, endHandler=None):
	super(XmlRegularElement, self).__init__(name)
	self.transitions = []
	self.state = self.final
	if startHandler:
	    self.startHandler = startHandler
	if endHandler:
	    self.endHandler = endHandler

    def addTransition(self, source, target, child):
	self.transitions.append((source, target, child))

    def addFinalState(self, state):
	self.transitions.append((state, self.final, None))

    def start(self, atts):
	assert self.state == self.final # prevent circular activation
	self.state = self.initial
	if self.startHandler:
	    self.startHandler(atts)

    def element(self, name, atts):
	for source, target, child in self.transitions:
	    if source == self.state and child and child.matches(name, atts):
		self.state = target
		return child
#       parser()->warnAboutUnexpectedElement(_name, name());
	return self.unexpectedElement

    def characters(self, cdata):
	if cdata.strip():
	    raise XmlValidityError('Element "%s" is declared to contain no character data' % self.name)

    def end(self):
	for source, target, child in self.transitions:
	    if source == self.state and child is None and target == self.final:
		self.state = self.final
		break
	else:
	    raise XmlValidityError('Premature end of regular element "%s"' % self.name)
	self.state = self.final
	if self.endHandler:
	    self.endHandler()


class XmlDocumentElement(XmlParentElement):
    """
    Helper class used by XmlSchemaParser when outside the document's
    root element.
    """

    def __init__(self):
	super(XmlDocumentElement, self).__init__('__root__')

    def setRoot(self, root):
	self.root = root

    def start(self, atts):
	self.hasRootOccured = False

    def end(self):
	if not self.hasRootOccured:
	    raise XmlWellFormednessError(
		'Document ended before root element "%s" was found' % self.root.name)

    def element(self, name, atts):
	if self.hasRootOccured:
	    raise XmlWellFormednessError('Document contains more than one root element')
	self.hasRootOccured = True
	if self.root.matches(name, atts):
	    return self.root
	else:
	    raise XmlValidityError(
		'Root element "%s" expected, found "%s" instead' % (self.root.name, name))

    def characters(self, cdata):
	raise XmlWellFormednessError('Character data outside root element')


class XmlSchemaParser(DefaultHandler, XmlDocumentElement):
    def __init__(self):
	super(XmlSchemaParser, self).__init__()
	self.parser = xml.sax.make_parser()
	self.parser.setContentHandler(self)
	self.rootElement = super(XmlSchemaParser, self)

    def startDocument(self):
	assert self.root
	assert len(self.stack) == 0
	self.stack.append(self.rootElement)
	try:
	    self.stack[-1].start(None)
	except XmlError, e:
	    print e

    def startElement(self, name, atts):
	try:
	    next = self.stack[-1].element(name, atts)
	    if next:
		next.start(atts)
		self.stack.append(next)
	    else:
		print XmlValidityError('Cannot handle start tag: "%s"' % name) ###
		self.stack.append(XmlIgnoreElement("*"))
	except XmlError, e:
	    print e

    def endElement(self, name):
	prev = self.stack.pop()
	try:
	    prev.end()
	except XmlError, e:
	    print e

    def characters(self, cdata):
	try:
	    self.stack[-1].characters(cdata)
	except XmlError, e:
	    print e

    def endDocument(self):
	if self.stack[-1] is self.rootElement:
	    self.stack[-1].end()
	    self.stack.pop()
	else:
	    print XmlValidityError('Document incomplete: root element not closed')
	assert len(self.stack) == 0

    def reset(self):
	self.stack = []
	self.parser.reset()

    def parse(self, data):
	self.reset()
	self.parser.feed(data)
	self.parser.close()


# ===========================================================================
class XmlDataOnlyElement(XmlEmptyElement):
    def start(self, atts):
	self.data = []
	self.atts = atts

    def characters(self, cdata):
	self.data.append(cdata)

    def end(self):
	if self.handler:
	    self.handler(self.atts, ''.join(self.data))
