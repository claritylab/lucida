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
#include <numeric>
#include "Filterbank.hh"
#include <Math/AnalyticFunctionFactory.hh>
#include <fenv.h>

using namespace Signal;

//============================================================================================

/**
 *  Filter in a filter-bank.
 */
class FilterBank::Filter : public Core::ReferenceCounted {
private:
    /** first index of interval of non-zero weight [start_..end_)*/
    size_t start_;
    /** first index after the interval of non-zero weight [start_..end_) */
    size_t end_;

    std::vector<FilterWeight> weights_;
public:
    Filter(size_t start, size_t end, const std::vector<FilterWeight> &weights);

    void normalize(NormalizationType);
    Data apply(const std::vector<Data> &in) const;
    void dump(Core::XmlWriter &) const;
};

FilterBank::Filter::Filter(size_t start, size_t end, const std::vector<FilterWeight> &weights) :
    start_(start), end_(end), weights_(weights)
{
    require(end_ >= start_);
    require(end_ - start_ == weights_.size());
}

void FilterBank::Filter::normalize(FilterBank::NormalizationType normalizationType)
{
    if (normalizationType != FilterBank::normalizeNone) {
	FilterWeight normalizationTerm = 1;
	if (normalizationType == FilterBank::normalizeSurface) {
	    normalizationTerm = std::accumulate(weights_.begin(), weights_.end(), (FilterWeight)0);
	} else defect();
	std::transform(weights_.begin(), weights_.end(), weights_.begin(),
		       std::bind2nd(std::divides<FilterWeight>(), normalizationTerm));
    }
}

FilterBank::Data FilterBank::Filter::apply(const std::vector<Data> &in) const
{
    require(end_ <= in.size());
    FilterWeight result = 0;
    for(size_t f = start_; f < end_; ++ f)
	result += in[f] * weights_[f - start_];
    return result;
}

void FilterBank::Filter::dump(Core::XmlWriter &o) const
{
    o << Core::XmlOpen("filter");
    for(size_t i = start_; i < end_; ++ i)
	o << i << " " << weights_[i - start_] << "\n";
    o << Core::XmlClose("filter");
}

//============================================================================================

/** Base class for filter builders
 *  Override the following functions to implement a specific filter:
 *    1) normalizedCenterPosition: return the position of the center of the filter in percent.
 *       -0 -> filter has no left flank.
 *       -1 -> filter has no right flank.
 *    2) weight(frequency): returns weight for the given frequency.
 */
class FilterBank::FilterBuilder : public virtual Core::Component {
protected:
    /** Start of filter to build (@see Filter). */
    size_t start_;
    /** End of filter to build (@see Filter). */
    size_t end_;
    /** Weights of filter to build (@see Filter). */
    std::vector<FilterWeight> weights_;

    /** Center point of filter to build. */
    Frequency center_;
    /** Width of filter to build. */
    Frequency width_;
    /** Maximal and minimal input frequency. */
    Frequency maximumFrequency_;
    Frequency minimumFrequency_;
    /** Mapping from discrete input indices to continuous values. */
    Math::UnaryAnalyticFunctionRef discreteToContinuousFunction_;
    /** Mapping from continuous values to discrete input indices. */
    Math::UnaryAnalyticFunctionRef continuousToDiscreteFunction_;
    /**
     *  Used for calculating the delta unit in the warped domain:
     *   delta warped-omega = d warping-function / d omega * delta omega.
     */
    Math::UnaryAnalyticFunctionRef derivedWarpingFunction_;
private:
    virtual bool setStart();
    virtual bool setEnd();
    bool setWeights();
protected:
    virtual FilterWeight weight(Frequency) const = 0;
public:
    FilterBuilder(const Core::Configuration &);
    virtual ~FilterBuilder();

    Core::Ref<FilterBank::Filter> create(
	    Frequency center, Frequency width, Frequency minimumFrequency, Frequency maximumFrequency,
	Math::UnaryAnalyticFunctionRef discreteToContinuousFunction,
	Math::UnaryAnalyticFunctionRef warpingFunction, bool warpDifferentialUnit);

    virtual Frequency normalizedCenterPosition() const = 0;
};

FilterBank::FilterBuilder::FilterBuilder(const Core::Configuration &c) :
    Core::Component(c),
    start_(0), end_(0), center_(0), width_(0)
{}

FilterBank::FilterBuilder::~FilterBuilder()
{}

Core::Ref<FilterBank::Filter> FilterBank::FilterBuilder::create(
	Frequency center, Frequency width, Frequency minimumFrequency, Frequency maximumFrequency,
    Math::UnaryAnalyticFunctionRef discreteToContinuousFunction,
    Math::UnaryAnalyticFunctionRef warpingFunction, bool shouldWarpDifferentialUnit)
{
    Core::Ref<FilterBank::Filter> result;
    center_ = center;
    width_ = width;
    minimumFrequency_ = minimumFrequency;
    maximumFrequency_ = maximumFrequency;

    discreteToContinuousFunction_ = Math::nest(warpingFunction, discreteToContinuousFunction);
    continuousToDiscreteFunction_ = discreteToContinuousFunction_->invert();
    if (continuousToDiscreteFunction_) {
	if (shouldWarpDifferentialUnit) {
	    if (warpingFunction->derive())
		derivedWarpingFunction_ = Math::nest(warpingFunction->derive(), discreteToContinuousFunction);
	} else {
	    derivedWarpingFunction_ = Math::AnalyticFunctionFactory::createConstant(1);
	}
	if (derivedWarpingFunction_) {
	    if (setStart() && setEnd() && setWeights())
		result = Core::ref(new Filter(start_, end_, weights_));
	} else {
	    error("Warping function is not derivable.");
	}
    } else {
	error("Warping function is not invertible.");
    }
    return result;
}

bool FilterBank::FilterBuilder::setStart()
{
    Frequency start = continuousToDiscreteFunction_->value(
	    std::max(center_ - normalizedCenterPosition() * width_, minimumFrequency_));
    if (isAlmostInteger(start))
	start = Core::round(start);
    else
	start = Core::ceil(start);
    if (start >= 0) {
	start_ = (size_t)start;
	return true;
    }
    error("Start point of the filter at center %f became negative (%d).", center_, (int)start);
    return false;
}

bool FilterBank::FilterBuilder::setEnd()
{
    Frequency end = continuousToDiscreteFunction_->value(
	std::min(center_ + (1.0 - normalizedCenterPosition()) * width_, maximumFrequency_));
    if (isAlmostInteger(end))
	end = Core::round(end) + 1;
    else
	end = Core::ceil(end);
    if (end > 0 && start_ < (size_t)end) {
	end_ = (size_t)end;
	return true;
    }
    error("Inconsistent end point of the filter at center %f: start=%zd end=%d.", center_, start_, (int)end);
    return false;
}

bool FilterBank::FilterBuilder::setWeights()
{
    weights_.resize(end_ - start_);
    for(u32 f = start_; f < end_; ++ f) {
	verify(derivedWarpingFunction_->value(f) >= 0);
	weights_[f - start_] = weight(discreteToContinuousFunction_->value(f)) *
	    derivedWarpingFunction_->value(f);
	verify(weights_[f - start_] >= 0);
    }
    return true;
}

/**
 *  Symmetric Triangular Filter Builder.
 */
class SymmetricalTriangularFilterBuilder : public FilterBank::FilterBuilder {
protected:
    virtual FilterBank::FilterWeight weight(FilterBank::Frequency) const;
public:
    SymmetricalTriangularFilterBuilder(const Core::Configuration &c) :
	Core::Component(c), FilterBank::FilterBuilder(c) {}

    virtual FilterBank::Frequency normalizedCenterPosition() const { return 0.5; }
};

FilterBank::FilterWeight SymmetricalTriangularFilterBuilder::weight(FilterBank::Frequency frequency) const
{
    FilterBank::FilterWeight result = (FilterBank::Frequency)1 - Core::abs(frequency - center_) / (width_ / 2);
    /* start_ and end_ control that 'frequency' stays within the interval [center-halfWidth..center+halfWidth],
     * thus the resulting weight has to be larger or equal to zero. Nevertheless rounding error can yield
     * slightly negative weight which will be clipped to zero.
     */
    ensure(result > -Core::Type<FilterBank::FilterWeight>::epsilon);
    return result >= 0 ? result : 0;
}

/**
 *  Trapeze Filter Builder.
 */
class TrapezeFilterBuilder : public FilterBank::FilterBuilder {
private:
    FilterBank::Frequency normalizedMiddleBorder() const { return 0.5 / (1.3 - (-2.5)); }
protected:
    virtual FilterBank::FilterWeight weight(FilterBank::Frequency) const;
public:
    TrapezeFilterBuilder(const Core::Configuration &c) :
	Core::Component(c), FilterBank::FilterBuilder(c) {}

    virtual FilterBank::Frequency normalizedCenterPosition() const { return 2.5 / (1.3 - (-2.5)); }
};

FilterBank::FilterWeight TrapezeFilterBuilder::weight(FilterBank::Frequency frequency) const
{
    FilterBank::Frequency relativeFrequency = frequency - center_;

    FilterBank::Frequency middleLeftBorder = -normalizedMiddleBorder() * width_;
    if (relativeFrequency < middleLeftBorder)
	return pow(10, relativeFrequency - middleLeftBorder);
    else {
	FilterBank::Frequency middleRightBorder = normalizedMiddleBorder() * width_;
	if (relativeFrequency <= middleRightBorder)
	    return 1;
	else
	    return pow(10, -2.5 * (relativeFrequency - middleRightBorder));
    }
}

/**
 *  TrapezeRasta Filter Builder.
 *  (based on the trapeze filter in the ICSI-Sprachcore-Toolkit (Rasta.c))
 */
class TrapezeRastaFilterBuilder : public FilterBank::FilterBuilder {
private:
    FilterBank::Frequency normalizedMiddleBorder() const { return 0.5 / (1.3 - (-2.5)); }
protected:
    virtual FilterBank::FilterWeight weight(FilterBank::Frequency) const;
public:
    TrapezeRastaFilterBuilder(const Core::Configuration &c) :
	Core::Component(c), FilterBank::FilterBuilder(c) {}

    virtual FilterBank::Frequency normalizedCenterPosition() const { return 2.5 / (1.3 - (-2.5)); }

    // Start and end boundaries are different to the normal way, so change them here. Same for weights
    virtual bool setStart();
    virtual bool setEnd();
};

FilterBank::FilterWeight TrapezeRastaFilterBuilder::weight(FilterBank::Frequency frequency) const {
    FilterBank::Frequency relativeFrequency = frequency - center_;
    FilterBank::Frequency middleBorder = normalizedMiddleBorder() * width_; // => 0.5

    if (relativeFrequency <= -middleBorder)
	return pow(10, relativeFrequency + middleBorder);
    else if (relativeFrequency >= middleBorder)
	return pow(10, -2.5 * (relativeFrequency - middleBorder));
    else
	return 1;
}

/**	Just round the index of the start position */
bool TrapezeRastaFilterBuilder::setStart() {
    FilterBank::Frequency start = continuousToDiscreteFunction_->value(
	    std::max(center_ - normalizedCenterPosition() * width_, minimumFrequency_));
    start = Core::round(start);

    // Check for negative boundary values ...
    if (start >= 0) {
	start_ = (size_t)start;
	return true;
    }

    // ... and report the error here
    error("Start point of the filter at center %f became negative (%d).", center_, (int)start);
    return false;
}

/**	Just round the index of the end position */
bool TrapezeRastaFilterBuilder::setEnd() {
    FilterBank::Frequency end = continuousToDiscreteFunction_->value(
	std::min(center_ + (1.0 - normalizedCenterPosition()) * width_, maximumFrequency_));

    end = Core::round(end) + 1;

    // Check for correct end boundary ...
    if (end > 0 && start_ < (size_t)end) {
	end_ = (size_t)end;
	return true;
    }

    // ... and report the error here
    error("Inconsistent end point of the filter at center %f: start=%zd end=%d.", center_, start_, (int)end);
    return false;
}

//============================================================================================

/**
 *  Base class for boundary types.
 *  Boundary classes control:
 *  - position of first filter
 *  - position of last filter
 *  - number of filters
 *  - spacing between filters
 *  - warping of filter center positions
 */
class FilterBank::Boundary : public virtual Core::Component {
protected:
    typedef FilterBank::Frequency Frequency;
protected:
    Frequency filterWidth_;
    Frequency spacing_;
    Frequency normalizedCenterPosition_;
    Frequency minimumFrequency_;
    Frequency maximumFrequency_;
    Math::UnaryAnalyticFunctionRef warpingFunction_;
    Math::UnaryAnalyticFunctionRef inverseWarpingFunction_;
protected:
    void setSpacing(Frequency Spacing);
    bool setWarpingFunction(Math::UnaryAnalyticFunctionRef warpingFunction);
    Frequency postprocessNumberOfFilters(Frequency nFilters) const;
protected:
    Boundary(const Core::Configuration &c) :
	Core::Component(c),
	filterWidth_(0), spacing_(0),
	normalizedCenterPosition_(0),
	minimumFrequency_(0),
	maximumFrequency_(0) {}
public:
    virtual ~Boundary() {}

    /**
     *  Initializes the object without maximumFrequency and warping.
     *  Use to initialize before calling outputSampleRate.
     */
    virtual void init(Frequency filterWidth, Frequency spacing,
		      Frequency normalizedCenterPosition);
    /**
     *  Complete initialization of the object.
     */
    virtual bool init(Frequency filterWidth, Frequency spacing,
		      Frequency normalizedCenterPosition,
		      Frequency minimumFrequency,
		      Frequency maximumFrequency,
		      Math::UnaryAnalyticFunctionRef warpingFunction);

    Frequency filterWidth() const { return filterWidth_; }

    virtual Frequency center(size_t filterIndex) const = 0;
    virtual size_t getNumberOfFilters() const = 0;
    /**
     *  @return is sample rate of the output of the filter-bank.
     *  Thus, output is inverse of distances between the filters.
     *  If centers are warped the distance is constant over the warped axes else
     *  distance is constant over the original unwarped axis.
     */
    virtual Frequency outputSampleRate() const { return (Frequency)1 / spacing_; }
};

void FilterBank::Boundary::init(
    Frequency filterWidth, Frequency spacing,
    Frequency normalizedCenterPosition)
{
    filterWidth_ = filterWidth;
    normalizedCenterPosition_ = normalizedCenterPosition;
    setSpacing(spacing);
}

bool FilterBank::Boundary::init(
    Frequency filterWidth, Frequency spacing,
    Frequency normalizedCenterPosition,
    Frequency minimumFrequency,
    Frequency maximumFrequency,
    Math::UnaryAnalyticFunctionRef warpingFunction)
{
    init(filterWidth, spacing, normalizedCenterPosition);
    minimumFrequency_ = minimumFrequency;
    maximumFrequency_ = maximumFrequency;
    return setWarpingFunction(warpingFunction);
}

void FilterBank::Boundary::setSpacing(Frequency spacing)
{
    verify(normalizedCenterPosition_ > 0);
    verify(filterWidth_ > 0);
    spacing_ = spacing;
    if (spacing_ == 0)
	spacing_ = normalizedCenterPosition_ * filterWidth_;
}

bool FilterBank::Boundary::setWarpingFunction(Math::UnaryAnalyticFunctionRef warpingFunction)
{
    warpingFunction_ = warpingFunction;
    if (!warpingFunction_)
	warpingFunction_ = Math::AnalyticFunctionFactory::createIdentity();
    inverseWarpingFunction_ = warpingFunction_->invert();
    if (!inverseWarpingFunction_)
	error("Warping function is not invertible.");
    return (bool)inverseWarpingFunction_;
}

FilterBank::Boundary::Frequency FilterBank::Boundary::postprocessNumberOfFilters(
    Frequency nFilters) const
{
    if (nFilters < 1) return 1;
    if (isAlmostInteger(nFilters)) return Core::round(nFilters);
    return nFilters;
}

/**
 *  Include Boundary: left and right boundaries of input are included but with small weights.
 *  -First filter starts at center equals spacing.
 *  -Last filter is chosen such that its left border is smaller than maximal-frequency and
 *   its right border is larger or equal to the maximal-frequency.
 *  =>Number of filters = ceil(maximum-frequency - (1.0 - normalized-center) * filter-width) / spacing)
 *  -Sample rate of output vector: 1.0 / spacing.
 *  -Note that if spacing > normalized-center * filter-width, the leftmost few input values will
 *   not contribute to the results.
 */
class IncludeBoundary: public FilterBank::Boundary {
    typedef FilterBank::Boundary Precursor;
public:
    IncludeBoundary(const Core::Configuration &c) :
	Core::Component(c), FilterBank::Boundary(c) {}
    virtual Frequency center(size_t filterIndex) const {
	return warpingFunction_->value(spacing_ * (filterIndex + 1));
    }
    virtual size_t getNumberOfFilters() const;
};

size_t IncludeBoundary::getNumberOfFilters() const
{
    return (size_t)Core::ceil(
	postprocessNumberOfFilters(
	    inverseWarpingFunction_->value(
		maximumFrequency_ - (1 - normalizedCenterPosition_) * filterWidth_) /
	    spacing_));
}

/**
 *  Stretch-To-Cover: filter width and spacing are stretched to cover the complete input exactly,
 *                    thus left and right boundaries of input are included with weight 0.
 *  -Basic idea to ensure the complete coverage of the whole interval [0..maximal-frequency]
 *   with minimal number of filters.
 *  -First filter starts at left-border equals 0.
 *  -Last filter is chosen such that its right border is smaller or equal to maximal-frequency and
 *   the right-border of the next filter would be larger than maximal-frequency.
 *  =>Number of filters = floor((maximal-frequency - filter-width) / spacing + 1)
 *  -Finally, spacing and filter width are multiplied by a number > 1 such that right border of the
 *   last filter falls on the maximal-frequency.
 *  -Sample rate of output vector: since spacing is changed according to the input length,
 *                                 the output sample-rate cannot be retrieved before the first
 *                                 input arrives, thus it will be set to 1.
 *  -Pre-warping of centers is supported in this boundary type since pre-warping introduces non-linearities.
 */
class StretchToCover: public FilterBank::Boundary {
    typedef FilterBank::Boundary Precursor;
public:
    StretchToCover(const Core::Configuration &c) :
	Core::Component(c), FilterBank::Boundary(c) {}

    virtual bool init(Frequency filterWidth, Frequency spacing,
		      Frequency normalizedCenterPosition,
		      Frequency minimumFrequency,
		      Frequency maximumFrequency,
		      Math::UnaryAnalyticFunctionRef warpingFunction);

    virtual Frequency center(size_t filterIndex) const {
	return minimumFrequency_ + spacing_ * filterIndex + normalizedCenterPosition_ * filterWidth_;
    }
    virtual size_t getNumberOfFilters() const {
	return (size_t)Core::floor(postprocessNumberOfFilters(
		    (maximumFrequency_ - minimumFrequency_ - filterWidth_) / spacing_ + 1));
    }
    virtual Frequency outputSampleRate() const { return (Frequency)1; }
};

bool StretchToCover::init(
    Frequency filterWidth, Frequency spacing,
    Frequency normalizedCenterPosition,
    Frequency minimumFrequency,
    Frequency maximumFrequency,
    Math::UnaryAnalyticFunctionRef warpingFunction)
{
    if (warpingFunction) // unwarping of center not possible in this boundary type.
	return false;

    Precursor::init(filterWidth, spacing, normalizedCenterPosition,
	    minimumFrequency, maximumFrequency, warpingFunction);

    size_t nFilters = getNumberOfFilters();
    Frequency coverage = (spacing_ * (nFilters - 1) + filterWidth_) / (maximumFrequency_-minimumFrequency_);
    if (nFilters == 1 && Core::isSignificantlyGreater(coverage, 1))
	return true; // If single filter ensures full coverage, then do not shrink it.

    require(Core::isAlmostEqual(coverage, 1) || coverage < 1); // If fails then nFilters was too high
    filterWidth_ /= coverage;
    spacing_ /= coverage;
    return true;
}

/**
 *  Emphasize Boundary: left and right boundaries of input are included with large weights.
 *  -First filter starts at center equals 0.
 *  -Last filter is chosen such that its center is smaller than maximal-frequency + spacing.
 *  =>Number of filters = ceil(maximal-frequency / spacing + 1)
 *  -Sample rate of output vector: 1.0 / spacing.
 */
class EmphasizeBoundary: public FilterBank::Boundary {
    typedef FilterBank::Boundary Precursor;
public:
    EmphasizeBoundary(const Core::Configuration &c) :
	Core::Component(c), FilterBank::Boundary(c) {}

    virtual Frequency center(size_t filterIndex) const {
	return warpingFunction_->value(spacing_ * filterIndex);
    }
    virtual size_t getNumberOfFilters() const {
	return (size_t)Core::floor(
	    postprocessNumberOfFilters(
		inverseWarpingFunction_->value(maximumFrequency_) / spacing_ + 1));
    }
};

//============================================================================================
FilterBank::FilterBank(const Core::Configuration &c) :
    Core::Component(c),
    builder_(0),
    boundary_(0),
    filterWidth_(0),
    spacing_(0),
    minimumFrequency_(0),
    maximumFrequency_(0),
    shouldWarpDifferenctialUnit_(true),
    shouldWarpCenterPositions_(true),
    normalizationType_(normalizeNone),
    needInit_(true)
{}

FilterBank::~FilterBank()
{
    delete builder_;
    delete boundary_;
}

void FilterBank::setFilterType(FilterType type)
{
    delete builder_;
    switch(type) {
    case typeTriangular: builder_ = new SymmetricalTriangularFilterBuilder(select("filter-builder"));
	break;
    case typeTrapeze: builder_ = new TrapezeFilterBuilder(select("filter-builder"));
	break;
    case typeRastaTrapeze: builder_ = new TrapezeRastaFilterBuilder(select("filter-builder"));
	break;
    default:
	defect();
    }
    needInit_ = true;
}

void FilterBank::setBoundaryType(BoundaryType type)
{
    delete boundary_;
    switch(type) {
    case includeBoundary: boundary_ = new IncludeBoundary(select("boundary"));
	break;
    case stretchToCover: boundary_ = new StretchToCover(select("boundary"));
	break;
    case emphasizeBoundary: boundary_ = new EmphasizeBoundary(select("boundary"));
	break;
    default:
	defect();
    }
    needInit_ = true;
}

bool FilterBank::init()
{
    if (!builder_ || !boundary_ || (maximumFrequency_ == 0) ||
	!boundary_->init(filterWidth_, spacing_,
		builder_->normalizedCenterPosition(), minimumFrequency_, maximumFrequency_,
			 shouldWarpCenterPositions_ ? Math::UnaryAnalyticFunctionRef() : warpingFunction_)) {
	return false;
    }
    filters_.resize(boundary_->getNumberOfFilters());
    for(size_t i = 0; i < filters_.size(); ++ i) {
	filters_[i] = builder_->create(boundary_->center(i),
		boundary_->filterWidth(), minimumFrequency_, maximumFrequency_,
				       discreteToContinuousFunction_,
				       warpingFunction_, shouldWarpDifferentialUnit());
	if (!filters_[i]) return false;
	filters_[i]->normalize(normalizationType_);
    }
    return !(needInit_ = false);
}

void FilterBank::apply(const std::vector<Data>& in, std::vector<Data>& out)
{
    if (needInit_ && !init()) defect();
    out.resize(filters_.size());
    for(size_t f = 0; f < filters_.size(); ++ f)
	out[f] = filters_[f]->apply(in);
}

void FilterBank::dump(Core::XmlWriter &o)
{
    if (needInit_) init();
    o << Core::XmlOpen("filter-bank") + Core::XmlAttribute("warping-function", warpingFunctionName_);
    for(size_t f = 0; f < filters_.size(); ++ f)
	filters_[f]->dump(o);
    o << Core::XmlClose("filter-bank");
}

FilterBank::Frequency FilterBank::outputSampleRate()
{
    if (needInit_) {
	verify(builder_);
	boundary_->init(filterWidth_, spacing_, builder_->normalizedCenterPosition());
    }
    return boundary_->outputSampleRate();
}

bool FilterBank::isAlmostInteger(Frequency x)
{
    static Frequency tolerance = 1e-10;
    return Core::abs(x - Core::round(x)) < tolerance;
}

//============================================================================================
const Core::Choice FilterBankNode::choiceFilterType(
    "triangular", typeTriangular,
    "trapeze", typeTrapeze,
    "trapezeRasta", typeRastaTrapeze,
    Core::Choice::endMark());
const Core::ParameterChoice FilterBankNode::paramFilterType(
    "type", &choiceFilterType, "filter bank type", typeTriangular);

const Core::ParameterFloat FilterBankNode::paramFilterWidth(
    "filter-width", "width of one filter in continuous units.", 268.258, 0);
const Core::ParameterFloat FilterBankNode::paramSpacing(
    "spacing", "distance between two neighboring filters.", 0, 0);

const Core::Choice FilterBankNode::choiceBoundaryType(
    "include-boundary", FilterBank::includeBoundary,
    "stretch-to-cover", FilterBank::stretchToCover,
    "emphasize-boundary", FilterBank::emphasizeBoundary,
    Core::Choice::endMark());
const Core::ParameterChoice FilterBankNode::paramBoundaryType(
    "boundary", &choiceBoundaryType, "boundary type", FilterBank::stretchToCover);

const Core::ParameterFloat FilterBankNode::paramFilteringIntervalStart(
    "filtering-interval-start", "Filters are placed only over this frequency.", 0, 0);
const Core::ParameterFloat FilterBankNode::paramFilteringInterval(
    "filtering-interval", "Filters are placed only below this frequency.", Core::Type<f32>::max, 0);

const Core::Choice FilterBankNode::choiceNormalizationType(
    "none", FilterBank::normalizeNone,
    "surface", FilterBank::normalizeSurface,
    Core::Choice::endMark());
const Core::ParameterChoice FilterBankNode::paramNormalizationType(
    "normalization", &choiceNormalizationType, "filterbank type", FilterBank::normalizeNone);

const Core::ParameterString FilterBankNode::paramWarpingFunction(
    "warping-function", "warping function declaration");
const Core::ParameterBool FilterBankNode::paramShouldWarpDifferentialUnit(
    "warp-differential-unit", "Controls if derivative of warping function is applied.", true);
const Core::ParameterBool FilterBankNode::paramShouldWarpCenterPositions(
    "warp-center-positions", "Controls if filter center position are warped.", true);

FilterBankNode::FilterBankNode(const Core::Configuration &c) :
    Core::Component(c),
    Node(c),
    FilterBank(c),
    StringExpressionNode(c, 1),
    filteringIntervalStart_(0),
    filteringInterval_(Core::Type<f32>::max),
    inputSize_(0),
    sampleRate_(0),
    needInit_(true),
    dumpChannel_(c, "dump-filters")
{
    addInput(0);
    addOutput(0);

    setFilterType((FilterType)paramFilterType(c));
    setFilterWidth(paramFilterWidth(c));
    setSpacing(paramSpacing(c));
    setBoundaryType((BoundaryType)paramBoundaryType(c));
    setFilteringIntervalStart(paramFilteringIntervalStart(c));
    setFilteringInterval(paramFilteringInterval(c));
    setWarpDifferentialUnit(paramShouldWarpDifferentialUnit(c));
    setWarpCenterPositions(paramShouldWarpCenterPositions(c));
    setNormalizationType((FilterBank::NormalizationType)paramNormalizationType(c));
    Flow::StringExpressionNode::setTemplate(paramWarpingFunction(c));
}

FilterBankNode::~FilterBankNode()
{}

void FilterBankNode::init(u32 inputSize)
{
    if (inputSize == 0)
	error("Empty input vector.");
    inputSize_ = inputSize;
    if (sampleRate_ <= 0)
	error("Sample rate is not set.");
    respondToDelayedErrors();
    Math::UnaryAnalyticFunctionRef discreteToContinuousFunction;
    Math::UnaryAnalyticFunctionRef warpingFunction;
    createAnalyticFunction(discreteToContinuousFunction, warpingFunction);
    respondToDelayedErrors();

    f64 maximumFrequency = Math::nest(warpingFunction, discreteToContinuousFunction)->value(inputSize_ - 1);
    if (filteringInterval_ < Core::Type<f32>::max) {
	if (maximumFrequency < filteringInterval_) {
	    error("Filter interval (%f) is broader than the the input vector (%f). ",
		  filteringInterval_, maximumFrequency);
	} else
	    maximumFrequency = filteringInterval_;
    }

    setDiscreteToContinuousFunction(discreteToContinuousFunction);
    setWarpingFunction(StringExpressionNode::value(), warpingFunction);
    setMinimumFrequency(filteringIntervalStart_);
    setMaximumFrequency(maximumFrequency);
    if (!isConfigurationAllowed())
	error("This configuration of the filter bank is not allowed.");
    respondToDelayedErrors();
    if (dumpChannel_.isOpen()) dump(dumpChannel_);
    needInit_ = false;
}

void FilterBankNode::createAnalyticFunction(
    Math::UnaryAnalyticFunctionRef &discreteToContinuousFunction,
    Math::UnaryAnalyticFunctionRef &warpingFunction)
{
    verify(inputSize_ > 0);

    Math::AnalyticFunctionFactory factory(select(paramWarpingFunction.name()));
    factory.setSampleRate(sampleRate_);
    factory.setDomainType(Math::AnalyticFunctionFactory::continuousDomain);

    // Discrete to continuous
    discreteToContinuousFunction = factory.createScaling(1 / sampleRate_);
    factory.setMaximalArgument(discreteToContinuousFunction->value(inputSize_ - 1));

    // Continuous to warped continuous
    warpingFunction = factory.createIdentity();
    std::string declaration = StringExpressionNode::value();
    if (!declaration.empty()) {
	warpingFunction = factory.createUnaryFunction(declaration);
	if (!warpingFunction) error("Failed to create warping function.");
    }
}

bool FilterBankNode::setParameter(const std::string &name, const std::string &value)
{
    if (paramFilterType.match(name))
	setFilterType((FilterType)paramFilterType(value));
    else if (paramFilterWidth.match(name))
	setFilterWidth(paramFilterWidth(value));
    else if (paramSpacing.match(name))
	setSpacing(paramSpacing(value));
    else if (paramBoundaryType.match(name))
	setBoundaryType((BoundaryType)paramBoundaryType(value));
    else if (paramFilteringIntervalStart.match(name))
	setFilteringIntervalStart(paramFilteringIntervalStart(value));
    else if (paramFilteringInterval.match(name))
	setFilteringInterval(paramFilteringInterval(value));
    else if (paramShouldWarpDifferentialUnit.match(name))
	setWarpDifferentialUnit(paramShouldWarpDifferentialUnit(value));
    else if (paramShouldWarpCenterPositions.match(name))
	setWarpCenterPositions(paramShouldWarpCenterPositions(value));
    else if (paramNormalizationType.match(name))
	setNormalizationType((FilterBank::NormalizationType)paramNormalizationType(value));
    else if (paramWarpingFunction.match(name))
	setTemplate(paramWarpingFunction(value));
    else
	return false;
    return true;
}

bool FilterBankNode::configure()
{
    Core::Ref<Flow::Attributes> attributes(new Flow::Attributes());
    getInputAttributes(0, *attributes);
    if (!configureDatatype(attributes, Flow::Vector<f32>::type()))
	return false;
    setSampleRate(atof(attributes->get("sample-rate").c_str()));
    if (!StringExpressionNode::configure(*attributes))
	return false;
    attributes->set("sample-rate", outputSampleRate());
    attributes->set("datatype", Flow::Vector<f32>::type()->name());
    return putOutputAttributes(0, attributes);
}

bool FilterBankNode::work(Flow::PortId p)
{
    Flow::DataPtr<Flow::Vector<f32> > in;
    if (!getData(0, in))
	return putData(0, in.get());

    if (StringExpressionNode::update(*in) || needInit_)
	init(in->size());

    if (in->size() != inputSize_) {
	criticalError("Input size (%zd) does not match the expected input size (%d)",
		      in->size(), inputSize_);
    }
    Flow::Vector<f32> *out = new Flow::Vector<f32>;
    out->setTimestamp(*in);
    apply(*in, *out);
    return putData(0, out);
}
