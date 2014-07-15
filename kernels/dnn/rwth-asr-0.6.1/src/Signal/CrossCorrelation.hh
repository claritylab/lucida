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
#ifndef _SIGNAL_CORRELATION_HH
#define _SIGNAL_CORRELATION_HH


#include "FastFourierTransform.hh"
#include <Core/Choice.hh>
#include <Core/Channel.hh>
#include <Core/Parameter.hh>
#include <Core/Utility.hh>
#include <Flow/Vector.hh>
#include <Flow/Node.hh>
#include <Math/Utilities.hh>
#include <Math/Matrix.hh>

namespace Signal {

    /** normalizeCrossCorrelationEstimate: normalizes cross-correlation estimate by number of addands
     * Leads to the so called unbiased estimate of the cross-correaltion function
     */
    template<class Argument>
    class normalizeCrossCorrelationEstimate : std::binary_function<Argument, s32, Argument> {
	s32 sizeX_;
	s32 sizeY_;
    public:
	normalizeCrossCorrelationEstimate(const std::vector<Argument> &x, const std::vector<Argument> &y) :
	    sizeX_((s32)x.size()), sizeY_((s32)y.size()) {}

	Argument operator()(Argument element, s32 m) {
	    s32 N = std::min(sizeX_ - m, sizeY_) - std::max(0, -m);
	    if (N > 0)
		return element / N;
	    return 0;
	}
    };

    /** normalizeCrossCorrelationUpperBound: normalizes upper bound of cross-correlation to 1
     *   |Rxy(m)| / sqrt(Rxx(0)Ryy(0)) <= 1
     *   With other words, the two sequences x, y are normalized such that the zeroth lag
     *   of their autocorrelation is 1.
     */
    template<class Argument>
    class normalizeCrossCorrelationUpperBound : std::binary_function<Argument, s32, Argument> {
	Argument upperBound_;
    public:
	normalizeCrossCorrelationUpperBound(const std::vector<Argument> &x, const std::vector<Argument> &y) {
	    upperBound_ = sqrt(std::inner_product(x.begin(), x.end(), x.begin(), 0.0) *
			       std::inner_product(y.begin(), y.end(), y.begin(), 0.0));
	}
	Argument operator()(Argument element, s32) { return element / upperBound_; }
    };

    /** normalizeCrossSquareDifferenceUpperBound: normalizes upper bound of cross-square-difference to 1.
     *   0 <= Qxy(m) / (sqrt(Rxx(0)) + sqrt(Ryy(0)))^2 <= 1
     */
    template<class Argument>
    class normalizeCrossSquareDifferenceUpperBound : std::binary_function<Argument, s32, Argument> {
	Argument upperBound_;
    public:
	normalizeCrossSquareDifferenceUpperBound(const std::vector<Argument> &x,
						    const std::vector<Argument> &y) {
	    upperBound_ =
		sqrt(std::inner_product(x.begin(), x.end(), x.begin(), 0.0)) +
		sqrt(std::inner_product(y.begin(), y.end(), y.begin(), 0.0));
	    upperBound_ *= upperBound_;
	}
	Argument operator()(Argument element, s32) { return element / upperBound_; }
    };

    /** normalizeCrossAbsoluteDifferenceUpperBound: normalizes upper bound of cross-absolute-difference to 1
     *   0 <= Dxy(m) / [sum( abs(x(t)) ) + sum( abs(y(t)) )] <= 1
     */
    template<class Argument>
    class normalizeCrossAbsoluteDifferenceUpperBound : std::binary_function<Argument, s32, Argument> {
	Argument upperBound_;
    public:
	normalizeCrossAbsoluteDifferenceUpperBound(const std::vector<Argument> &x,
						   const std::vector<Argument> &y) {
	    upperBound_ = 0;
	    for(typename std::vector<Argument>::const_iterator i = x.begin(); i != x.end(); ++ i)
		upperBound_ += Core::abs(*i);
	    for(typename std::vector<Argument>::const_iterator i = y.begin(); i != y.end(); ++ i)
		upperBound_ += Core::abs(*i);
	}
	Argument operator()(Argument element, s32) { return element / upperBound_; }
    };

    /** CrossCorrelation
     * Rxy(m) = sum_{t = max(0, -m)}^{min(sizeX_ - m, sizeY_)} similarity(x_{t + m}, y_{t})
     *
     * where:
     *   "similarty" can be:
     *     multiplication: x_{t + m} * y_{t}
     *     absolute difference: |x_{t + m} - y_{t}|^power
     *
     *   m is element of [begin..end); begin and end are give in continuous units (e.g.: sec, Hz)
     *
     * normalitions:
     *   Rxy(m) can be normalized by
     *     - number of addands: leads to unbiased estimate of cross-correlation
     *     - upper bound: leads to |Rxy(t)| <= 1
     *
     * application of fast Fourier Transform:
     *   can be applied if similarity function is the multiplication.
     *   controlled over the parameter useFastFourierTransform
     */
    class CrossCorrelation {
    public:
	typedef f32 Data;

	enum SimilarityFunctionType { Multiplication, AbsoluteDifference };
	enum NormalizationType { None, UnbiasedEstimate, UpperBound };
    private:
	std::vector<Data> X_;
	std::vector<Data> Y_;

	/** Corss-correlation calculated on the interval [begin..end) */
	s32 begin_;
	s32 end_;

	SimilarityFunctionType similarityFunctionType_;
	f64 power_;

	NormalizationType normalizationType_;

	bool useFastFourierTransform_;
    private:
	/** CorssCorrelation: corss-correlation implemented in frequency domain
	 */
	void crossCorrelation(const std::vector<Data> &x, const std::vector<Data> &y,
			      std::vector<Data> &Rxy);
	/** CorssCorrelation: corss-correlation implemented in time domain
	 * @param product is a binary functor implementing an arbitrary similarity function
	 *
	 * remark:
	 *   This implementation compares only the overlapping part of the shifted x and y.
	 *   It does not extend any of the signals by zeros. This does not make any difference
	 *   if applying 'multiplication' as similarity function but it do make a significant
	 *   difference for absolute-difference kind of similarity functions.
	 */
	template<class OperationProduct>
	void crossCorrelation(const std::vector<Data> &x, const std::vector<Data> &y,
			      std::vector<Data> &Rxy, OperationProduct product) const {

	    // interval [begin..end) too broad, x and y do not overlap
	    ensure(begin_ > -((s32)y.size()) && end_ <= (s32)x.size());

	    Rxy.resize(end_ - begin_);
	    std::fill(Rxy.begin(), Rxy.end(), 0);

	    for(s32 m = begin_; m < end_; ++ m) {

		s32 begin = std::max(0, -m);
		s32 end = std::min((s32)x.size() - m, (s32)y.size());

		for(s32 n = begin; n < end; ++ n)
		    Rxy[m - begin_] += product(x[n + m], y[n]);
	    }
	}

	/* normalize: normalizes elements of Rxy(m)
	 * @param normalization is an arbitrary binary functor with parameter (element, m)
	 */
	template<class OperationNormalization>
	void normalize(const std::vector<Data> &x, const std::vector<Data> &y,
		       std::vector<Data> &Rxy, OperationNormalization normalization) const {

	    for(s32 m = begin_; m < end_; ++ m)
		Rxy[m - begin_] = normalization(Rxy[m - begin_], m);
	}

	void init();

	void crossCorrelationWithMultiplication(const std::vector<Data> &x, const std::vector<Data> &y,
						std::vector<Data> &Rxy);
	void crossCorrelationWithAbsoluteDifference(const std::vector<Data> &x, const std::vector<Data> &y,
					      std::vector<Data> &Rxy) const;

	void normalizeUpperBound(const std::vector<Data> &x, const std::vector<Data> &y,
				 std::vector<Data> &Rxy) const;
    public:
	CrossCorrelation();

	void apply(const std::vector<Data> &x, const std::vector<Data> &y, std::vector<Data> &Rxy);
	void apply(const std::vector<Data> &x, const std::vector<Data> &y, std::vector<Data> &Rxy,
		   Core::Component &errorChannels);

	void setBegin(s32 begin) { begin_ = begin; }
	void setEnd(s32 end) { end_ = end; }

	void setSimilarityFunctionType(const SimilarityFunctionType similarityFunctionType) {
	    similarityFunctionType_ = similarityFunctionType;
	}

	void setPower(const f64 power) { power_ = power; }
	void setNormalizationType(const NormalizationType normalizationType) {
	    normalizationType_ = normalizationType;
	}
	void setUseFastFourierTransform(const bool use) { useFastFourierTransform_ = use; }
    };

    /** Autocorrelation of from segment of amplitude spectrum. */
    class BandpassAutocorrelation {
    public:
	typedef f32 Data;
    private:
	/** Row index: order, column index discreteFrrequency */
	Math::Matrix<Data> R_;
	std::vector<Data> amplitudeSquare_;
    public:
	BandpassAutocorrelation() {}
	~BandpassAutocorrelation() {}

	/** Computes the R_ matrix
	 *  @param nComponents is number of autocorrelation components (order + 1 for AR estimation).
	 *  @param amplitude is the half(odd number of elements) of the amplitude spectrum.
	 */
	bool init(size_t nComponents, const std::vector<Data>& amplitude);
	/** Retrives the autocorrelation for the frequency band [@param bandBegin..@param bandEnd) */
	bool work(u32 bandBegin, u32 bandEnd, std::vector<Data> &autocorrelation);
    };

    /** CrossCorrelationNode
     */
    class CrossCorrelationNode : public Flow::Node, public CrossCorrelation {
    public:
	static const Core::ParameterFloat paramBegin;
	static const Core::ParameterFloat paramEnd;
	static const Core::ParameterInt paramNumberOfCoefficients;
	static const Core::Choice choiceSimilarityFunctionType;
	static const Core::ParameterChoice paramSimilarityFunctionType;
	static const Core::ParameterFloat paramPower;
	static const Core::Choice choiceNormalizationType;
	static const Core::ParameterChoice paramNormalizationType;
	static const Core::ParameterBool paramUseFastFourierTransform;
    private:
	/** Corss-correlation calculated on the constinuous interval [begin..end) */
	f64 continuousBegin_;
	f64 continuousEnd_;
	f64 sampleRate_;

	/** Corss-correlation calculated on the discrete interval [0..nCoefficients_)
	 *  Note: if not 0, this parameter overwrites the interval given by continuousBegin_ and continuousend_.
	 */
	size_t nCoefficients_;
	bool needInit_;
    private:
	void setSampleRate(f64 sampleRate) {
	    if (sampleRate_ != sampleRate) { sampleRate_ = sampleRate; needInit_ = true; }
	}
	void setContinuousBegin(f64 begin) {
	    if (continuousBegin_ != begin) { continuousBegin_ = begin; needInit_ = true; }
	}
	void setContinuousEnd(f64 end) {
	    if (continuousEnd_ != end) { continuousEnd_ = end; needInit_ = true; }
	}
	void setNumberOfCoefficients(size_t n) {
	    if (nCoefficients_ != n) { nCoefficients_ = n; needInit_ = true; }
	}

	void init();
	bool getData(Flow::DataPtr<Flow::Vector<f32> > &x, Flow::DataPtr<Flow::Vector<f32> > &y);
    public:
	static std::string filterName() { return "signal-cross-correlation"; }
	CrossCorrelationNode(const Core::Configuration &c);
	virtual ~CrossCorrelationNode() {}

	Flow::PortId getInput(const std::string &name);
	Flow::PortId getOutput(const std::string &name) { return 0; }

	virtual bool setParameter(const std::string &name, const std::string &value);
	virtual bool configure();
	virtual bool work(Flow::PortId p);
    };
}

#endif // _SIGNAL_CORRELATION_HH
