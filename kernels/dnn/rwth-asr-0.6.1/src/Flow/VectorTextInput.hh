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
#ifndef _FLOW_VECTOR_TEXT_INPUT_HH
#define _FLOW_VECTOR_TEXT_INPUT_HH

#include "Attributes.hh"
#include "Node.hh"
#include "Vector.hh"
#include <Core/Types.hh>
#include <Core/Component.hh>
#include <cmath>
#include <fstream>
#include <typeinfo>


namespace Flow {

    class VectorTextInputNodeBase :
	public SourceNode
    {
    public:
	static const Core::ParameterString paramFileName;
	static const Core::ParameterFloat paramLength;
	static const Core::ParameterFloat paramShift;
	static const Core::ParameterFloat paramOffset;
	static const Core::ParameterInt paramSampleRate;
    private:
	class Parser;
	friend class Parser;
	Parser *parser_;
	std::string fileName_;
	Time timeInS_;
	Time shiftInS_;
	Time offsetInS_;
	Time lengthInS_;
	u32 sampleRate_;
    protected:
	const Datatype *type_;
	DataPtr<Timestamp> out_;
	mutable Core::XmlChannel fileinfoChannel_;
	virtual void create(size_t dim) = 0;
	virtual void setElements(const std::string &buffer) = 0;
	void issue();
    public:
	VectorTextInputNodeBase(const Core::Configuration&, const Datatype*);
	virtual bool configure();
	virtual bool setParameter(const std::string &name, const std::string &value);
	virtual bool work(PortId);
    };

    /**
     * VectorTextInputNode: data source using XML input
     *
     * Node reads an arbitrary XML file filtering out only tags produced by
     * Flow::Vector::dump(...). All other tags are ignored.
     * Node goes though the whole file and puts the read Vector<T> objects into
     * its output link.
     *
     */

    template<class T>
    class VectorTextInputNode :
	public VectorTextInputNodeBase
    {
    private:
	size_t givenSize_;
    protected:
	virtual void create(size_t dim) {
	    givenSize_ = dim;
	    out_ = dataPtr(new Vector<T>());
	}
	virtual void setElements(const std::string &buffer) {
	    DataPtr<Vector<T> > out(out_);
	    ensure(out);

	    out->clear();
	    std::istringstream stream(buffer);
	    std::copy(std::istream_iterator<T>(stream), std::istream_iterator<T>(),
		      std::back_inserter(*out));
	    if (givenSize_ > 0 && givenSize_ != out->size())
		error("Vector dimension mismatch while processing \"%s\": %zd given and %zd read.", buffer.c_str(), givenSize_, out->size());
	}
    public:
	static std::string filterName() {
	    return std::string("generic-vector-") + Core::Type<T>::name + "-text-input";
	}
	VectorTextInputNode(const Core::Configuration &c) :
	    Core::Component(c), VectorTextInputNodeBase(c, Vector<T>::type()), givenSize_(0) {}
	virtual ~VectorTextInputNode() {}
    };

} // namespace Flow

#endif // _FLOW_VECTOR_TEXT_INPUT_HH
