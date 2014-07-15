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
#ifndef _FLOW_TYPE_CONVERTER_HH
#define _FLOW_TYPE_CONVERTER_HH

#include "DataAdaptor.hh"
#include "Node.hh"
#include "Vector.hh"


namespace Flow {

    /** Base class for converter functors.
     */
    template <class In, class Out>
    struct Converter {
	typedef In InputType;
	typedef Out OutputType;
    };


    /** Converts Vector<In> to Vector<Out>
     */
    template <class In, class Out>
    struct VectorConverter : public Converter<Vector<In>, Vector<Out> > {
	VectorConverter(const Core::Configuration&) {}

	Vector<Out>* operator() (const Vector<In> &v) {
	    Vector<Out> *result = new Vector<Out>(v.size());
	    std::copy(v.begin(), v.end(), result->begin());
	    return result;
	}
    };


    /** Converts String to Out
     */
    template <class Out>
    struct StringConverter :
	public virtual Core::Component,
	public Converter<String, Out>
    {
	StringConverter(const Core::Configuration &c) : Component(c) {}

	Out* operator() (const String &v) {
	    if (!v().empty()) {
		Out *result = new Out;
		std::stringstream s(v());
		s >> (*result)();
		return result;
	    }
	    criticalError("Input is empty.");
	    return 0;
	}
    };

    /** Converts scalar In to String
     */
    template <class In>
    struct ScalarToStringConverter :
	public virtual Core::Component,
	public Converter<In, String>
    {
	ScalarToStringConverter(const Core::Configuration &c) : Component(c) {}

	String* operator() (const In &v) {
	    std::stringstream s;
	    s << v();
	    return new String(s.str());
	}
    };

    /** Converts Vector<In> of size 1 to Out
     */
    template <class In, class Out>
    struct VectorToScalarConverter :
	public virtual Core::Component,
	public Converter<Vector<In>, Out>
    {
	VectorToScalarConverter(const Core::Configuration &c) : Component(c) {}

	Out* operator() (const Vector<In> &v) {
	    if (v.size() == 1)
		return new Out(v.front());
	    criticalError("Input is of size %zd, instead of 1.", v.size());
	    return 0;
	}
    };


    /** Converts In to Vector<Out> of size 1
     */
    template <class In, class Out>
    struct ScalarToVectorConverter : public Converter<In, Vector<Out> > {
	ScalarToVectorConverter(const Core::Configuration&) {}

	Vector<Out>* operator() (const In &v) {
	    Vector<Out> *result = new Vector<Out>(1);
	    (*result)[0] = v();
	    return result;
	}
    };


    /** Converts object of type C::InputType to
     *  type C::OutputType by using the functor C.
     */
    template <class C>
    class TypeConverterNode : public SleeveNode {
    public:
	typedef typename C::InputType In;
	typedef typename C::OutputType Out;
    private:
	C converter_;
    public:
	static std::string filterName() {
	    return std::string("generic-convert-") + In::type()->name() + "-to-" + Out::type()->name();
	}
	TypeConverterNode(const Core::Configuration &c) : Component(c), SleeveNode(c), converter_(c) {}
	virtual ~TypeConverterNode() {}

	virtual bool configure();
	virtual bool work(PortId p);
    };


    template <class C>
    bool TypeConverterNode<C>::configure() {
	Core::Ref<Attributes> a(new Attributes());
	getInputAttributes(0, *a);
	if (!configureDatatype(a, In::type()))
	    return false;
	a->set("datatype", Out::type()->name());
	return putOutputAttributes(0, a);
    }


    template <class C>
    bool TypeConverterNode<C>::work(PortId p) {
	DataPtr<In> in;
	if (!getData(0, in))
	    return putData(0, in.get());

	Out *out = converter_(*in);
	require(out != 0);
	out->setTimestamp(*in);

	return putData(0, out);
    }


} // namespace Flow

#endif // _FLOW_TYPE_CONVERTER_HH
