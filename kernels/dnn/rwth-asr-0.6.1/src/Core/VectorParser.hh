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
#ifndef _CORE_VECTOR_PARSER_HH
#define _CORE_VECTOR_PARSER_HH

#include <iterator>
#include "XmlBuilder.hh"

namespace Core {

    /** XmlVectorElement implements XmlBuilderElement for an arbitrary XmlSchemaParser
     *  which parses a vector of a given type.
     *
     *  Vector object is of type std::vector<T>
     *  XML tag name: vector-type. E.g.: vector-f32
     *
     *  Elements should be stored separated by whitespace characters.
     *  Remark: end-of-line character does not have any special role.
     */
    template<class T>
    class XmlVectorElement :
	public XmlBuilderElement<std::vector<T>,
				 XmlElement,
				 CreateByContext>
    {
    public:

	typedef XmlBuilderElement<std::vector<T>,
				  XmlElement,
				  CreateByContext> Predecessor;
	typedef XmlVectorElement<T> Self;
	typedef std::vector<T>* (XmlContext::*CreationHandler)(const XmlAttributes atts);

    private:

	bool sizeGiven_;
	typename std::vector<T>::size_type size_;

	std::string buffer_;

    protected:

	virtual void start(const XmlAttributes atts);
	virtual void end();
	virtual void characters(const char *ch, int length) {
	    buffer_.insert(buffer_.size(), ch, length);
	}

	virtual XmlElement* element(const char *elementName) { return 0; }

    public:

	static const NameHelper<std::vector<T> > tagName;

	XmlVectorElement(XmlContext *context, CreationHandler newVector,
			 const char *name = tagName.c_str());
    };


    template<class T>
    const NameHelper<std::vector<T> >  XmlVectorElement<T>::tagName;


    template<class T>
    XmlVectorElement<T>::XmlVectorElement(XmlContext *context, CreationHandler newVector,
					  const char *name) :
	Predecessor(name, context, newVector),
	sizeGiven_(false),
	size_(0) {
    }


    template<class T>
    void XmlVectorElement<T>::start(const XmlAttributes atts) {

	Predecessor::start(atts);

	buffer_ = std::string();


	sizeGiven_ = false;

	const char *size = atts["size"];

	if (size != 0) {
	    sizeGiven_ = true;
	    size_ = atoi(size);
	}
    }


    template<class T>
    void XmlVectorElement<T>::end() {

	Predecessor::product_->clear();

	std::istringstream stream(buffer_);

	std::copy(std::istream_iterator<T>(stream), std::istream_iterator<T>(),
		  std::back_inserter(*Predecessor::product_));

	if (sizeGiven_ && size_ != Predecessor::product_->size())
	    this->parser()->error("Vector dimension mismatch: %zd given and %zd read.", size_, Predecessor::product_->size());

	Predecessor::end();
    }


    /** XML document containing one single vector. */
    template<class T>
    class XmlVectorDocument : public XmlSchemaParser {

	typedef XmlSchemaParser Predecessor;
	typedef XmlVectorDocument Self;

	std::vector<T> &vector_;
	std::vector<T>* pseudoCreateVector(const XmlAttributes atts) { return &vector_; }

    public:

	XmlVectorDocument(const Configuration &c, std::vector<T> &vector);
    };


    template<class T>
    XmlVectorDocument<T>::XmlVectorDocument(const Configuration &c, std::vector<T> &vector) :
	Predecessor(c),
	vector_(vector) {

	setRoot(collect(new XmlVectorElement<T>(this,
					XmlVectorElement<T>::creationHandler(
					    &Self::pseudoCreateVector))));
    }

} //namespace Core

#endif // _CORE_VECTOR_PARSER_HH
