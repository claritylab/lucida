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
#ifndef _SIGNAL_COSINE_TRANSFORM_HH
#define _SIGNAL_COSINE_TRANSFORM_HH

#include <Math/Matrix.hh>
#include <Math/AnalyticFunctionFactory.hh>
#include <Flow/StringExpressionNode.hh>
#include <Flow/Vector.hh>

namespace Signal {

    /** Discrete Cosine Transform with an optional warping of the input 'omega' range. */
    class CosineTransform {
    public:
	typedef f32 Value;

	enum InputType { NplusOneData, evenAboutNminusHalf };
    private:
	Math::Matrix<Value> transformation_;
	/** Depends on input type. @see initNplusOneData and initEvenAboutNminusHalf */
	size_t N_;
	bool normalize_;
    private:
	/** Input data f assumed to have N+1 elments:
	 *  F_k = 1/2[f_0 + (-1)^k] + sum_{n=1}^{N-1}( f_n cos( warp(pi*n/N) * k).
	 */
	void initNplusOneData(
	    size_t inputSize, size_t outputSize,
	    Math::UnaryAnalyticFunctionRef warpingFunction,
	    Math::UnaryAnalyticFunctionRef derivedWarpingFunction);
	/** Input data f of length N is extended by N new elements such that it is
	 *  even about N + 1/2 and periodic:
	 *  F_k = sum_{n=0}^{N-1}( f_n cos( warp(pi*(n+1/2)/N) * k ).
	 */
	void initEvenAboutNminusHalf(
	    size_t inputSize, size_t outputSize,
	    Math::UnaryAnalyticFunctionRef warpingFunction,
	    Math::UnaryAnalyticFunctionRef derivedWarpingFunction);
    public:
	CosineTransform();

	void init(InputType inputType, size_t inputSize, size_t outputSize, bool normalize = false) {
	    init(inputType, inputSize, outputSize, normalize,
		 Math::AnalyticFunctionFactory::createIdentity(), true);
	}
	void init(InputType inputType, size_t inputSize, size_t outputSize, bool normalize,
		  Math::UnaryAnalyticFunctionRef, bool shouldWarpDifferentialUnit);

	/** Calculates cosinus transform of @param in.
	 *  If normalization is set at initialization, the result (@param out) is is divided by N.
	 *  Remark:
	 *   Normalization is not included in the transformation matrix, as it neither is in (inverse) FFT.
	 */
	void apply(const std::vector<Value> &in, std::vector<Value> &out) const;

	size_t inputSize() const { return transformation_.nColumns(); }
    };

    /** Cosine Transform Node
     *  Performs cosine transform with several options:
     *  -arbitrary integrated warping function.
     *  -"N+1" and "even about N-1/2" input types
     *  -sample rate handling motivated by inverse fourier transform.
     *
     *  Parameter warping-function: @see FilterbankNode.
     */
    class CosineTransformNode : public Flow::StringExpressionNode, CosineTransform {
    public:
	static const Core::Choice choiceInputType;
	static const Core::ParameterChoice paramInputType;
	static const Core::ParameterInt paramOutputSize;
	static const Core::ParameterBool paramNormalize;
	static const Core::ParameterString paramWarpingFunction;
	static const Core::ParameterBool paramWarpDifferentialUnit;
    private:
	InputType inputType_;
	size_t outputSize_;
	f64 frequencyDomainSampleRate_;
	bool normalize_;
	bool shouldWarpDifferenctialUnit_;
	bool needInit_;
    private:
	void setInputType(InputType inputType) {
	    if (inputType_ != inputType) { inputType_ = inputType; needInit_ = true; }
	}
	void setOutputSize(size_t size) {
	    if (outputSize_ != size) { outputSize_ = size; needInit_ = true; }
	}
	/** Sets if result should be normalized by input length N
	 *  For definition of N @see CosineTransform.
	 */
	void setNormalize(bool normalize) {
	    if (normalize_ != normalize) { normalize_ = normalize; needInit_ = true; }
	}
	/** Sets sample rate of input.
	 *  For motivation of sample rate @see timeDomainSampleRate.
	 */
	void setFrequencyDomainSampleRate(f64 s) {
	    if (frequencyDomainSampleRate_ != s) { frequencyDomainSampleRate_ = s; needInit_ = true; }
	}
	/** Calculates the time domain sample rate of the input.
	 *  Motivation:
	 *    Since discrete cosine transform does not handle sample rate,
	 *    conversion of sample rate is based on the similarity of cosine transform
	 *    to inverse Fourier transform. Thus, input is assumed to be in frequency domain and
	 *    output will be transformed into time domain. Further assumption is that
	 *    input is only the first half of a symmetric function (similar to amplitude spectrum.).
	 *  Formulas:
	 *    -"N+1" input type: time-sample-rate = 2 * (inputSize - 1) / frequency-sample-rate.
	 *    -"even about N-1/2" input type: time-sample-rate = 2 * inputSize / frequency-sample-rate.
	 */
	f64 timeDomainSampleRate(size_t inputSize);

	Math::UnaryAnalyticFunctionRef createWarpingFunction(f64 sampleRate);
	void setWarpDifferentialUnit(bool should) {
	    if (shouldWarpDifferenctialUnit_ != should) {
		shouldWarpDifferenctialUnit_ = should; needInit_ = true;
	    }
	}
	void init(size_t inputSize);
    public:
	static std::string filterName() { return "signal-cosine-transform"; }
	CosineTransformNode(const Core::Configuration &c);
	virtual ~CosineTransformNode() {}

	Flow::PortId getInput(const std::string &name) {
	    return name.empty() ? 0 : StringExpressionNode::getInput(name);
	}
	Flow::PortId getOutput(const std::string &name) { return 0; }

	virtual bool setParameter(const std::string &name, const std::string &value);
	virtual bool configure();
	virtual bool work(Flow::PortId p);
    };
}


#endif // _SIGNAL_COSINE_TRANSFORM_HH
