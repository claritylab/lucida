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
// $Id: XmlBuilder.hh 6223 2006-11-14 17:01:38Z rybach $

#ifndef _CORE_XML_BUILDER_HH
#define _CORE_XML_BUILDER_HH

#include <memory>
#include <sstream>
#include "Assertions.hh"
#include "XmlParser.hh"
#include <string>

namespace Core {
    template <class CM, typename P, typename PH, typename HA>
    class XmlBuilderElementTemplate :
	public CM
    {
    public:
	typedef CM Precursor;
	typedef CM ContentModel;
	typedef P Product;
	typedef PH ProductHolder;
	typedef HA HandlerArgument;

	typedef void (XmlContext::*Handler)(HandlerArgument);
	template <class T> static Handler
	handler(void (T::*h)(HandlerArgument)) {
	    return static_cast<Handler>(h);
	}

    protected:
	Handler handler_;
	ProductHolder product_;

    public:
	virtual void start(const XmlAttributes atts) {
	    Precursor::start(atts);
	}

	virtual void end() {
	    Precursor::end();
	    if (handler_)
		(this->context()->*handler_)(product_);
	}

	XmlBuilderElementTemplate(
	    const char *_name, XmlContext *_context, Handler _handler = 0) :
	    Precursor(_name, _context), handler_(_handler) {}
    };

    struct CreateStatic {};
    struct CreateUsingNew {};
    struct CreateByContext {};

    /**
     * \warning An XmlBuilderElement cannot be an (indirect) child of itself.
     */
    template <typename P, class CM, class CP>
    class XmlBuilderElement;


    template <typename P, class CM>
    class XmlBuilderElement<P, CM, CreateStatic>:
	public XmlBuilderElementTemplate<CM, P, P, const P&>
    {
	typedef XmlBuilderElementTemplate<CM, P, P, const P&> Precursor;
    public:
	typedef const P& HandlerArgument;
	typedef void (XmlContext::*Handler)(HandlerArgument);
	typedef P Product;

	virtual void start(const XmlAttributes atts) {
	    Precursor::product_ = Product();
	    Precursor::start(atts);
	}

	virtual void end() {
	    Precursor::end();
	    Precursor::product_ = Product();
	}

	XmlBuilderElement(
	    const char *_name, XmlContext *_context, Handler _handler = 0) :
	    Precursor(_name, _context, _handler) {}
    };


    template <typename P, class CM>
    class XmlBuilderElement<P, CM, CreateUsingNew>:
	public XmlBuilderElementTemplate<CM, P, std::auto_ptr<P>, std::auto_ptr<P>&>
    {
	typedef XmlBuilderElementTemplate<CM, P, std::auto_ptr<P>, std::auto_ptr<P>&>
	Precursor;
    public:
	typedef std::auto_ptr<P>& HandlerArgument;
	typedef void (XmlContext::*Handler)(HandlerArgument);
	typedef P Product;

	virtual void start(const XmlAttributes atts) {
	    require(!Precursor::product_.get()); // prevent circular activation
	    Precursor::product_.reset(new Product());
	    Precursor::start(atts);
	}

	virtual void end() {
	    Precursor::end();
	    Precursor::product_.reset();
	}

	XmlBuilderElement(
	    const char *_name, XmlContext *_context, Handler _handler = 0) :
	    Precursor(_name, _context, _handler) {}
    };


    template <typename P, class CM>
    class XmlBuilderElement<P, CM, CreateByContext> :
	public XmlBuilderElementTemplate<CM, P, P*, P*>
    {
	typedef XmlBuilderElementTemplate<CM, P, P*, P*> Precursor;
    public:
	typedef P* CreationHandlerResult;
	typedef CreationHandlerResult
	(XmlContext::*CreationHandler)(const XmlAttributes atts);
	template <class T> static CreationHandler
	creationHandler(CreationHandlerResult (T::*h)(const XmlAttributes atts)) {
	    return static_cast<CreationHandler>(h);
	}

    protected:
	CreationHandler creationHandler_;

    public:
	typedef P* HandlerArgument;
	typedef void (XmlContext::*Handler)(HandlerArgument);
	virtual void start(const XmlAttributes atts) {
	    require(!Precursor::product_); // prevent circular activation
	    Precursor::product_ = (this->context()->*creationHandler_)(atts);
	    Precursor::start(atts);
	}

	virtual void end() {
	    Precursor::end();
	    Precursor::product_ = 0;
	}

	XmlBuilderElement(
	    const char *_name, XmlContext *_context,
	    CreationHandler _creationHandler, Handler _handler = 0) :
	    Precursor(_name, _context, _handler),
	    creationHandler_(_creationHandler)
	{
	    require(_creationHandler);
	    Precursor::product_ = 0;
	}
    };

    /**
     * XML element declaration for text only elements.
     */
    /*
    class XmlStringBuilderElement :
	public XmlBuilderElement<std::string, XmlEmptyElement, CreateStatic>
    {
	typedef XmlBuilderElement<std::string, XmlEmptyElement, CreateStatic> Precursor;
    public:

	virtual void characters(const char *ch, int len);
	XmlStringBuilderElement(
	    const char *_name, XmlContext *_context, Handler _handler = 0) :
	    Precursor(_name, _context, _handler) {}
    };
    */

    template <typename P>
    struct StringConversion {
	bool operator()(const std::string &in, P &out) const {
	    return strconv(in, out);
	}
    };

    template <>
    struct StringConversion<std::string> {
	bool operator()(const std::string &in, std::string &out) const {
	    out = in;
	    return true;
	}
    };

    template <typename P, class C = StringConversion<P> >
    class XmlDataOnlyBuilderElement :
	public XmlBuilderElementTemplate<XmlEmptyElement, P, P, const P&>
    {
	typedef XmlBuilderElementTemplate<XmlEmptyElement, P, P, const P&> Precursor;
	typedef typename Precursor::Handler Handler;
    public:
	typedef C Conversion;
    private:
	std::string str_;
    protected:
	Conversion converter_;

	XmlDataOnlyBuilderElement(const char *_name, XmlContext *_context, Handler _handler, Conversion _converter) :
	    Precursor(_name, _context, _handler),
	    converter_(_converter) {}

    public:
	virtual void start(const XmlAttributes atts) {
	    XmlBuilderElementTemplate<XmlEmptyElement, P, P, const P&>::Precursor::start(atts);
	    str_ = std::string();
	}

	virtual void characters(const char *ch, int len) {
	    str_.append((char*) ch, len);
	}

	virtual void end() {
	    if (!converter_(str_, Precursor::product_)) {
		this->parser()->error("In element \"%s\": non-interpretable value \"%s\"",
				this->name(), str_.c_str());
	    }
	    Precursor::end();
	    Precursor::product_ = typename Precursor::Product();
	}

	XmlDataOnlyBuilderElement(const char *_name, XmlContext *_context, typename Precursor::Handler _handler = 0) :
	    Precursor(_name, _context, _handler) {}
    };



    template<typename P>
    struct ChoiceConversion {
	const Choice choice;

	bool operator() (const std::string &in, P &out) const {
	    return strconv(in, out, choice);
	}

	ChoiceConversion(const Choice & choice):
	    choice(choice) {}
    };

    template<typename P>
    class XmlChoiceBuilderElement :
	public XmlDataOnlyBuilderElement<P, ChoiceConversion<P> > {
    protected:
	typedef XmlChoiceBuilderElement Self;
	typedef XmlDataOnlyBuilderElement<P, ChoiceConversion<P> > Precursor;
	typedef typename Precursor::Handler Handler;

    public:
	XmlChoiceBuilderElement(
	    const char *_name,
	    XmlContext *_context,
	    const Choice & _choice,
	    Handler _handler = 0
	    ) :
	    Precursor(_name, _context, _handler, ChoiceConversion<P>(_choice)) {}

	const Choice & choice() {
	    return Precursor::converter_.choice;
	}
    };

    typedef XmlDataOnlyBuilderElement<std::string> XmlStringBuilderElement;
    typedef XmlDataOnlyBuilderElement<bool>        XmlBooleanBuilderElement;
    typedef XmlDataOnlyBuilderElement<u32>         XmlUnsignedBuilderElement;
    typedef XmlDataOnlyBuilderElement<s32>         XmlSignedBuilderElement;
    typedef XmlDataOnlyBuilderElement<f32>         XmlFloatBuilderElement;
    typedef XmlChoiceBuilderElement<Choice::Value> XmlChoiceElement;

} // namespace Core

#endif // _CORE_XML_BUILDER_HH
