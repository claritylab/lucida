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
// $Id: XmlParser.hh 6223 2006-11-14 17:01:38Z rybach $

#ifndef _CORE_XML_PARSER_HH
#define _CORE_XML_PARSER_HH

#include "Component.hh"
#include <cstdarg>
#include <libxml/parser.h>
#include <stack>
#include <vector>

namespace Core {

class XmlElement;

/**
 * XML element attributes.
 *
 * This class is used to pass XML element attributes to handler
 * functions.  It provides two ways of access: by enumerating
 * attributes or by keyword lookup.
 */

class XmlAttributes {
private:
    mutable int size_;
    const xmlChar **atts_ ;
public:
    explicit XmlAttributes(const xmlChar **_atts) {
	size_ = -1;
	atts_ = _atts ;
    } ;

    struct Item {
	const char *key;
	const char *value;
    };

    /** number of attributes */
    int size() const;

    /** Get key and value for the @c i th attribute. */
    Item operator[](int i) const ;

    /** Get attribute by keyword.
     * @param key the attribute name to look for.
     * @return the value assigned to @c key or 0 if there is no such
     * attribute. */
    const char *operator[](const char *key) const ;
} ;


/**
 * Basic event driven XML parser.
 *
 * The XML Parser is built in top of libxml2 (the XML parser for
 * Gnome).  This class provides a thin wrapper around the SAX
 * interface of libxml.  All callback entries are converted into
 * virtual member functions which can (and should) be overridden to
 * implement event driven Xml parsers.
 * @see full documentation of libxml:
 * <a href="file:/usr/share/doc/libxml2-dev/libxml-dev.html">local</a> or
 * <a href="http://xmlsoft.org/">on the web</a>
 */

class XmlParser :
    public Component
{
    typedef XmlParser self ;

private:
    static xmlParserInputPtr SAX_resolveEntity_callback(
	void *ctx, const xmlChar *publicId, const xmlChar *systemId) ;
    static void SAX_internalSubset_callback(
	void *ctx, const xmlChar *name,
	const xmlChar *ExternalID, const xmlChar *SystemID) ;
    static void SAX_externalSubset_callback(
	void *ctx, const xmlChar *name,
	const xmlChar *ExternalID, const xmlChar *SystemID);
    static xmlEntityPtr SAX_getEntity_callback(
	void *ctx, const xmlChar *name);
    static xmlEntityPtr SAX_getParameterEntity_callback(
	void *ctx, const xmlChar *name);
    static void SAX_entityDecl_callback(
	void *ctx,
	const xmlChar *name, int type, const xmlChar *publicId,
	const xmlChar *systemId, xmlChar *content);
    static void SAX_notationDecl_callback(
	void *ctx, const xmlChar *name,
	const xmlChar *publicId, const xmlChar *systemId);
    static void SAX_attributeDecl_callback(
	void *ctx, const xmlChar *elem,
	const xmlChar *name, int type, int def,
	const xmlChar *defaultValue, xmlEnumerationPtr tree);
    static void SAX_elementDecl_callback(
	void *ctx, const xmlChar *name,
	int type, xmlElementContentPtr content);
    static void SAX_unparsedEntityDecl_callback(
	void *ctx,
	const xmlChar *name, const xmlChar *publicId,
	const xmlChar *systemId, const xmlChar *notationName);
//  static void SAX_setDocumentLocator_callback(
//         void *ctx,
//         xmlSAXLocatorPtr loc);
    static void SAX_startDocument_callback(
	void *ctx);
    static void SAX_endDocument_callback(
	void *ctx);
    static void SAX_startElement_callback(
	void *ctx, const xmlChar *name,
	const xmlChar **atts);
    static void SAX_endElement_callback(
	void *ctx, const xmlChar *name);
    static void SAX_reference_callback(
	void *ctx, const xmlChar *name);
    static void SAX_characters_callback(
	void *ctx, const xmlChar *ch, int len);
    static void SAX_cdataBlock_callback(
	void *ctx, const xmlChar *ch, int len);
    static void SAX_ignorableWhitespace_callback(
	void *ctx,
	const xmlChar *ch, int len);
    static void SAX_processingInstruction_callback(
	void *ctx,
	const xmlChar *target, const xmlChar *data);
    static void SAX_comment_callback(void *ctx, const xmlChar *value);
    static void SAX_warning_callback(void *ctx, const char *msg, ...)
	__attribute__ ((format (printf, 2, 3)));
    static void SAX_error_callback(void *ctx, const char *msg, ...)
	__attribute__ ((format (printf, 2, 3)));
    static void SAX_fatalError_callback(void *ctx, const char *msg, ...)
	__attribute__ ((format (printf, 2, 3)));
    static int SAX_isStandalone_callback(void *ctx);
    static int SAX_hasInternalSubset_callback(void *ctx);
    static int SAX_hasExternalSubset_callback(void *ctx);

    static const xmlSAXHandler SAX_callbacks ;
    xmlParserCtxtPtr ctxt_ ;
//  xmlSAXLocatorPtr document_locator_ ;

    /** Reads a chunk of size @param bufferSize from @param is and
     *  stores the result in @param buffer.
     *  @return number of read characters
     */
    size_t readChunk(std::istream &is, char *buffer, size_t bufferSize);

protected:
    virtual int isStandalone() ;
    virtual int hasInternalSubset() ;
    virtual int hasExternalSubset() ;
    virtual xmlParserInputPtr resolveEntity(
	const char *publicId, const char *systemId) ;
    virtual void internalSubset(
	const char *name, const char *ExternalID, const char *SystemID) ;
    virtual void externalSubset(
	const char *name, const char *ExternalID, const char *SystemID) ;
    virtual xmlEntityPtr getEntity(
	const char *name);
    virtual xmlEntityPtr getParameterEntity(
	const char *name);
    virtual void entityDecl(
	const char *name, int type, const char *publicId,
	const char *systemId, const char *content);
    virtual void notationDecl(
	const char *name, const char *publicId, const char *systemId);
    virtual void attributeDecl(
	const char *elem, const char *name, int type, int def,
	const char *defaultValue, xmlEnumerationPtr tree);
    virtual void elementDecl(
	const char *name, int type, xmlElementContentPtr content);
    virtual void unparsedEntityDecl(
	const char *name, const char *publicId,
	const char *systemId, const char *notationName);
//  virtual void setDocumentLocator(xmlSAXLocatorPtr loc);
    virtual void startDocument() ;
    virtual void endDocument() ;
    virtual void startElement(const char *name, const XmlAttributes atts);
    virtual void endElement(const char *name);
    virtual void reference(const char *name);
    virtual void characters(const char *ch, int len);
    virtual void cdataBlock(const char *ch, int len);
    virtual void ignorableWhitespace(const char *ch, int len);
    virtual void processingInstruction(const char *target, const char *data);
    virtual void comment(const char *value);

    int parse();

    /**
     * Output a warning/error message on the appropriate channel.
     * All error message will be augmented by file name and line
     * number where the error occured.
     **/
    virtual XmlChannel *vErrorMessage(ErrorType mt, const char *msg, va_list) const;

    /**
     * Disable parsing.
     * Call giveUp() if it seems hopeless trying to continue parsing.
     **/
    void giveUp();

private:
    bool warnAboutUnexpectedElements_;

public:
    struct Location {
	const char *systemId ;
	int line, column ;
    } ;
    Location location() const ;

    static const ParameterBool paramWarnAboutUnexpectedElements;

    /**
     * Issue a warning about an unexpected element.
     * This method must be called by decendents of XmlParentElement
     * whenever they use their unexpectedElement().  The reason that
     * this deserves an extra method is that these warnings can be
     * turned of.  It is not a good idea to silently ignore unknown
     * element, because they might be typos.
     **/
    void warnAboutUnexpectedElement(const char *element,
				    const char *context);

    XmlParser(const Configuration&) ;
    virtual ~XmlParser() ;

    /** Parse a character string.
     * @return 0 on success
     */
    int parseString(const char *str);

    /** Parse a stream.
     * @return 0 on success
     */
    int parseStream(std::istream &i, const std::string &filename = "from stream");

    /** Parse a file.
     * @return 0 on success
     */
    int parseFile(const char *filename);
} ;


// ===========================================================================

class XmlSchemaParser;

class XmlContext {
public:
    typedef XmlContext Self;

    typedef void (Self::*StartHandler)(const XmlAttributes atts) ;
    typedef void (Self::*EndHandler)() ;
    typedef void (Self::*CharactersHandler)(const char *ch, int len) ;

    template <class T>
    static StartHandler startHandler(void (T::*h)(const XmlAttributes atts)) {
	return static_cast<StartHandler>(h);
    }

    template <class T>
    static EndHandler endHandler(void (T::*h)()) {
	return static_cast<EndHandler>(h);
    }

    template <class T>
    static CharactersHandler charactersHandler(void (T::*h)(const char *ch, int len)) {
	return static_cast<CharactersHandler>(h);
    }

    virtual XmlSchemaParser* parser() = 0;

    virtual ~XmlContext() {}
};

/**
 *  Easy solution for collecting and destroying XmlElements.
 *  Note: once XmlElements will be reference counted, this
 *  class becomes obsolete.
 */
class XmlElementGarbageCollector {
    std::vector<XmlElement*> xmlElements_;
public:
    XmlElementGarbageCollector();
    ~XmlElementGarbageCollector();

    XmlElement * add(XmlElement *e) { xmlElements_.push_back(e); return e; }
};

/**
 * Abstract XML element declaration for use with XmlSchemaParser.
 *
 * Instances of (decendants of) XmlElement determine the behaviour of
 * an XmlSchemaParser (State pattern).  In consequence they can be
 * seen as content models for the various elements of an XML
 * applications. XmlElement serves as an abstract base class for
 * various content models.
 *
 * When a state is entered start() is called with the attributes of
 * the XML start tag which caused the parser to enter this state.
 * When the corresponding end tag is found, end() is called.  In
 * between character data and nested XML elements may occur.  These
 * will cause calls to characters() and element() respectively.
 * element() must return the state to be entered for the the
 * respective element.
 *
 * @see XmlSchemaParser */

class XmlElement :
    public XmlContext
{
public:
    typedef XmlElement *pointer;
    static const char *wildcard;
private:
    XmlContext *context_;
    const char *name_ ;
    XmlElementGarbageCollector garbageCollector_;
protected:
    /**
     *  Collects child elements.
     *  Call this function with the newly created child elements
     *  in order to collect and destroy them if they are not needed anymore.
     */
    XmlElement *collect(XmlElement *child) { return garbageCollector_.add(child); }
public:
    XmlElement(const char *_name, XmlContext *_context) ;

    const char *name() const { return name_ ; } ;

    XmlContext *context() const { return context_ ; }
    virtual XmlSchemaParser* parser() {
	return (context()) ? context()->parser() : 0;
    }

    bool matches(const char *_name) const;

    virtual ~XmlElement() {} ;

    /**
     * Handle start of the current XML element.
     * start() is called when the current state is entered.
     *
     * @param atts attributes of the XML start tag which caused the parser
     * to enter this state.  The name of the element will generally be
     * implied by the fact that THIS state is entered.
     */
    virtual void start(const XmlAttributes atts) {}

    /**
     * Handle end of the current XML element.
     * end() is called when end tag of the current element is found.
     *
     * @param parser The XmlSchemaParser delegating the request.
     */
    virtual void end() {}

    /**
     * Handle charcter data in the current XML element.
     *
     * @param parser The XmlSchemaParser delegating the request.
     */
    virtual void characters(const char *ch, int len) = 0 ;

    /**
     * Determine the state to be entered for a nested XML element.
     * @param  name of the XML element encountered
     * @return XmlElement to handle the encountered element, or 0 if
     * the element should not be allowed.
     *
     * @param parser The XmlSchemaParser delegating the request.
     */
    virtual XmlElement* element(const char *name) = 0 ;
} ;

/**
 * XML Element declaration for the empty content model.
 *
 * XmlEmptyElement must not have any content.  characters() and
 * element() will raise a validity error
*/
class XmlEmptyElement :
    public XmlElement
{
    bool allowWhitespace_;
public:
    virtual void end() ;
    virtual void characters(const char *ch, int len) ;
    virtual XmlElement* element(const char *name) ;

    XmlEmptyElement(const char *_name, XmlContext *_context,
		    bool allowWhitespace = true);
} ;

class XmlEmptyElementRelay :
    public XmlEmptyElement
{
private:
    XmlContext::StartHandler startHandler_ ;
public:
    virtual void start(const XmlAttributes atts) ;
    XmlEmptyElementRelay(
	const char *_name, XmlContext *_context,
	XmlContext::StartHandler _startHandler = 0);
};


/**
 * XML Element declarator base class for elments with child elements.
 */

class XmlParentElement :
    public XmlElement
{
protected:
    XmlElement *unexpectedElement_;
    virtual XmlElement *unexpectedElement();

public:
    XmlParentElement(const char *_name, XmlContext *_context) ;

    void flattenUnknownElements();
    void ignoreUnknownElements();
};


/**
 * Helper class.used for ignoring unknown elements.
 *
 * XmlIgnoreElement will ingore all character data and nested
 * elements.  The wildcard XmlIgnoreElement is available as a Singleton
 * via ingoreElement().
 */

class XmlIgnoreElement :
    public XmlElement
{
private:
    static XmlElement *ignoreElement_ ;
public:
    static XmlElement *ignoreElement() ;
public:
    virtual void start(const XmlAttributes atts) ;
    virtual void end() ;
    virtual void characters(const char *ch, int len) ;
    virtual XmlElement* element(const char *name) ;

    XmlIgnoreElement(const char *_name, XmlContext *_context = 0) ;
} ;


/**
 * Helper class.used for flattening unknown elements.
 *
 * XmlFlattenElement will redirect all character and element events to
 * @c context_.  It will thus appear as if the unknown element's
 * content was included in the parent directly.
 */

class XmlFlattenElement :
    public XmlElement
{
protected:
    XmlParentElement *parent() const {
	return static_cast<XmlParentElement*>(context());
    }
public:
    virtual void start(const XmlAttributes atts);
    virtual void end() ;
    virtual void characters(const char *ch, int len);
    virtual XmlElement* element(const char *name);

    XmlFlattenElement(const char *_name, XmlContext *_context);
};


class  XmlFlattenElementRelay :
    public XmlFlattenElement
{
private:
    XmlContext::StartHandler      startHandler_ ;
    XmlContext::EndHandler        endHandler_ ;

public:
    virtual void start(const XmlAttributes atts) ;
    virtual void end() ;

    XmlFlattenElementRelay(
	const char *_name, XmlContext *_context,
	XmlContext::StartHandler      = 0,
	XmlContext::EndHandler        = 0);
};

/**
 * Helper class used by XmlSchemaParser when outside the document's
 * root element.
 */

class XmlDocumentElement :
    public XmlParentElement
{
private:
    XmlElement *root_ ;
    bool hasRootOccured_;
protected:
    void setRoot(XmlElement*);
public:
    virtual void start(const XmlAttributes atts) ;
    virtual void end() ;
    virtual void characters(const char *ch, int len) ;
    virtual XmlElement* element(const char *name) ;

    const XmlElement *root() const { return root_; }
    XmlDocumentElement(XmlElement *_root = 0) ;
} ;


/**
 * XML element declaration for the mixed content model.
 *
 * XmlMixedElement allows character data interspersed with the
 * elements declared in children_.  Neither order nor number of
 * elements and character data is restricted.
 */

class XmlMixedElement :
    public XmlParentElement
{
protected:
    std::vector<XmlElement*> children_ ;
    void vAddChildren(va_list);

public:
    virtual XmlElement* element(const char *name);

    XmlMixedElement(const char *_name, XmlContext *_context) ;

    /**
     * Constructor.
     *
     * @param ... List of const XmlElement* element declarations which
     * may occur within the current element.  The list must be
     * terminated with XML_NO_MORE_CHILDREN and each item must
     * be explicitly type-marked with XML_CHILD.  */

    XmlMixedElement(const char *_name, XmlContext *_context,
		    XmlElement *child, ...);

#define XML_CHILD(child) (static_cast<XmlElement*>(child))
#define XML_NO_MORE_CHILDREN ((XmlElement*)0)

    virtual ~XmlMixedElement();

    /**
     * Add a new child element.
     * @param decl XML element declaration to be used when an
     * XML element of the same name occurs within the current element.
     */
    void addChild(XmlElement *decl) ;

} ;

class  XmlMixedElementRelay :
    public XmlMixedElement
{
private:
    XmlContext::StartHandler      startHandler_ ;
    XmlContext::EndHandler        endHandler_ ;
    XmlContext::CharactersHandler charactersHandler_ ;

public:
    virtual void start(const XmlAttributes atts) ;
    virtual void end() ;
    virtual void characters(const char *ch, int len);

    XmlMixedElementRelay(
	const char *_name, XmlContext *_context,
	XmlContext::StartHandler      = 0,
	XmlContext::EndHandler        = 0,
	XmlContext::CharactersHandler = 0);

    XmlMixedElementRelay(
	const char *_name, XmlContext *_context,
	XmlContext::StartHandler,
	XmlContext::EndHandler,
	XmlContext::CharactersHandler,
	XmlElement *child, ...);
};


/**
 * XML element declaration for the regular content model.
 *
 * XML elements following the regular content model have child
 * elements according to a regular language.  (Character data is not
 * allowed.)  In the DTD this language is specified by a regular
 * expression.  XmlRegularElement does (currently) not provide regular
 * expression parsing.  Instead the language must be specified as a
 * deterministic, epsilon-free finite state automaton.  The
 * construction of this automaton is left to the user.  States are
 * identified with integers.  The initial state is @c initial, any
 * state may be marked as final with addFinalState(). Transitions can
 * be defined using addTransition().  Remember that the automaton must
 * be deterministic, since this is not checked for and can lead to
 * unexpected results!
 *
 * \warning An XmlRegularElement cannot be an (indirect) child of itself.
 */

class XmlRegularElement :
    public XmlParentElement
{
public:
    typedef int State;

    static const State initial =  0;
    static const State final   = -1;

private:
    struct Transition {
	State from, to;
	XmlElement *child;
    };
    typedef std::vector<Transition> TransitionList;

    State state_;
    TransitionList transitions_;

public:
    XmlRegularElement(const char *_name, XmlContext *_context);

    virtual void start(const XmlAttributes atts) ;
    virtual void end() ;
    virtual void characters(const char *ch, int len);
    virtual XmlElement* element(const char *name);

    /** Add a transition to the automaton. */
    void addTransition(State from, State to, XmlElement *child);

    /** Mark state as final. */
    void addFinalState(State state);

    /** Write automaton in Graphviz Dot format.  (for debugging
     * purposes mainly.) */
    void draw(std::ostream&) const;
} ;

class XmlRegularElementRelay :
    public XmlRegularElement
{
    typedef XmlRegularElement Predecessor;
private:
    XmlContext::StartHandler startHandler_;
    XmlContext::EndHandler   endHandler_;

public:
    virtual void start(const XmlAttributes atts) ;
    virtual void end() ;

    XmlRegularElementRelay(
	const char *_name, XmlContext *_context,
	XmlContext::StartHandler      = 0,
	XmlContext::EndHandler        = 0);
};


/**
 * Stack based XML parser.
 * This class should be used for deriving application specific XML
 * parsers.  It enhances XmlParser by providing means for reacting
 * according to the current position in the XML element hierarchy.
 *
 * Design: When writing an event driven parser (e.g. based on the SAX
 * interface, by deriving from XmlParser) you will quickly find
 * yourself introducing state variables and writing complicated
 * callback functions with many case distinction.  Following
 * "Design Patterns" (<a href="http://www-bib.informatik.rwth-aachen.de/cgi-bin/bibquery.cgi?nr=800815">available in library</a>)
 * we apply the State pattern to concentrate the state dependent
 * behaviour and make transitions between states more evident.
 * Participants:
 * - Conext: XmlSchemaParser
 * - State: XmlElement
 *
 * Since XML applications are context free languages we have to
 * implement a push-down automaton.  For this reason the state of the
 * parser is not defined by a single XmlElement, but by a stack of
 * XmlElements.
 *
 * Instances of (decendents of) XmlElement can be seen as content
 * models for the various elements of an XML applications.  Most
 * predefined content models use method pointers to re-delegate work
 * back to the (applications specific) parser.  Note that these
 * callback methods are state dependent; the whole state
 * discrimination and transition business is already take care of.
 *
 * @see XmlElement
 */

class XmlSchemaParser :
    public XmlParser,
    public XmlDocumentElement
{
    typedef XmlSchemaParser Self ;
private:
    std::stack<XmlElement*> parse_stack_ ;

protected:
    virtual void startDocument() ;
    virtual void endDocument() ;
    virtual void startElement(const char *name, const XmlAttributes atts);
    virtual void endElement(const char *name);
    virtual void characters(const char *ch, int len) ;

    XmlSchemaParser(const Configuration&) ;
public:
    virtual XmlSchemaParser* parser() { return this; }
} ;


} // namespace Core


#endif // _CORE_XML_PARSER_HH
