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
#ifndef _FLOW_TYPED_AGGREGATE_HH
#define _FLOW_TYPED_AGGREGATE_HH

#include "Merger.hh"
#include "Vector.hh"

namespace Flow {

    /** Aggregate of instances of class T.
     *  TypedAggregate<T> collects objects strictly of the same type coming from different streams.
     *  Note:
     *    -It does not support polymoric I/O.
     *    -Aggregate is a general implementation of the same problem supporting polymorfism.
     *     Advantage of this class compared to Aggregate is that the type of the elements in known.
     *     Thus there is no need for type checking when accessing the elements.
     */
    template<class T>
    class TypedAggregate : public Timestamp, public std::vector<DataPtr<T> >
    {
	typedef TypedAggregate Self;
	typedef std::vector<DataPtr<T> > PrecursorVector;
    public:
	typedef T DataType;
    protected:
	Core::XmlOpen xmlOpen() const {
	    return (Timestamp::xmlOpen() + Core::XmlAttribute("size", this->size()));
	}
    public:
	static const Datatype *type();
	TypedAggregate() : Timestamp(type()) {}
	TypedAggregate(PrecursorVector &v) : Timestamp(type()), PrecursorVector(v) {}
	virtual ~TypedAggregate() {}

	virtual Data* clone() const { return new Self(*this); }

	virtual Core::XmlWriter& dump(Core::XmlWriter&) const;
	virtual bool read(Core::BinaryInputStream&);
	virtual bool write(Core::BinaryOutputStream&) const;
    };

    template<class T>
    const Datatype *TypedAggregate<T>::type()
    {
	static Core::NameHelper<Self> name;
	static DatatypeTemplate<Self> dt(name);
	return &dt;
    }

    template<class T>
    bool TypedAggregate<T>::read(Core::BinaryInputStream &is)
    {
	Timestamp::read(is);
	u32 s; is >> s;
	this->resize(s);
	for(typename PrecursorVector::iterator i = this->begin(); i != this->end(); ++ i) {
	    *i = dataPtr(new DataType());
	    (*i)->read(is);
	}
	return is;
    }

    template<class T>
    bool TypedAggregate<T>::write(Core::BinaryOutputStream &os) const
    {
	Timestamp::write(os);
	os << (u32)this->size();
	for(typename PrecursorVector::const_iterator i = this->begin(); i != this->end(); ++ i)
	    (*i)->write(os);
	return os;
    }

    template<class T>
    Core::XmlWriter& TypedAggregate<T>::dump(Core::XmlWriter &os) const
    {
	os << xmlOpen();
	for (typename PrecursorVector::const_iterator i = this->begin(); i != this->end(); ++i)
	    (*i)->dump(os);
	os << xmlClose();
	return os;
    }

    /** Pointer Vector Filter.
     *  All input packets must be derived from type TypedAggregate<T>::DataType, which will be aggregated,
     *  e.g. their references will be collected in a vector.
     */
    template<class T>
    class TypedAggregateNode :
	public MergerNode<typename TypedAggregate<T>::DataType, TypedAggregate<T> >
    {
	typedef MergerNode<typename TypedAggregate<T>::DataType, TypedAggregate<T> > Precursor;
	typedef typename TypedAggregate<T>::DataType DataType;
    public:
	static std::string filterName() {
	    return std::string("generic-aggregation-" + DataType::type()->name());
	}
	TypedAggregateNode(const Core::Configuration &c) :
	    Core::Component(c), Precursor(c) {}
	virtual ~TypedAggregateNode() {}

	virtual TypedAggregate<DataType> *merge(std::vector<DataPtr<DataType> > &inputData) {
	    return new TypedAggregate<DataType>(inputData);
	}
    };

    template<class T>
    class TypedDisaggregateNode : public Node
    {
	typedef Node Precursor;
	typedef typename TypedAggregate<T>::DataType DataType;
    public:
	static std::string filterName() {
	    return std::string("generic-disaggregation-" + DataType::type()->name());
	}
	TypedDisaggregateNode(const Core::Configuration &c)
	    : Core::Component(c), Precursor(c) {
	    addInput(0);
	}
	virtual ~TypedDisaggregateNode() {}

	virtual PortId getInput(const std::string &name) {
	    return 0;
	}
	virtual PortId getOutput(const std::string &name);
	virtual bool configure();

	virtual bool work(PortId out);
    protected:
	typedef std::map<u32, PortId> OutputMap;
	OutputMap outputs_;
    };

    template<class T>
    PortId TypedDisaggregateNode<T>::getOutput(const std::string &name)
    {
	u32 n = 0;
	if (!name.empty())
	    n = atoi(name.c_str());
	OutputMap::const_iterator p = outputs_.find(n);
	if (p == outputs_.end()) {
	    return outputs_[n] = addOutput();
	} else {
	    return p->second;
	}
    }

    template<class T>
    bool TypedDisaggregateNode<T>::configure()
    {
	Core::Ref<Attributes> a(new Attributes);
	getInputAttributes(0, *a);
	if (!configureDatatype(a, TypedAggregate<DataType>::type()))
	    return false;
	a->set("datatype", DataType::type()->name());
	return putOutputAttributes(0, a);
    }

    template<class T>
    bool TypedDisaggregateNode<T>::work(PortId)
    {
	DataPtr<TypedAggregate<DataType> > in;
	DataPtr<DataType> out;
	if (!getData(0, in)) {
	    for (OutputMap::const_iterator i = outputs_.begin(); i != outputs_.end(); ++i) {
		putData(i->second, out.get());
	    }
	    return true;
	}
	bool ok = true;
	for (OutputMap::const_iterator i = outputs_.begin(); i != outputs_.end(); ++i) {
	    if (i->first >= in->size()) {
		error("invalid component %d", i->first);
		return false;
	    } else {
		ok = putData(i->second, (*in)[i->first].get()) && ok;
	    }
	}
	return ok;
    }



} // namespace Flow

namespace Core {
    template <typename T>
    class NameHelper<Flow::TypedAggregate<T> > : public std::string {
    public:
	NameHelper() : std::string(std::string("aggregate-") + Core::NameHelper<T>()) {}
    };
} // namespace Core

#endif // _FLOW_TYPED_AGGREGATE_HH
