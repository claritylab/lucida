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
#include "CrossCorrelation.hh"
#include <Math/Complex.hh>
#include <Math/Utilities.hh>

using namespace Core;
using namespace Signal;

//========================================================================================================
CrossCorrelation::CrossCorrelation() :
    begin_(0), end_(0),
    similarityFunctionType_(Multiplication), power_(1.0),
    normalizationType_(None),
    useFastFourierTransform_(true)
{}

void CrossCorrelation::crossCorrelation(const std::vector<Data> &x, const std::vector<Data> &y,
					std::vector<Data> &Rxy)
{
    u32 K = std::max(Core::abs(begin_), Core::abs(end_ - 1));

    RealFastFourierTransform fft(std::max(x.size(), y.size()) + K);

    if (!fft.transform(X_ = x))
	defect();

    if (&x == &y) { // autocorrelation
	Math::transformAlternatingComplexToAlternatingComplex(
	    X_.begin(), X_.end(), X_.begin(), X_.begin(), Math::conjugateMultiplies<Data>());
    } else {
	fft.transform(Y_ = y);
	Math::transformAlternatingComplexToAlternatingComplex(
	    X_.begin(), X_.end(), Y_.begin(), X_.begin(), Math::conjugateMultiplies<Data>());
    }

    RealInverseFastFourierTransform iFft(fft.length(), fft.outputSampleRate());
    if (!iFft.transform(X_))
	defect();

    Rxy.resize(end_ - begin_);
    for(s32 m = begin_; m < end_; ++ m)
	Rxy[m - begin_] = m >= 0 ? *(X_.begin() + m) : *(X_.end() + m);
}

void CrossCorrelation::crossCorrelationWithMultiplication(const std::vector<Data> &x,
							  const std::vector<Data> &y,
							  std::vector<Data> &Rxy)
{
    if (useFastFourierTransform_)
	crossCorrelation(x, y, Rxy);
    else
	crossCorrelation(x, y, Rxy, std::multiplies<Data>());
}

void CrossCorrelation::crossCorrelationWithAbsoluteDifference(const std::vector<Data> &x,
							      const std::vector<Data> &y,
							      std::vector<Data> &Rxy) const
{
    if (power_ == 1.0) {
	crossCorrelation(x, y, Rxy, Math::absoluteDifference<Data>());
    } else if (power_ == 0.5) {
	crossCorrelation(x, y, Rxy, Math::absoluteDifferenceSquareRoot<Data>());
    } else if (power_ == 2.0) {
	crossCorrelation(x, y, Rxy, Math::absoluteDifferenceSquare<Data>());
    } else {
	crossCorrelation(x, y, Rxy, Math::absoluteDifferencePower<Data>(power_));
    }
}

void CrossCorrelation::normalizeUpperBound(const std::vector<Data> &x,
					   const std::vector<Data> &y,
					   std::vector<Data> &Rxy) const
{
    if (similarityFunctionType_ == Multiplication)
	normalize(x, y, Rxy, normalizeCrossCorrelationUpperBound<Data>(x, y));
    else if (similarityFunctionType_ == AbsoluteDifference) {
	if (power_ == 1.0) {
	    normalize(x, y, Rxy, normalizeCrossAbsoluteDifferenceUpperBound<Data>(x, y));
	} else if (power_ == 2.0) {
	    normalize(x, y, Rxy, normalizeCrossSquareDifferenceUpperBound<Data>(x, y));
	} else
	    hope(false); // not implemented
    } else
	defect();
}

void CrossCorrelation::apply(const std::vector<Data> &x, const std::vector<Data> &y,
			     std::vector<Data> &Rxy)
{
    if (similarityFunctionType_ == Multiplication)
	crossCorrelationWithMultiplication(x, y, Rxy);
    else if (similarityFunctionType_ == AbsoluteDifference)
	crossCorrelationWithAbsoluteDifference(x, y, Rxy);
    else
	defect();

    if (normalizationType_ == UnbiasedEstimate)
	normalize(x, y, Rxy, normalizeCrossCorrelationEstimate<Data>(x, y));
    else if (normalizationType_ == UpperBound)
	normalizeUpperBound(x, y, Rxy);
    else if (normalizationType_ != None)
	defect();
}

void CrossCorrelation::apply(const std::vector<Data> &x, const std::vector<Data> &y,
			     std::vector<Data> &Rxy,
			     Core::Component &errorChannels)
{
    if (begin_ > end_)
	errorChannels.error("Discrete begin time (%d) is larger then Discrete end time (%d).", begin_, end_);
    if ((similarityFunctionType_ != Multiplication || !useFastFourierTransform_) &&
	(begin_ <= -((s32)y.size()) || end_ > (s32)x.size())) {
	errorChannels.error("Cannot calculate the interval [%d..%d) with x.size (%zd) and y.size (%zd).",
			    begin_, end_, x.size(), y.size());
    }
    if (Core::isMalformed(x.begin(), x.end()) || Core::isMalformed(y.begin(), y.end()))
	errorChannels.error("One of the inputs contains malformed element.");

    apply(x, y, Rxy);
}

//========================================================================================================
bool BandpassAutocorrelation::work(u32 bandBegin, u32 bandEnd, std::vector<Data> &autocorrelation)
{
    size_t nComponents = R_.nRows();
    require_(autocorrelation.size() == nComponents);
    for(size_t n = 0; n < nComponents; ++ n)
	autocorrelation[n] = R_[n][bandEnd] - R_[n][bandBegin];
    return true;
}

bool BandpassAutocorrelation::init(size_t nComponents, const std::vector<Data>& amplitude)
{
    require(nComponents > 0);
    require(!amplitude.empty());

    amplitudeSquare_.resize(amplitude.size());
    std::transform(amplitude.begin(), amplitude.end(), amplitude.begin(),
		   amplitudeSquare_.begin(), std::multiplies<Data>());

    R_.resize(nComponents, amplitudeSquare_.size());
    u32 nSamples = 2 * (amplitudeSquare_.size() - 1);
    for(size_t n = 0; n < nComponents; ++ n) {
	/* Integral from 0 to pi must be done on both sides
	 * The amplitude spectrum is symetric, so the same data [0..N/2] can be used,
	 * just shifted ones: positive discrete freq.  = [0..N/2-1],
	 * negative negative discrete freq.  [1..N/2] */
	f64 kernel = 1.0;
	f64 conjugateKernel;

	R_[n][0] = 0.0;
	size_t discreteFrequency;
	for(discreteFrequency = 0; discreteFrequency < nSamples / 2; discreteFrequency ++) {
	    conjugateKernel = cos(2.0 * M_PI / nSamples * (discreteFrequency + 1) * n);
	    R_[n][discreteFrequency + 1] = R_[n][discreteFrequency] +
		1.0 / nSamples * (amplitudeSquare_[discreteFrequency] * kernel +
				  amplitudeSquare_[discreteFrequency + 1] * conjugateKernel);
	    kernel = conjugateKernel;
	}
    }
    return true;
}

//========================================================================================================
const ParameterFloat CrossCorrelationNode::paramBegin
("begin", "correlation is calculated for [begin..end) (in continous unit depending on previous nodes)", 0);
const ParameterFloat CrossCorrelationNode::paramEnd
("end", "correlation is calculated for [begin..end) (in continous unit depending on previous nodes)", 0);
const ParameterInt CrossCorrelationNode::paramNumberOfCoefficients
("nr-coefficients", "correlation is calculated for 0, 1, 2, ..nr-coefficients-1 discrete values", 0);

const Choice CrossCorrelationNode::choiceSimilarityFunctionType
("multiplication", Multiplication,
 "absolute-difference", AbsoluteDifference,
 Choice::endMark());
const ParameterChoice  CrossCorrelationNode::paramSimilarityFunctionType
("similarity-function", &choiceSimilarityFunctionType, "type of similarity function", Multiplication);

const ParameterFloat CrossCorrelationNode::paramPower
("power", "power of similarity function", 1.0);

const Choice CrossCorrelationNode::choiceNormalizationType
("none", None,
 "unbiased-estimate", UnbiasedEstimate,
 "upper-bound", UpperBound,
 Choice::endMark());
const ParameterChoice  CrossCorrelationNode::paramNormalizationType
("normalization", &choiceNormalizationType, "type of normalization", None);

const ParameterBool CrossCorrelationNode::paramUseFastFourierTransform
("use-fft", "use/not FFT for correlation", true);


CrossCorrelationNode::CrossCorrelationNode(const Core::Configuration &c) :
    Core::Component(c), Node(c),
    continuousBegin_(0), continuousEnd_(0), sampleRate_(0),
    nCoefficients_(0),
    needInit_(true)
{
    setContinuousBegin(paramBegin(c));
    setContinuousEnd(paramEnd(c));
    setNumberOfCoefficients(paramNumberOfCoefficients(c));
    setSimilarityFunctionType((SimilarityFunctionType)paramSimilarityFunctionType(c));
    setPower(paramPower(c));
    setNormalizationType((NormalizationType)paramNormalizationType(c));
    setUseFastFourierTransform(paramUseFastFourierTransform(c));

    addInputs(2);
    addOutputs(1);
}

void CrossCorrelationNode::init()
{
    if (sampleRate_ <= 0)
	error("Sample rate (%f) is smaller or equal to 0.", sampleRate_);

    s32 begin = (s32)rint(continuousBegin_ * sampleRate_);
    s32 end = (s32)rint(continuousEnd_ * sampleRate_);
    if (nCoefficients_ != 0) {
	if (continuousBegin_ != 0 || continuousEnd_ != 0) {
	    warning("Ambiguous parameters: begin=%f, end=%f vs. nr-coefficients=%zd.\n"\
		    "Nr-coefficients will be used.", continuousBegin_, continuousEnd_, nCoefficients_);
	}
	begin = 0;
	end = nCoefficients_;
    }
    setBegin(begin);
    setEnd(end);
    needInit_ = false;
}

bool CrossCorrelationNode::configure()
{
    Core::Ref<Flow::Attributes> a(new Flow::Attributes());
    f64 sampleRate = 0;
    for (Flow::PortId i = 0; i < nInputs(); i++) {
	Core::Ref<const Flow::Attributes> b = getInputAttributes(i);
	if (!configureDatatype(b, Flow::Vector<f32>::type()))
	    return false;

	f64 sr = atof(b->get("sample-rate").c_str());
	if (sampleRate == 0)
	    sampleRate = sr;
	else if (!Core::isAlmostEqual(sampleRate, sr))
	    error("Sample rate of inputs differ.");

	a->merge(*b);
    }
    setSampleRate(sampleRate);
    return putOutputAttributes(0, a);
}

Flow::PortId CrossCorrelationNode::getInput(const std::string &name)
{
    if (name == "x")
	return 0;
    else if (name == "y")
	return 1;
    return Flow::IllegalPortId;
}

bool CrossCorrelationNode::setParameter(const std::string &name, const std::string &value)
{
    if (paramBegin.match(name))
	setContinuousBegin(paramBegin(value));
    else if (paramEnd.match(name))
	setContinuousEnd(paramEnd(value));
    else if (paramNumberOfCoefficients.match(name))
	setNumberOfCoefficients(paramNumberOfCoefficients(value));
    else if (paramSimilarityFunctionType.match(name))
	setSimilarityFunctionType((SimilarityFunctionType)paramSimilarityFunctionType(value));
    else if (paramPower.match(name))
	setPower(paramPower(value));
    else if (paramNormalizationType.match(name))
	setNormalizationType((NormalizationType)paramNormalizationType(value));
    else if (paramUseFastFourierTransform.match(name))
	setUseFastFourierTransform(paramUseFastFourierTransform(value));
    else
	return false;
    return true;
}

bool CrossCorrelationNode::getData(Flow::DataPtr<Flow::Vector<f32> > &x,
				   Flow::DataPtr<Flow::Vector<f32> > &y)
{
    Node::getData(0, x);
    Node::getData(1, y);

    if ((bool)x && (bool)y)
	return true;

    if (!x && !y)
	return false;

    x.release();
    y.release();

    criticalError("Input streams have different length.");

    return false;
}

bool CrossCorrelationNode::work(Flow::PortId p)
{
    Flow::DataPtr<Flow::Vector<f32> > x, y;

    if (!getData(x, y))
	return putData(0, x.get());

    if (needInit_) init();
    Flow::Vector<f32>* Rxy = new Flow::Vector<f32>;
    Rxy->setTimestamp(*x);

    apply(*x, *y, *Rxy, *this);

    return putData(0, Rxy);
}
