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
#ifndef _CORE_MATRIX_PARSER_HH
#define _CORE_MATRIX_PARSER_HH

#include "XmlBuilder.hh"
#include <Math/Matrix.hh>

namespace Core {

    /** XmlMatrixElement implements XmlBuilderElement for an arbitrary XmlSchemaParser
     *  which parses a matrix of a given type.
     *
     *  Matrix object is of type Math::Matrix<T>
     *  XML tag name: matrix-type. E.g.: matrix-f32
     *
     *  Elements should be stored row after row, separated by whitespace characters.
     *  Remark: end-of-line character does not have any special role.
     */
    template<class T>
    class XmlMatrixElement :
	public XmlBuilderElement<Math::Matrix<T>,
				 XmlElement,
				 CreateByContext>
    {
    public:
	typedef XmlBuilderElement<Math::Matrix<T>,
				  XmlElement,
				  CreateByContext> Predecessor;
	typedef XmlMatrixElement<T> Self;
	typedef Math::Matrix<T>* (XmlContext::*CreationHandler)(const XmlAttributes atts);
    private:
	std::string buffer_;
    protected:
	virtual void start(const XmlAttributes atts);
	virtual void end();
	virtual void characters(const char *ch, int length) {
	    buffer_.insert(buffer_.size(), ch, length);
	}

	virtual XmlElement* element(const char *elementName) { return 0; }
    public:
	static const NameHelper<Math::Matrix<T> > tagName;

	XmlMatrixElement(XmlContext *context, CreationHandler newMatrix,
			 const char *name = tagName.c_str());
    };


    template<class T>
    const NameHelper<Math::Matrix<T> > XmlMatrixElement<T>::tagName;


    template<class T>
    XmlMatrixElement<T>::XmlMatrixElement(XmlContext *context, CreationHandler newMatrix,
					  const char *name) :
	Predecessor(name, context, newMatrix)
    {}

    template<class T>
    void XmlMatrixElement<T>::start(const XmlAttributes atts)
    {
	Predecessor::start(atts);

	buffer_ = std::string();
	const char *nRows = atts["nRows"];

	if (nRows == 0)
	    this->parser()->error("Number of rows attribute (nRows) not given.");

	const char *nColumns = atts["nColumns"];

	if (nColumns == 0)
	    this->parser()->error("Number of columns attribute (nColumns) is not given.");

	Predecessor::product_->resize(atoi(nRows), atoi(nColumns));
    }


    template<class T>
    void XmlMatrixElement<T>::end() {

	std::istringstream stream(buffer_);

	for(u32 row = 0; row < Predecessor::product_->nRows(); ++ row) {
	    for(u32 column = 0; column < Predecessor::product_->nColumns(); ++ column) {
		stream >> (*Predecessor::product_)[row][column];
		if (stream.eof()) {
		    this->parser()->error("Number of elements smaller than necesseary.");
		    Predecessor::end();
		    return;
		}
	    }
	}

	T tmp;
	if (!(stream >> tmp).eof())
	    this->parser()->error("Number of elements larger than necesseary.");

	Predecessor::end();
    }

    /** XML document containing one single matrix. */
    template<class T>
    class XmlMatrixDocument : public XmlSchemaParser {
	typedef XmlSchemaParser Predecessor;
	typedef XmlMatrixDocument Self;
    private:
	XmlMatrixElement<T> *matrixElement_;
	Math::Matrix<T> &matrix_;
	Math::Matrix<T>* pseudoCreateMatrix(const XmlAttributes atts) { return &matrix_; }
    public:
	XmlMatrixDocument(const Configuration &c, Math::Matrix<T> &matrix);
	virtual ~XmlMatrixDocument();
    };

    template<class T>
    XmlMatrixDocument<T>::XmlMatrixDocument(const Configuration &c, Math::Matrix<T> &matrix) :
	Predecessor(c),
	matrixElement_(0),
	matrix_(matrix)
    {
	matrixElement_ = new XmlMatrixElement<T>(
	    this, XmlMatrixElement<T>::creationHandler(&Self::pseudoCreateMatrix));
	setRoot(matrixElement_);
    }

    template<class T>
    XmlMatrixDocument<T>::~XmlMatrixDocument()
    {
	delete matrixElement_;
    }

} // namespace Core

#endif // _CORE_MATRIX_PARSER_HH
