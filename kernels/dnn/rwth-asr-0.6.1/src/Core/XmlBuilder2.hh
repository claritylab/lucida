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
#ifndef _CORE_XML_BUILDER2_HH
#define _CORE_XML_BUILDER2_HH

#include <Core/Assertions.hh>
#include <Core/Choice.hh>
#include <Core/Delegation.hh>
#include <Core/StringUtilities.hh>
#include <Core/XmlParser.hh>

namespace Core {

    template<typename T>
    class BuildDelegation :
	protected Delegation<const T &>,
	protected Delegation<T *> {

    public:
	typedef Delegation<const T &> ParsedDelegation;
	typedef Delegation<T *>       CreatedDelegation;

	typedef typename ParsedDelegation::Target  ParsedHandler;
	typedef typename CreatedDelegation::Target CreatedHandler;

    protected:
	/**
	  Calls first parsed handler and then created handler.
	**/
	void delegate(T *t);

	/**
	  Calls first parsed handler and then created handler;
	  if no created handler exists, t is deleted
	**/
	void delegateOrDelete(T *t);

	/**
	  Calls first parsed handler and then created handler;
	  return t if no created handler exists, else
	  return a newly created instance of T .
	**/
	T *delegateAndCreate(T *t);

	/**
	  Calls parsed handler.
	**/
	bool delegateParsed(const T &t);

	/**
	  Calls created handler.
	**/
	bool delegateCreated(T *t);

    public:
	void setParsedHandler(ParsedHandler *handler) {
	    ParsedDelegation::set(handler);
	}

	template<class HandlerClass>
	void setParsedHandler(HandlerClass &handler, void (HandlerClass::*deleg)(const T &)) {
	    ParsedDelegation::set(handler, deleg);
	}

	void resetParsedHandler() {
	    ParsedDelegation::reset();
	}

	void setCreatedHandler(CreatedHandler *handler) {
	    CreatedDelegation::set(handler);
	}

	template<class HandlerClass>
	void setCreatedHandler(HandlerClass &handler, void (HandlerClass::*deleg)(T *)) {
	    CreatedDelegation::set(handler, deleg);
	}

	void resetCreatedHandler() {
	    CreatedDelegation::reset();
	}
    };


} // namespace Core

namespace XmlBuilder2 {

    template<typename T>
    struct StringConversion {
	bool operator()(const std::string &in, T &out) const {
	    return Core::strconv(in, out);
	}
    };

    template<typename T, class Converter = StringConversion<T> >
    class XmlDataOnlyBuilderElement :
	public Core::XmlEmptyElement,
	public Core::BuildDelegation<T> {

    private:
	std::string cdata_;
	Converter conv_;
	T *t_;

    public:
	XmlDataOnlyBuilderElement(const char *name, XmlContext *context, const Converter &conv = Converter()) :
	    XmlEmptyElement(name, context),
	    cdata_(),
	    conv_(conv),
	    t_(new T()) {}
	~XmlDataOnlyBuilderElement() {
	    delete t_;
	}

	void characters(const char *ch, int len) {
	    cdata_.append(ch, len);
	}

	void end();
    };


    template<>
    class XmlDataOnlyBuilderElement<std::string, StringConversion<std::string> > :
	public Core::XmlEmptyElement,
	public Core::BuildDelegation<std::string> {

    private:
	std::string cdata_;

    public:
	XmlDataOnlyBuilderElement(const char *name, XmlContext *context) :
	    XmlEmptyElement(name, context) {}

	void characters(const char *ch, int len) {
	    cdata_.append(ch, len);
	}

	void end() {
	    delegateParsed(cdata_);
	    if (CreatedDelegation::hasTarget())
		CreatedDelegation::delegate(new std::string(cdata_));
	    cdata_.clear();
	}
    };


    typedef XmlDataOnlyBuilderElement<std::string> XmlStringBuilderElement;
    typedef XmlDataOnlyBuilderElement<bool>        XmlBooleanBuilderElement;
    typedef XmlDataOnlyBuilderElement<u32>         XmlUnsignedBuilderElement;
    typedef XmlDataOnlyBuilderElement<s32>         XmlSignedBuilderElement;
    typedef XmlDataOnlyBuilderElement<f64>         XmlFloatBuilderElement;


    class XmlChoiceBuilderElement :
	public Core::XmlEmptyElement,
	public Core::BuildDelegation<Core::Choice::Value> {

    private:
	const Core::Choice *choice_;
	std::string cdata_;

    public:
	XmlChoiceBuilderElement(const char *name, Core::XmlContext *context, const Core::Choice *choice) :
	    XmlEmptyElement(name, context),
	    choice_(choice),
	    cdata_() {}

	void characters(const char *ch, int len) {
	    cdata_.append(ch, len);
	}

	void end() {
	    Core::stripWhitespace(cdata_);
	    delegateParsed((*choice_)[cdata_]);
	    if (CreatedDelegation::hasTarget())
		CreatedDelegation::delegate(new Core::Choice::Value((*choice_)[cdata_]));
	    cdata_.clear();
	}
    };

} // namespace XmlBuilder2


template<typename T>
void Core::BuildDelegation<T>::delegate(T *t) {
    if (ParsedDelegation::hasTarget())
	ParsedDelegation::delegate(*t);
    if (CreatedDelegation::hasTarget())
	CreatedDelegation::delegate(t);
}

template<typename T>
void Core::BuildDelegation<T>::delegateOrDelete(T *t) {
    if (ParsedDelegation::hasTarget())
	ParsedDelegation::delegate(*t);
    if (CreatedDelegation::hasTarget())
	CreatedDelegation::delegate(t);
    else
	delete t;
}

template<typename T>
T *Core::BuildDelegation<T>::delegateAndCreate(T *t) {
    if (ParsedDelegation::hasTarget()) {
	ParsedDelegation::delegate(*t);
    }
    if (CreatedDelegation::hasTarget()) {
	CreatedDelegation::delegate(t);
	return new T();
    } else {
	return t;
    }
}

template<typename T>
bool Core::BuildDelegation<T>::delegateParsed(const T &t) {
    if (ParsedDelegation::hasTarget()) {
	ParsedDelegation::delegate(t);
	return true;
    } else
	return false;
}

template<typename T>
bool Core::BuildDelegation<T>::delegateCreated(T *t) {
    if (CreatedDelegation::hasTarget()) {
	CreatedDelegation::delegate(t);
	return true;
    } else
	return false;
}


template<typename T, class Converter>
void XmlBuilder2::XmlDataOnlyBuilderElement<T, Converter>::end() {
    if (conv_(cdata_, *t_))
	t_ = Core::BuildDelegation<T>::delegateAndCreate(t_);
    else
	parser()->error("In element \"%s\": non-interpretable value \"%s\"",
			name(), cdata_.c_str());
    cdata_.clear();
}


#endif // _CORE_XML_BUILDER2_HH
