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
#ifndef _SIGNAL_VECTOR_NORMALIZATION_HH
#define _SIGNAL_VECTOR_NORMALIZATION_HH


#include <Core/Parameter.hh>
#include <Core/Utility.hh>

#include <Flow/Data.hh>
#include <Flow/Vector.hh>
#include <Flow/Node.hh>


namespace Signal {

    /** MeanEnergyVectorNormalization: divides each element by square of frame mean energy
     *
     * square of frame mean energy = ( ( sum_{i=0}^{N-1} (x[i]^2) ) / N ) ^ {1/2}
     *   where N is the number of samples in frame
     *
     * Applied normally to amplitude like values.
     */

    template <class T>
    class MeanEnergyVectorNormalization {
    public:

	typedef T Value;

	static std::string name() {
	    return Core::Type<Value>::name + std::string("-mean-energy");
	}

	void operator()(std::vector<Value>& v) {

	    Value squareMeanEnergy =
		sqrt(std::inner_product(v.begin(), v.end(), v.begin(), 0.0) / v.size());

	    std::transform(v.begin(), v.end(), v.begin(),
			   std::bind2nd(std::multiplies<T>(), (Value)1 / squareMeanEnergy));

	}
    };


    /** EnergyVectorNormalization: divides each element by square of frame energy
     *
     * square of frame energy = ( ( sum_{i=0}^{N-1} (x[i]^2) ) ) ^ {1/2}
     *   where N is the number of samples in frame
     *
     * Applied normally to amplitude like values.
     */

    template <class T>
    class EnergyVectorNormalization {
    public:
	typedef T Value;

	static std::string name() {
	    return Core::Type<Value>::name + std::string("-energy");
	}

	void operator()(std::vector<Value>& v) {

	    Value squareEnergy = sqrt(std::inner_product(v.begin(), v.end(), v.begin(), 0.0));

	    std::transform(v.begin(), v.end(), v.begin(),
			   std::bind2nd(std::multiplies<T>(), (Value)1 / squareEnergy));

	}
    };

    /** AmplitudeSpectrumEnergyVectorNormalization: divides each element by square of frame energy
     *    calculated form (half) amplitude spectrum
     *
     *  square of frame energy =
     *    ( 1 / N * (2 * sum_{omega=1}^{N/2 - 1} (x^2(omega)) + x^2(0) + x^2(N/2)) ) ^{1/2}
     *    where N is length of FFT
     *
     *
     * Applied normally to amplitude like values.
     */

    template <class T>
    class AmplitudeSpectrumEnergyVectorNormalization {
    public:

	typedef T Value;

	static std::string name() {
	    return Core::Type<Value>::name + std::string("-amplitude-spectrum-energy");
	}

	void operator()(std::vector<Value>& v) {

	    hope(!v.empty());

	    Value squareEnergy =
		sqrt((v.front() * v.front() + v.back() * v.back() +
		      2 * std::inner_product(v.begin() + 1, v.end() - 1, v.begin() + 1, 0.0)) /
		     T((v.size() - 1) * 2));

	    std::transform(v.begin(), v.end(), v.begin(),
			   std::bind2nd(std::multiplies<T>(), (Value)1 / squareEnergy));
	}
    };

    /** MeanVectorNormalization */

    template <class T>
    class MeanVectorNormalization {
    public:
	typedef T Value;

	static std::string name() {
	    return Core::Type<Value>::name + std::string("-mean");
	}

	void operator()(std::vector<Value>& v) {

	    Value mean = std::accumulate(v.begin(), v.end(), 0.0) / v.size();

	    std::transform(v.begin(), v.end(), v.begin(),
			   std::bind2nd(std::plus<T>(), -mean));
	}
    };

    /** VarianceVectorNormalization */

    template <class T>
    class VarianceVectorNormalization {
    public:
	typedef T Value;

	static std::string name() {
	    return Core::Type<Value>::name + std::string("-variance");
	}

	void operator()(std::vector<Value>& v) {

	    Value sum = std::accumulate(v.begin(), v.end(), 0.0);
	    Value sumSquare = std::inner_product(v.begin(), v.end(), v.begin(), 0.0);

	    Value mean = sum / v.size();
	    Value deviation = sqrt((sumSquare - sum * sum / v.size()) / v.size());

	    std::transform(v.begin(), v.end(), v.begin(),
			   std::bind2nd(std::plus<T>(), -mean));

	    std::transform(v.begin(), v.end(), v.begin(),
			   std::bind2nd(std::multiplies<T>(), (Value)1 / deviation));
	}
    };

    /** MaximumVectorNormalization: divides each element of frame by maximum element */

    template <class T>
    class MaximumVectorNormalization {
    public:
	typedef T Value;

	static std::string name() {
	    return Core::Type<Value>::name + std::string("-maximum");
	}

	void operator()(std::vector<Value>& v) {

	    Value maximum = *std::max_element(v.begin(), v.end());

	    std::transform(v.begin(), v.end(), v.begin(),
			   std::bind2nd(std::multiplies<T>(), (Value)1 / maximum));
	}
    };


    /** VectorNormalizationNode */

    template<class NormalizationFunction>
    class VectorNormalizationNode : public Flow::SleeveNode {
    private:

	NormalizationFunction normalizationFunction_;

    public:

	static std::string filterName()
	    { return "signal-vector-" + NormalizationFunction::name() + "-normalization"; }

	VectorNormalizationNode(const Core::Configuration &c) : Core::Component(c), SleeveNode(c) {}

	virtual ~VectorNormalizationNode() {}

	virtual bool setParameter(const std::string &name, const std::string &value) { return false; }

	virtual bool configure() {

	    Core::Ref<const Flow::Attributes> a = getInputAttributes(0);
	    if (!configureDatatype(a, Flow::Vector<f32>::type()))
		return false;

	    return putOutputAttributes(0, a);
	}

	virtual bool work(Flow::PortId p) {

	    Flow::DataPtr<Flow::Vector<typename NormalizationFunction::Value> > in;
	    if (!getData(0, in))
		return SleeveNode::putData(0, in.get());

	    in.makePrivate();
	    normalizationFunction_(*in);

	    return putData(0, in.get());
	}
    };


}

#endif // _SIGNAL_VECTOR_NORMALIZATION_HH
