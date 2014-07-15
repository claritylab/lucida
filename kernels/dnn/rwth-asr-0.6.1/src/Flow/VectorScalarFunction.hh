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
#ifndef _FLOW_VECTOR_SCALAR_FUNCTION_HH
#define _FLOW_VECTOR_SCALAR_FUNCTION_HH

#include <math.h>
#include <numeric>
#include <Core/Parameter.hh>

#include "Node.hh"
#include "Vector.hh"
#include "DataAdaptor.hh"

namespace Flow {

    /** Base class for vector-scalar functions.
     */
    template <class A, class P>
    class VectorScalarFunction {
    public:
	typedef A ArgumentType;
	typedef P ParameterType;
    };


    /** NormFunction: parameter-th norm of input vector
     *  norm = (sum_i |x_i|^parameter )^{1/parameter}
     */
    template <class T>
    class NormFunction :
	public virtual Core::Component,
	public VectorScalarFunction<T, f64> {
    public:
	static std::string name() { return "norm"; }
	NormFunction(const Core::Configuration &c) : Component(c) {}

	T apply(const std::vector<T> &v, f64 parameter) {
	    if (!v.empty()) {
		typename std::vector<T>::const_iterator begin = v.begin();
		typename std::vector<T>::const_iterator end = v.end();
		T result = 0;
		if (parameter == (f64)1) {
		    for(; begin != end; ++ begin) result += Core::abs(*begin);
		} else if (parameter == (f64)2) {
		    result = sqrt(std::inner_product(begin, end, begin, 0.0));
		} else if (parameter >= Core::Type<f64>::max) {
		    for(; begin != end; ++ begin) {
			T tmp = Core::abs(*begin);
			if (result < tmp) result = tmp;
		    }
		} else {
		    for(; begin != end; ++ begin) result += pow(Core::abs(*begin), parameter);
		    result = pow(result, 1.0 / parameter);
		}
		return result;
	    }
	    criticalError("Input vector is empty.");
	    return 0;
	}
    };


    /** EnergyFunction: energy of input vector
     *  energy = sum_i |x_i|^2
     */
    template <class T>
    class EnergyFunction : public VectorScalarFunction<T, T> {
    public:
	static std::string name() { return "energy"; }
	EnergyFunction(const Core::Configuration &) {}
	T apply(const std::vector<T> &v, T) {
	    return std::inner_product(v.begin(), v.end(), v.begin(), 0.0);
	}
    };


    const Core::ParameterFloat paramVectorScalarFunctionParameter("value", "parametric value", 0);

    /** VectorScalarFunctionNode maps a vector to a scalar value.
     */
    template<class Function>
    class VectorScalarFunctionNode : public SleeveNode {
    public:
	typedef typename Function::ArgumentType ArgumentType;
	typedef typename Function::ParameterType ParameterType;
    private:
	ParameterType functionParameter_;
	Function function_;
    private:
	void setFunctionParameter(ParameterType parameter) { functionParameter_ = parameter; }
    public:
	static std::string filterName() {
	    return std::string("generic-vector-") + Core::Type<ArgumentType>::name +
		"-" + Function::name();
	}

	VectorScalarFunctionNode(const Core::Configuration &c);
	virtual ~VectorScalarFunctionNode() {}

	virtual bool configure();
	virtual bool setParameter(const std::string &name, const std::string &value);
	virtual bool work(PortId p);
    };


    template<class Function>
    VectorScalarFunctionNode<Function>::VectorScalarFunctionNode(const Core::Configuration &c) :
	Component(c), SleeveNode(c), function_(c) {
	setFunctionParameter(paramVectorScalarFunctionParameter(c));
    }


    template<class Function>
    bool VectorScalarFunctionNode<Function>::configure() {
	Core::Ref<Attributes> attributes(new Attributes());
	getInputAttributes(0, *attributes);

	if (!configureDatatype(attributes, Vector<ArgumentType>::type()))
	    return false;

	attributes->set("datatype", DataAdaptor<ArgumentType>::type()->name());
	attributes->remove("sample-rate");

	return putOutputAttributes(0, attributes);
    }

    template<class Function>
    bool VectorScalarFunctionNode<Function>::setParameter(
	const std::string &name, const std::string &value) {
	if (paramVectorScalarFunctionParameter.match(name))
	    setFunctionParameter(paramVectorScalarFunctionParameter(value));
	else
	    return false;

	return true;
    }

    template<class Function>
    bool VectorScalarFunctionNode<Function>::work(PortId p) {
	DataPtr<Vector<ArgumentType> > in;

	if (!getData(0, in))
	    return putData(0, in.get());

	DataAdaptor<ArgumentType> *out = new DataAdaptor<ArgumentType>;
	out->setTimestamp(*in);
	(*out)() = function_.apply(*in, functionParameter_);

	return putData(0, out);
    }

}  // namespace Flow


#endif // _FLOW_VECTOR_SCALAR_FUNCTION_HH
