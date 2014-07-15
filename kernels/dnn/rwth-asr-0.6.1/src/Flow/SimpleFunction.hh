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
#ifndef _FLOW_VECTOR_FUNCTION_HH
#define _FLOW_VECTOR_FUNCTION_HH

#include <cmath>
#include <numeric>

#include <Core/Parameter.hh>

#include "Flow/Node.hh"
#include "Flow/Vector.hh"
#include "Flow/DataAdaptor.hh"

namespace Flow {

/**
 * Base class for functions vith one parameter
 */
template <class A, class P>
class SimpleFunction {
public:
	typedef A ArgumentType;
	typedef P ParameterType;
};


template <class T>
class VectorLogFunction : public SimpleFunction<Vector<T>, T> {
public:
	void apply(Vector<T> &v, T) {
		std::transform(v.begin(), v.end(), v.begin(), std::ptr_fun(log10));
	}
	static std::string name() { return Vector<T>::type()->name() + "-log"; }
};

template <class T>
class LogFunction : public SimpleFunction<DataAdaptor<T>, T> {
public:
	void apply(DataAdaptor<T> &v, T) { v() = log10(v()); }
	static std::string name() { return DataAdaptor<T>::type()->name() + "-log"; }
};

template <class T>
class VectorLogPlusFunction  : public SimpleFunction<Vector<T>, T> {
public:
	void apply(Vector<T> &v, T parameter) {
		for (u32 i = 0; i < v.size(); i++) v[i] = log10(v[i] + parameter); };
	static std::string name() { return Vector<T>::type()->name() + "-log-plus"; }
};

template <class T>
class LogPlusFunction  : public SimpleFunction<DataAdaptor<T>, T> {
public:
	void apply(DataAdaptor<T> &v, T parameter) { v() = log10(v() + parameter); };
	static std::string name() { return DataAdaptor<T>::type()->name() + "-log-plus"; }
};

/**	Add small value to avoid -infs
 */
template <class T>
class VectorLnFunctionSave  : public SimpleFunction<Vector<T>, T> {
public:
	void apply(Vector<T> &v, T) {
		const T tinyValue = 1.175494e-38;
		std::transform(v.begin(), v.end(), v.begin(), std::bind2nd(std::plus<T>(), tinyValue));
		std::transform(v.begin(), v.end(), v.begin(), std::ptr_fun(log));
	}
	static std::string name() { return Vector<T>::type()->name() + "-ln-save"; }
};

template <class T>
class VectorLnFunction  : public SimpleFunction<Vector<T>, T> {
public:
	void apply(Vector<T> &v, T) {
		std::transform(v.begin(), v.end(), v.begin(), std::ptr_fun(log));
	}
	static std::string name() { return Vector<T>::type()->name() + "-ln"; }
};

template <class T>
class LnFunction  : public SimpleFunction<DataAdaptor<T>, T> {
public:
	void apply(DataAdaptor<T> &v, T) { v() = log(v()); }
	static std::string name() { return DataAdaptor<T>::type()->name() + "-ln"; }
};

template <class T>
class VectorExpFunction  : public SimpleFunction<Vector<T>, T> {
public:
	void apply(Vector<T> &v, T) {
		std::transform(v.begin(), v.end(), v.begin(), std::ptr_fun(exp));
	}
	static std::string name() { return Vector<T>::type()->name() + "-exp"; }
};

template <class T>
class ExpFunction  : public SimpleFunction<DataAdaptor<T>, T> {
public:
	void apply(DataAdaptor<T> &v, T) { v() = exp(v()); }
	static std::string name() { return DataAdaptor<T>::type()->name() + "-exp"; }
};

template <class T>
class VectorPowerFunction  : public SimpleFunction<Vector<T>, T> {
public:
	void apply(Vector<T> &v, T parameter) {
		for (u32 i = 0; i < v.size(); i++) v[i] = pow(v[i], parameter); };
	static std::string name() { return Vector<T>::type()->name() + "-power"; }
};

template <class T>
class PowerFunction  : public SimpleFunction<DataAdaptor<T>, T> {
public:
	void apply(DataAdaptor<T> &v, T parameter) { v() = pow(v(), parameter); };
	static std::string name() { return DataAdaptor<T>::type()->name() + "-power"; }
};

template <class T>
class VectorSqrtFunction : public SimpleFunction<Vector<T>, T> {
public:
	void apply(Vector<T> &v, T) {
		std::transform(v.begin(), v.end(), v.begin(), std::ptr_fun(sqrt));
	}
	static std::string name() { return Vector<T>::type()->name() + "-sqrt"; }
};

template <class T>
class SqrtFunction : public SimpleFunction<DataAdaptor<T>, T> {
public:
	void apply(DataAdaptor<T> &v, T) { v() = sqrt(v()); }
	static std::string name() { return DataAdaptor<T>::type()->name() + "-sqrt"; }
};

template <class T>
class VectorCosFunction  : public SimpleFunction<Vector<T>, T> {
public:
	void apply(Vector<T> &v, T) {
		std::transform(v.begin(), v.end(), v.begin(), std::ptr_fun(cos));
	}
	static std::string name() { return Vector<T>::type()->name() + "-cos"; }
};

template <class T>
class CosFunction  : public SimpleFunction<DataAdaptor<T>, T> {
public:
	void apply(DataAdaptor<T> &v, T) { v() = cos(v()); }
	static std::string name() { return DataAdaptor<T>::type()->name() + "-cos"; }
};

template <class T>
class VectorScalarAdditionFunction  : public SimpleFunction<Vector<T>, T> {
public:
	void apply(Vector<T> &v, T value) { for (u32 i = 0; i < v.size(); i++) v[i] += value; };
	static std::string name() { return Vector<T>::type()->name() + "-addition"; }
};

template <class T>
class AdditionFunction  : public SimpleFunction<DataAdaptor<T>, T> {
public:
	void apply(DataAdaptor<T> &v, T value) { v() += value; };
	static std::string name() { return DataAdaptor<T>::type()->name() + "-addition"; }
};

template <class T>
class VectorScalarMultiplicationFunction  : public SimpleFunction<Vector<T>, T> {
public:
	void apply(Vector<T> &v, T value) { for (u32 i = 0; i < v.size(); i++) v[i] *= value; };
	static std::string name() { return Vector<T>::type()->name() + "-multiplication"; }
};

template <class T>
class MultiplicationFunction  : public SimpleFunction<DataAdaptor<T>, T> {
public:
	void apply(DataAdaptor<T> &v, T value) { v() *= value; };
	static std::string name() { return DataAdaptor<T>::type()->name() + "-multiplication"; }
};

/**
 * Quantize (i.e. round) each component of vector to nearest integer
 * multiple of @c parameter
 */
template <class T>
class VectorQuantizationFunction  : public SimpleFunction<Vector<T>, T> {
public:
	static std::string name() { return Vector<T>::type()->name() + "-quantize"; }
	void apply(Vector<T> &v, T parameter) {
		if (parameter == 1.0 || parameter == 0.0) {
			for (u32 i = 0; i < v.size(); i++)
				v[i] = rint(v[i]);
		} else {
			for (u32 i = 0; i < v.size(); i++)
				v[i] = rint(v[i] / parameter) * parameter;
		}
	};
};

template <class T>
class VectorAbsoluteValueFunction : public SimpleFunction<Vector<T>, T> {
public:
	void apply(Vector<T> &v, T ) {
		std::transform(v.begin(), v.end(), v.begin(), Core::absoluteValue<T>());
	}
	static std::string name() { return Vector<T>::type()->name() + "-abs"; }
};

template <class T>
class AbsoluteValueFunction : public SimpleFunction<DataAdaptor<T>, T> {
public:
	void apply(DataAdaptor<T> &v, T ) { v() = Core::abs(v()); }
	static std::string name() { return DataAdaptor<T>::type()->name() + "-abs"; }
};

/**
 *  Calculates the differences of adjacent elements.
 *  First element is set to zero.
 */
template <class T>
class AdjacentDifference : public SimpleFunction<Vector<T>, T> {
public:
	void apply(Vector<T> &v, T) {
		std::adjacent_difference(v.begin(), v.end(), v.begin());
		v.front() = 0;
	}
	static std::string name() { return Vector<T>::type()->name() + "-adjacent-difference"; }
};

/**
 * SimpleFunctionNode for neural network activation function LINEAR
 * y_i = x_i
 */
template <class T>
class VectorLinearFunction : public SimpleFunction<Vector<T>, T> {
public:
	void apply(Vector<T> &v, T) {}

	static std::string name() { return Vector<T>::type()->name() + "-linear"; }
};

/**
 * SimpleFunctionNode for neural network activation function SIGMOID
 * y_i = 1 / (1 + EXP( -x_i) )
 */
template <class T>
class VectorSigmoidFunction : public SimpleFunction<Vector<T>, T> {
public:
	void apply(Vector<T> &v, T) {
		// apply sigmoid
		//for (u32 i = 0; i < v.size(); i++) {v[i] = (1 / (1 + exp(- (v[i])) ) );};
		for (typename Vector<T>::iterator it = v.begin(); it != v.end(); it++)
			*it = (1 / (1 + exp (-(*it)) ) );
	}

	static std::string name() { return Vector<T>::type()->name() + "-sigmoid"; }
};

/**
 * SimpleFunctionNode for neural network activation function SOFTMAX
 * y_i = EXP(x_i - MAX(x)) / SUM( EXP( x_i - MAX(i) ) )
 */
template <class T>
class VectorSoftmaxFunction : public SimpleFunction<Vector<T>, T> {
public:
	void apply(Vector<T> &v, T) {
		/* MAX(v) */
		T maxValue = *std::max_element(v.begin(), v.end());
		/* v[i] = v[i] - MAX(v) */
		std::transform(v.begin(), v.end(), v.begin(),
				std::bind2nd(std::plus<T>(), -maxValue));
		/* v[i] = exp( v[i] - MAX(v) )*/
		std::transform(v.begin(), v.end(), v.begin(), std::ptr_fun(exp));

		/* sum( exp( v[i] - MAX(v) ) ) */
		T sumValue = std::accumulate(v.begin(), v.end(), (T)0);
		/* exp( (v[i] - MAX(v)) ) / sum( exp( (v[i] - MAX(v)) ) ) */
		std::transform(v.begin(), v.end(), v.begin(),
				std::bind2nd(std::multiplies<T>(), (T)1 / sumValue));
	}

	static std::string name() { return Vector<T>::type()->name() + "-softmax"; }
};

/**
 * SimpleFunctionNode for neural network activation function TANH
 * y_i = TANH(x_i)
 */
template <class T>
class VectorTanhFunction : public SimpleFunction<Vector<T>, T> {
public:
	void apply(Vector<T> &v, T) {
		/* v[i] = tanh( v[i] )*/
		std::transform(v.begin(), v.end(), v.begin(), std::ptr_fun(tanh));
	}

	static std::string name() { return Vector<T>::type()->name() + "-tanh"; }
};

/**
 *  SimpleFunctionNode template for functions with one parameter
 */

const Core::ParameterFloat paramSimpleFunctionParameter("value", "parametric value", 0);

template<class Function>
class SimpleFunctionNode : public SleeveNode {
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
		return std::string("generic-") + Function::name();
	}

	SimpleFunctionNode(const Core::Configuration &c);
	virtual ~SimpleFunctionNode() {}

	virtual bool configure();
	virtual bool setParameter(const std::string &name, const std::string &value);
	virtual bool work(PortId p);
};


template<class Function>
SimpleFunctionNode<Function>::SimpleFunctionNode(const Core::Configuration &c) :
Component(c), SleeveNode(c) {
	setFunctionParameter(paramSimpleFunctionParameter(c));
}


template<class Function>
bool SimpleFunctionNode<Function>::configure() {
	Core::Ref<const Attributes> attributes = getInputAttributes(0);
	if (!configureDatatype(attributes, ArgumentType::type())) return false;
	return putOutputAttributes(0, attributes);
}


template<class Function>
bool SimpleFunctionNode<Function>::setParameter(const std::string &name, const std::string &value) {
	if (paramSimpleFunctionParameter.match(name))
		setFunctionParameter(paramSimpleFunctionParameter(value));
	else
		return false;
	return true;
}


template<class Function>
bool SimpleFunctionNode<Function>::work(PortId p) {
	DataPtr<ArgumentType> in;

	if (!getData(0, in))
		return putData(0, in.get());

	in.makePrivate();
	function_.apply(*in, functionParameter_);

	return putData(0, in.get());
}

} // namespace Flow


#endif // _FLOW_VECTOR_FUNCTION_HH
