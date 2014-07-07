// Copyright 2011 RWTH Aachen University. All rights reserved.
//
// Licensed under the RWTH ASR License (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.hltpr.rwth-aachen.de/rwth-asr/rwth-asr-license.html
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// $Id: XmlParser.cc 8249 2011-05-06 11:57:02Z rybach $

#include "Assertions.hh"
#include "XmlParser.hh"
#include <iostream>
#include <libxml/SAX.h>
#include <libxml/parserInternals.h>
#include <sstream>
#include <string>

using namespace Core;


namespace XmlParserPrivate {
    /** xmlChar to char */
    const char *x2c(const xmlChar *x) {
	return reinterpret_cast<const char*>(x);
    }

    /** char to xmlChar */
    const xmlChar *c2x(const char *c) {
	return reinterpret_cast<const xmlChar*>(c);
    }
}

using namespace XmlParserPrivate;

// callback wrappers for libXml ==============================================

xmlParserInputPtr XmlParser::SAX_resolveEntity_callback(
    void *ctx, const xmlChar *publicId, const xmlChar *systemId)
{
    return static_cast<self*>(ctx) -> resolveEntity(
	x2c(publicId), x2c(systemId));
}

void XmlParser::SAX_internalSubset_callback(
    void *ctx, const xmlChar *name,
    const xmlChar *ExternalID, const xmlChar *SystemID)
{
    static_cast<self*>(ctx) -> internalSubset(
	x2c(name), x2c(ExternalID), x2c(SystemID));
}

void XmlParser::SAX_externalSubset_callback(
    void *ctx, const xmlChar *name,
    const xmlChar *ExternalID, const xmlChar *SystemID)
{
    static_cast<self*>(ctx) -> externalSubset(
	x2c(name), x2c(ExternalID), x2c(SystemID));
}

xmlEntityPtr XmlParser::SAX_getEntity_callback(
    void *ctx, const xmlChar *name)
{
    return static_cast<self*>(ctx) -> getEntity(x2c(name));
}

xmlEntityPtr XmlParser::SAX_getParameterEntity_callback(
    void *ctx, const xmlChar *name)
{
    return static_cast<self*>(ctx) -> getParameterEntity(x2c(name));
}

void XmlParser::SAX_entityDecl_callback(
    void *ctx,
    const xmlChar *name, int type, const xmlChar *publicId,
    const xmlChar *systemId, xmlChar *content)
{
    static_cast<self*>(ctx) -> entityDecl(
	x2c(name), type, x2c(publicId), x2c(systemId), x2c(content));
}

void XmlParser::SAX_notationDecl_callback(
    void *ctx, const xmlChar *name,
    const xmlChar *publicId, const xmlChar *systemId)
{
    static_cast<self*>(ctx) -> notationDecl(
	x2c(name), x2c(publicId), x2c(systemId));
}

void XmlParser::SAX_attributeDecl_callback(
    void *ctx, const xmlChar *elem,
    const xmlChar *name, int type, int def,
    const xmlChar *defaultValue, xmlEnumerationPtr tree)
{
    static_cast<self*>(ctx) -> attributeDecl(
	x2c(elem), x2c(name), type, def, x2c(defaultValue), tree);
}

void XmlParser::SAX_elementDecl_callback(
    void *ctx, const xmlChar *name,
    int type, xmlElementContentPtr content)
{
    static_cast<self*>(ctx) -> elementDecl(
	x2c(name), type, content);
}

void XmlParser::SAX_unparsedEntityDecl_callback(
    void *ctx,
    const xmlChar *name, const xmlChar *publicId,
    const xmlChar *systemId, const xmlChar *notationName)
{
    static_cast<self*>(ctx) -> unparsedEntityDecl(
	x2c(name), x2c(publicId), x2c(systemId), x2c(notationName));
}

// void XmlParser::SAX_setDocumentLocator_callback(
//     void *ctx,
//     xmlSAXLocatorPtr loc)
// {
//     static_cast<self*>(ctx) -> set_document_locator(loc) ;
// }

void XmlParser::SAX_startDocument_callback(
    void *ctx)
{
    static_cast<self*>(ctx) -> startDocument() ;
}

void XmlParser::SAX_endDocument_callback(
    void *ctx)
{
    static_cast<self*>(ctx) -> endDocument() ;
}

void XmlParser::SAX_startElement_callback(
    void *ctx, const xmlChar *name,
    const xmlChar **atts)
{
    static_cast<self*>(ctx) -> startElement(
	x2c(name), XmlAttributes(atts)) ;
}

void XmlParser::SAX_endElement_callback(
    void *ctx, const xmlChar *name)
{
    static_cast<self*>(ctx) -> endElement(x2c(name)) ;
}

void XmlParser::SAX_reference_callback(
    void *ctx, const xmlChar *name)
{
    static_cast<self*>(ctx) -> reference(x2c(name)) ;
}

void XmlParser::SAX_characters_callback(
    void *ctx, const xmlChar *ch,
    int len)
{
    static_cast<self*>(ctx) -> characters(x2c(ch), len) ;
}

void XmlParser::SAX_cdataBlock_callback(
    void *ctx, const xmlChar *ch,
    int len)
{
    static_cast<self*>(ctx) -> cdataBlock(x2c(ch), len) ;
}

void XmlParser::SAX_ignorableWhitespace_callback(
    void *ctx,
    const xmlChar *ch, int len)
{
    static_cast<self*>(ctx) -> ignorableWhitespace(x2c(ch), len) ;
}

void XmlParser::SAX_processingInstruction_callback(
    void *ctx,
    const xmlChar *target, const xmlChar *data)
{
    static_cast<self*>(ctx) -> processingInstruction(x2c(target), x2c(data));
}

void XmlParser::SAX_comment_callback(void *ctx, const xmlChar *value) {
    static_cast<self*>(ctx) -> comment(x2c(value));
}

void XmlParser::SAX_warning_callback(void *ctx, const char *msg, ...) {
    va_list ap ;
    va_start(ap, msg) ;
    static_cast<self*>(ctx) -> vWarning(msg, ap);
    va_end(ap) ;
}

void XmlParser::SAX_error_callback(void *ctx, const char *msg, ...) {
    va_list ap ;
    va_start(ap, msg) ;
    static_cast<self*>(ctx) -> vError(msg, ap);
    va_end(ap) ;
}

void XmlParser::SAX_fatalError_callback(void *ctx, const char *msg, ...) {
    va_list ap ;
    va_start(ap, msg) ;
    static_cast<self*>(ctx) -> vCriticalError(msg, ap);
    va_end(ap) ;
}

int XmlParser::SAX_isStandalone_callback(void *ctx) {
    return static_cast<self*>(ctx) -> isStandalone() ;
}

int XmlParser::SAX_hasInternalSubset_callback(void *ctx) {
    return static_cast<self*>(ctx) -> hasInternalSubset() ;
}

int XmlParser::SAX_hasExternalSubset_callback(void *ctx) {
    return static_cast<self*>(ctx) -> hasExternalSubset() ;
}

// callback table for libXml
const xmlSAXHandler XmlParser::SAX_callbacks = {
    XmlParser::SAX_internalSubset_callback,
    XmlParser::SAX_isStandalone_callback,
    XmlParser::SAX_hasInternalSubset_callback,
    XmlParser::SAX_hasExternalSubset_callback,
    XmlParser::SAX_resolveEntity_callback,
    XmlParser::SAX_getEntity_callback,
    XmlParser::SAX_entityDecl_callback ,
    XmlParser::SAX_notationDecl_callback,
    XmlParser::SAX_attributeDecl_callback,
    XmlParser::SAX_elementDecl_callback,
    XmlParser::SAX_unparsedEntityDecl_callback,
    0, // XmlParser::SAX_setDocumentLocator_callback ,
    XmlParser::SAX_startDocument_callback ,
    XmlParser::SAX_endDocument_callback ,
    XmlParser::SAX_startElement_callback ,
    XmlParser::SAX_endElement_callback ,
    XmlParser::SAX_reference_callback ,
    XmlParser::SAX_characters_callback ,
    XmlParser::SAX_ignorableWhitespace_callback ,
    XmlParser::SAX_processingInstruction_callback ,
    XmlParser::SAX_comment_callback ,
    XmlParser::SAX_warning_callback ,
    XmlParser::SAX_error_callback ,
    XmlParser::SAX_fatalError_callback ,
    XmlParser::SAX_getParameterEntity_callback,
    XmlParser::SAX_cdataBlock_callback,
    XmlParser::SAX_externalSubset_callback,
} ;

// default implementations of parse functions ================================

int XmlParser::isStandalone() {
    return 0; // ::isStandalone(ctxt_) ;
}

int XmlParser::hasInternalSubset() {
    return 0; //::hasInternalSubset(ctxt_) ;
}

int XmlParser::hasExternalSubset() {
    return 0 ; // ::hasExternalSubset(ctxt_)
}

xmlParserInputPtr XmlParser::resolveEntity(
    const char *publicId, const char *systemId)
{
    return ::resolveEntity(ctxt_, c2x(publicId), c2x(systemId)) ;
}

void XmlParser::internalSubset(
    const char *name, const char *ExternalID, const char *SystemID)
{
    // ::internalSubset(ctxt_, name, ExternalID, SystemID) ;
}

void XmlParser::externalSubset(
    const char *name, const char *ExternalID, const char *SystemID)
{
    // ::externalSubset(ctxt_, name, ExternalID, SystemID) ;
}

xmlEntityPtr XmlParser::getEntity(const char *name) {
    return ::xmlGetPredefinedEntity(c2x(name)) ;
}

xmlEntityPtr XmlParser::getParameterEntity(const char *name) {
    return NULL ;
}

void XmlParser::entityDecl(
    const char *name, int type, const char *publicId,
    const char *systemId, const char *content)
{
//  ::entityDecl(ctxt_, name, type, publicId, systemId, content) ;
}

void XmlParser::attributeDecl(
    const char *elem, const char *name, int type, int def,
    const char *defaultValue, xmlEnumerationPtr tree)
{
//  ::attributeDecl(ctxt_, elem, name, type, def, defaultValue, tree) ;
}

void XmlParser::elementDecl(
    const char *name, int type, xmlElementContentPtr content)
{
//  ::elementDecl(ctxt_, name, type, content) ;
}

void XmlParser::notationDecl(
    const char *name, const char *publicId, const char *systemId)
{
//  ::notationDecl(ctxt_, name, publicId, systemId) ;
}

void XmlParser::unparsedEntityDecl(
    const char *name, const char *publicId,
    const char *systemId, const char *notationName)
{
//  ::unparsedEntityDecl(ctxt_, name, publicId, systemId, notationName) ;
}

// void XmlParser::set_document_locator(xmlSAXLocatorPtr loc) {}

void XmlParser::startDocument() {
//  startDocument(ctxt_) ;
}

void XmlParser::endDocument() {
//  endDocument(ctxt_) ;
}

void XmlParser::startElement(const char *name, const XmlAttributes atts) {
//  startElement(ctxt_, name, atts) ;
}

void XmlParser::endElement(const char *name) {
//  endElement(ctxt_, name) ;
}

void XmlParser::reference(const char *name) {
//  ::reference(name) ;
}

void XmlParser::characters(const char *ch, int len) {
//  ::characters(ctxt_, ch, len) ;
}

void XmlParser::cdataBlock(const char *ch, int len) {
//  cdataBlock(ctxt_, ch, len) ;
    characters(ch, len) ;
}

void XmlParser::ignorableWhitespace(const char *ch, int len) {
//  ignorableWhitespace(ctxt_, ch, len) ;
}

void XmlParser::processingInstruction(
    const char *target, const char *data)
{
//  processingInstruction(ctxt_, target, data) ;
}

void XmlParser::comment(const char *value) {
//  ::comment(ctxt_, value) ;
}

XmlChannel *XmlParser::vErrorMessage(ErrorType mt, const char *msg, va_list ap) const {
    XmlChannel *chn = Component::vErrorMessage(mt, msg, ap);
    if (ctxt_ != NULL) {
	*chn << " in \""   << x2c(xmlSAX2GetSystemId(ctxt_)) << "\""
	     << " line "   << xmlSAX2GetLineNumber(ctxt_)
	     << " column " << xmlSAX2GetColumnNumber(ctxt_);
    }
    return chn;
}

XmlParser::Location XmlParser::location() const {
    Location result ;

    result.systemId  = x2c(xmlSAX2GetSystemId(ctxt_));
    result.line      = xmlSAX2GetLineNumber(ctxt_);
    result.column    = xmlSAX2GetColumnNumber(ctxt_);

    return result ;
}

void XmlParser::giveUp() {
    ctxt_->valid = 0 ;
    ctxt_->disableSAX = 1 ;

    error("unrecoverable error, giving up parsing");
}

// public functions ==========================================================

const ParameterBool XmlParser::paramWarnAboutUnexpectedElements(
    "warn-about-unexpected-elements",
    "enable (default) warnings about unexpected elements",
    true,
    "By default a warning is issued whenever an unknown element is "
    "ignored or flattend.  Use this flag to turn this off.");

XmlParser::XmlParser(const Configuration &c) :
    Component(c),
    ctxt_(NULL)
{
    warnAboutUnexpectedElements_ = paramWarnAboutUnexpectedElements(config);
}

XmlParser::~XmlParser() {}

void XmlParser::warnAboutUnexpectedElement(
    const char *element,
    const char *context)
{
    if (warnAboutUnexpectedElements_)
	warning("Found unexpected element: \"%s\" within \"%s\"",
		element, context) ;
}

int XmlParser::parse() {
    int ret = 0;

    if (ctxt_ == NULL) return -1;

    xmlSAXHandler *oldSax = ctxt_->sax;
    ctxt_->sax = const_cast<xmlSAXHandler*>(&SAX_callbacks) ;
    ctxt_->userData = static_cast<void*>(this)  ;

    xmlParseDocument(ctxt_) ;

    if (ctxt_->wellFormed && ctxt_->valid)
	ret = 0 ;
    else
	ret = (ctxt_->errNo != 0) ? ctxt_->errNo : -1;

    ctxt_->sax = oldSax;

    respondToDelayedErrors();

    return ret ;
}

int XmlParser::parseString(const char *str) {
    ctxt_ = xmlCreateMemoryParserCtxt(const_cast<char*>(str), strlen(str));
    if (ctxt_ == NULL)
	error("Failed to setup XML parser");
    int status = parse();
    xmlFreeParserCtxt(ctxt_); ctxt_ = NULL;
    return status;
}

/**
 *  Implementation issues:
 *    - xmlCreatePushParserCtxt needs at least 4 bytes to allow content encoding detection
 *    - xmlParseChunk does only seem to work if called at least twice!
 */
int XmlParser::parseStream(std::istream &is, const std::string &filename) {
    int status = 0;
    char buffer[4096];
    size_t nRead;

    nRead = readChunk(is, buffer, 4);
    ctxt_ = xmlCreatePushParserCtxt(
	const_cast<xmlSAXHandler*>(&SAX_callbacks),
	static_cast<void*>(this),
	buffer, nRead, filename.c_str());
    if (ctxt_ == NULL)
	error("Failed to setup XML parser");

    while (!status && (nRead = readChunk(is, buffer, sizeof(buffer))) > 0)
	status = xmlParseChunk(ctxt_, buffer, nRead, 0);
    status = xmlParseChunk(ctxt_, buffer, 0, 1);

    if (ctxt_->wellFormed && ctxt_->valid)
	status = 0 ;
    else
	status = (ctxt_->errNo != 0) ? ctxt_->errNo : -1;
    xmlFreeParserCtxt(ctxt_); ctxt_ = NULL;
    respondToDelayedErrors();
    return status;
}

size_t XmlParser::readChunk(std::istream &is, char *buffer, size_t bufferSize) {
    require(bufferSize > 1);

#if GCC_v3
    /** in gcc-3.1 istream::readsome(...) returns always zero. It worked fine in gcc-3.0.4. */
    return is.readsome(buffer, bufferSize - 1);
#else
    is.read(buffer, bufferSize);
    return is.gcount();
#endif
}

int XmlParser::parseFile(const char *filename) {
    ctxt_ = xmlCreateFileParserCtxt(filename) ;
    if (ctxt_ == NULL) {
	error("Failed to setup XML parser for \"%s\"", filename);
	return -1;
    }
    int status = parse();
    xmlFreeParserCtxt(ctxt_); ctxt_ = NULL;
    return status;
}

// ===========================================================================
XmlElementGarbageCollector::XmlElementGarbageCollector()
{}

XmlElementGarbageCollector::~XmlElementGarbageCollector()
{
    for (std::vector<XmlElement*>::iterator i = xmlElements_.begin(); i != xmlElements_.end(); ++i)
	delete *i;
}

// ===========================================================================
XmlElement::XmlElement(
    const char *_name, XmlContext *_context) :
    context_(_context),
    name_(_name)
{}

const char *XmlElement::wildcard = " * ";

bool XmlElement::matches(const char *_name) const {
    return (strcmp(name(), wildcard) == 0)
	|| (strcmp(name(),    _name) == 0);
}

// ===========================================================================
int XmlAttributes::size() const {
    if (size_ < 0) {
	if (atts_) {
	    size_ = 0;
	    for (const xmlChar **a = atts_ ; *a ; a += 2) ++size_;
	}
    }
    return size_;
}

XmlAttributes::Item XmlAttributes::operator[](int i) const {
    require(0 <= i && i < size());
    verify(atts_);
    Item result;
    result.key   = x2c(atts_[2*i    ]);
    result.value = x2c(atts_[2*i + 1]);
    return result;
}

const char *XmlAttributes::operator[](const char *key) const {
    if (atts_)
	for (const xmlChar **a = atts_ ; *a ; a += 2)
	    if (xmlStrcmp(a[0], c2x(key)) == 0)
		return x2c(a[1]);
    return 0 ;
}

// ===========================================================================
XmlDocumentElement::XmlDocumentElement(XmlElement *_root) :
    XmlParentElement(" ROOT ", this),
    root_(_root)
{}

void XmlDocumentElement::setRoot(XmlElement *_root) {
    root_ = _root;
}

void XmlDocumentElement::start(const XmlAttributes atts) {
    hasRootOccured_ = false;
}

void XmlDocumentElement::end() {
    if (!hasRootOccured_) {
	/* well-formedness error */
	parser()->error("Document ended before root element \"%s\" was found",
			root_->name());
    }
}

XmlElement* XmlDocumentElement::element(const char *name) {
    if (hasRootOccured_) {
	/* well-formedness error */
	parser()->error("Document contains more than one root element");
	return 0;
    }
    hasRootOccured_ = true;

    if (root_->matches(name)) {
	return root_ ;
    } else {
	/* validityError */
	parser()->error("Root element \"%s\" expected, found \"%s\" instead",
			root_->name(), name);
	return 0 ;
    }
}

void XmlDocumentElement::characters(const char *ch, int len) {
    /* well-formedness error */
    parser()->error("Character data outside root element");
}

// ===========================================================================
XmlEmptyElement::XmlEmptyElement(
    const char *_name, XmlContext *_context, bool allowWhitespace) :
    XmlElement(_name, _context),
    allowWhitespace_(allowWhitespace)
{}

void XmlEmptyElement::end() {}

XmlElement* XmlEmptyElement::element(const char *_name) {
    /* validityError */
    parser()->error("Element \"%s\" is declared to have empty content",
		    name()) ;
    return 0 ;
}

void XmlEmptyElement::characters(const char *ch, int len) {
    if (!allowWhitespace_ || std::string((char*) ch, len).find_first_not_of(utf8::whitespace) != std::string::npos)
	// validityError
	parser()->error("Element \"%s\" is declared to have empty content",
			name()) ;
}

// ===========================================================================
XmlEmptyElementRelay::XmlEmptyElementRelay(
    const char *_name, XmlContext *_context,
    XmlContext::StartHandler _startHandler /*= 0*/) :
    XmlEmptyElement(_name, _context)
{
    startHandler_ = _startHandler ;
}

void XmlEmptyElementRelay::start(const XmlAttributes atts) {
    if (startHandler_)
	(context()->*startHandler_)(atts) ;
}


// ===========================================================================
XmlElement *XmlIgnoreElement::ignoreElement_ = 0 ;

XmlIgnoreElement::XmlIgnoreElement(
    const char *_name, XmlContext *_context) :
    XmlElement(_name, _context)
{}

XmlElement *XmlIgnoreElement::ignoreElement() {
    if (!ignoreElement_)
	ignoreElement_ = new XmlIgnoreElement(wildcard, 0);
    return ignoreElement_ ;
}

void XmlIgnoreElement::start(const XmlAttributes atts) {}

void XmlIgnoreElement::end()  {}

XmlElement* XmlIgnoreElement::element(const char *name) {
    return ignoreElement();
}

void XmlIgnoreElement::characters(const char *ch, int len) {}

// ===========================================================================
XmlFlattenElement::XmlFlattenElement(
    const char *_name, XmlContext *_context) :
    XmlElement(_name, _context)
{
    require(parent());
}

void XmlFlattenElement::start(const XmlAttributes atts) {}

void XmlFlattenElement::end() {}

void XmlFlattenElement::characters(const char *ch, int len) {
    parent()->characters(ch, len);
}

XmlElement* XmlFlattenElement::element(const char *name) {
    return parent()->element(name);
}

// ===========================================================================
XmlFlattenElementRelay::XmlFlattenElementRelay(
    const char *_name, XmlContext *_context,
    XmlContext::StartHandler      _startHandler,
    XmlContext::EndHandler        _endHandler) :
    XmlFlattenElement(_name, _context)
{
    startHandler_       = _startHandler;
    endHandler_         = _endHandler;
}

void XmlFlattenElementRelay::start(const XmlAttributes atts) {
    if (startHandler_)
	(context()->*startHandler_)(atts) ;
}

void XmlFlattenElementRelay::end() {
    if (endHandler_)
	(context()->*endHandler_)() ;
}

// ===========================================================================
XmlParentElement::XmlParentElement(
    const char *_name, XmlContext *_context) :
    XmlElement(_name, _context)
{
    unexpectedElement_ = 0;
}

void XmlParentElement::flattenUnknownElements() {
    require(!unexpectedElement_);
    unexpectedElement_ = collect(new XmlFlattenElement(wildcard, this));
}

void XmlParentElement::ignoreUnknownElements() {
    require(!unexpectedElement_);
    unexpectedElement_ = collect(new XmlIgnoreElement(wildcard, this));
}

XmlElement* XmlParentElement::unexpectedElement() {
    return unexpectedElement_;
}

// ===========================================================================
XmlMixedElement::XmlMixedElement(
    const char *_name, XmlContext *_context) :
    XmlParentElement(_name, _context)
{}


XmlMixedElement::XmlMixedElement(
    const char *_name, XmlContext *_context,
    XmlElement *_child,  ...) :
    XmlParentElement(_name, _context)
{
    va_list ap ;

    if (_child) {
	addChild(_child);
	va_start(ap, _child) ;
	vAddChildren(ap);
	va_end(ap) ;
    }
}

XmlMixedElement::~XmlMixedElement() {
}

void XmlMixedElement::addChild(XmlElement *decl) {
    children_.push_back(decl) ;
}

void XmlMixedElement::vAddChildren(va_list ap) {
    while (XmlElement *decl = va_arg(ap, XmlElement::pointer)) {
	require(dynamic_cast<XmlElement*>(decl)); // ensure proper type
	addChild(decl);
    }
}

XmlElement* XmlMixedElement::element(const char *_name) {
    std::vector<XmlElement*>::const_iterator i ;
    for (i = children_.begin() ; i != children_.end() ; ++i) {
	if ((*i)->matches(_name))
	    return (*i) ;
    }
    parser()->warnAboutUnexpectedElement(_name, name());
    return unexpectedElement();
}

// ===========================================================================
XmlMixedElementRelay::XmlMixedElementRelay(
    const char *_name, XmlContext *_context,
    XmlContext::StartHandler      _startHandler,
    XmlContext::EndHandler        _endHandler,
    XmlContext::CharactersHandler _charactersHandler) :
    XmlMixedElement(_name, _context)
{
    startHandler_       = _startHandler;
    endHandler_         = _endHandler;
    charactersHandler_  = _charactersHandler;
}

XmlMixedElementRelay::XmlMixedElementRelay(
    const char *_name, XmlContext *_context,
    XmlContext::StartHandler      _startHandler,
    XmlContext::EndHandler        _endHandler,
    XmlContext::CharactersHandler _charactersHandler,
    XmlElement *_child, ...) :
    XmlMixedElement(_name, _context)
{
    va_list ap ;

    startHandler_       = _startHandler ;
    endHandler_         = _endHandler ;
    charactersHandler_  = _charactersHandler ;

    if (_child) {
	addChild(_child);
	va_start(ap, _child);
	vAddChildren(ap);
	va_end(ap);
    }
}

void XmlMixedElementRelay::start(const XmlAttributes atts) {
    if (startHandler_)
	(context()->*startHandler_)(atts) ;
}

void XmlMixedElementRelay::end() {
    if (endHandler_)
	(context()->*endHandler_)() ;
}

void XmlMixedElementRelay::characters(const char *ch, int len) {
    if (charactersHandler_)
	(context()->*charactersHandler_)(ch, len) ;
}

// ===========================================================================


XmlRegularElement::XmlRegularElement(
    const char *_name, XmlContext *_context) :
    XmlParentElement(_name, _context)
{
    state_ = final;
}

void XmlRegularElement::addTransition(State from, State to, XmlElement *child) {
    require(child);
    Transition nt;
    nt.from  = from;
    nt.to    = to;
    nt.child = child;
    transitions_.push_back(nt);
}

void XmlRegularElement::addFinalState(State state) {
    Transition nt;
    nt.from  = state;
    nt.to    = final;
    nt.child = 0;
    transitions_.push_back(nt);
}

void XmlRegularElement::start(const XmlAttributes atts) {
    require(state_ == final); // prevent circular activation
    state_ = initial;
}

XmlElement* XmlRegularElement::element(const char *_name) {
    TransitionList::const_iterator t;
    for (t = transitions_.begin() ; t != transitions_.end() ; ++t) {
	if (t->from == state_ && t->child && t->child->matches(_name)) {
	    state_ = t->to;
	    return t->child;
	}
    }
    parser()->warnAboutUnexpectedElement(_name, name());
    return unexpectedElement();
}

void XmlRegularElement::characters(const char *ch, int len) {
    if (std::string((char*) ch, len).find_first_not_of(utf8::whitespace) != std::string::npos) {
	// validity error
	parser()->error("Element \"%s\" is declared to contain no character data",
			name()) ;
    }
}

void XmlRegularElement::end() {
    TransitionList::const_iterator t;
    for (t = transitions_.begin() ; t != transitions_.end() ; ++t) {
	if (t->from == state_ && !t->child && t->to == final) {
	    state_ = final;
	    return; // OK
	}
    }

    std::ostringstream os;
    for (t = transitions_.begin() ; t != transitions_.end() ; ++t) {
	if (t->from == state_ && t->child)
	    os << " \"" << t->child->name() << "\"";
    }
/*
    draw(os);
*/
    os << '\0';
    parser()->error("Premature end of regular element \"%s\"", name())
	<< " expected one of" << os.str() << ".";
    state_ = final;
}

void XmlRegularElement::draw(std::ostream &os) const {
    TransitionList::const_iterator t;
    os << "digraph \"" << name() << "\" {" << std::endl
       << "rankdir=LR;" << std::endl
       << initial << " [style=bold];" << std::endl;
    for (t = transitions_.begin() ; t != transitions_.end() ; ++t) {
	if (t->child) {
	    os << t->from << " -> " << t->to << " [label=\""
	       << t->child->name() << "\"];" << std::endl;
	} else {
	    os << t->from << " [peripheries=2];" << std::endl;
	}
    }
    os << "}" << std::endl;
}

// ===========================================================================
XmlRegularElementRelay::XmlRegularElementRelay(
    const char *_name, XmlContext *_context,
    XmlContext::StartHandler      _startHandler,
    XmlContext::EndHandler        _endHandler) :
    XmlRegularElement(_name, _context)
{
    startHandler_ = _startHandler;
    endHandler_   = _endHandler;
}

void XmlRegularElementRelay::start(const XmlAttributes atts) {
    Predecessor::start(atts);
    if (startHandler_)
	(context()->*startHandler_)(atts) ;
}

void XmlRegularElementRelay::end() {
    Predecessor::end();
    if (endHandler_)
	(context()->*endHandler_)() ;
}

// ===========================================================================
void XmlSchemaParser::startDocument() {
    require(root()) ;

    verify(parse_stack_.empty()) ;

    parse_stack_.push(this) ;
    parse_stack_.top()->start(XmlAttributes(0)) ;
}

void XmlSchemaParser::endDocument() {
    verify(!parse_stack_.empty());
    if (parse_stack_.top() == this) {
	parse_stack_.top()->end() ;
	parse_stack_.pop() ;
    } else {
	error("Document incomplete: root element not closed");
    }
}

void XmlSchemaParser::startElement(
    const char *_name, const XmlAttributes atts)
{
    verify(!parse_stack_.empty());
    XmlElement *next = parse_stack_.top()->element(_name) ;
    if (next) {
	require(next->parser() == this || !next->parser());
	next->start(atts) ;
	parse_stack_.push(next) ;
    } else {
	// validity error
	error("Cannot handle start tag: \"%s\"", _name);
	parse_stack_.push(XmlIgnoreElement::ignoreElement()) ;
    }
}

void XmlSchemaParser::endElement(const char *name) {
    verify(!parse_stack_.empty());
    if (parse_stack_.top()->matches(name)) {
	parse_stack_.top()->end() ;
	parse_stack_.pop() ;
    } else {
	// This error will actually be caught by libxml.
	// well-formedness error
	error("Found mismatched closing tag: \"%s\" instead of \"%s\"",
	      name, parse_stack_.top()->name()) ;
    }
}

void XmlSchemaParser::characters(const char *ch, int len) {
    verify(!parse_stack_.empty());
    parse_stack_.top()->characters(ch, len) ;
}

XmlSchemaParser::XmlSchemaParser(const Configuration &c) :
    XmlParser(c),
    XmlDocumentElement()
{}
