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
#ifndef _SIGNAL_FILTERBANK_HH
#define _SIGNAL_FILTERBANK_HH

#include "Utility.hh"
#include "Node.hh"
#include <Core/Parameter.hh>
#include <Flow/Vector.hh>
#include <Flow/StringExpressionNode.hh>
#include <Math/AnalyticFunction.hh>

namespace Signal {

    /** Filter bank
     *  Constrains and parameters:
     *   -Each filter is assumed to have the same length.
     *   -Filters have a central point and relative to this a left (negative number) and a right border.
     *    Outside of the borders (f < left border and f > right border) filter weights are zero.
     *    Left and right borders are configured by 'normalizedCenterPosition', @see FilterBuilder.
     *  -Filters are placed equidistantly given the spacing.
     *   @param shouldWarpCenterPositions_ controls over which axis the spacing is constant:
     *    -yes : filters are placed equidistantly over the warped axis.
     *    -no  : filters are placed equidistantly over the original unwarped axis.
     *        Implemented by simply pre-warping the center positions, since the inverse warping
     *        applied which building the filters will yield equidistant filters over the original unwarped axis.
     *        Note that the filter widths are warped in these cases.
     *  -If spacing is set to zero, it will be set automatically to normalized-center * filter-width,
     *   i.e. the size of the left flank.
     *  -Parts of filters reaching out of the interval [f_min..f_max] are set to zero.
     *  -Positions of first and last filters are controlled by different boundary type classes,
     *   @see FilterBank::Boundary.
     *   Supported types:
     *     -include-boundary
     *     -stretch-to-cover
     *     -emphasize-boundary.
     */
    class FilterBank : public virtual Core::Component {
    public:
	typedef f64 Frequency;
	typedef f32 Data;
	typedef Data FilterWeight;
	enum FilterType { typeTriangular, typeTrapeze, typeRastaTrapeze };
	enum BoundaryType { includeBoundary, stretchToCover, emphasizeBoundary };
	enum NormalizationType { normalizeNone, normalizeSurface };
    public:
	class Filter;
	class FilterBuilder;
	class Boundary;
    private:
	FilterBuilder *builder_;
	Boundary *boundary_;
	Frequency filterWidth_;
	Frequency spacing_;
	Frequency minimumFrequency_;
	Frequency maximumFrequency_;
	Math::UnaryAnalyticFunctionRef discreteToContinuousFunction_;
	std::string warpingFunctionName_;
	Math::UnaryAnalyticFunctionRef warpingFunction_;
	bool shouldWarpDifferenctialUnit_;
	bool shouldWarpCenterPositions_;
	NormalizationType normalizationType_;
	std::vector<Core::Ref<Filter> > filters_;
	bool needInit_;
    private:
	bool init();
	static bool isAlmostInteger(Frequency x);
    public:
	FilterBank(const Core::Configuration &);
	virtual ~FilterBank();

	void setFilterWidth(Frequency width) {
	    if (filterWidth_ != width) { filterWidth_ = width; needInit_ = true; }
	}
	void setSpacing(Frequency spacing) {
	    if (spacing_ != spacing) { spacing_ = spacing; needInit_ = true; }
	}
	void setFilterType(FilterType type);
	void setBoundaryType(BoundaryType type);
	void setMinimumFrequency(Frequency f) {
	    if (minimumFrequency_ != f) { minimumFrequency_ = f; needInit_ = true; }
	}
	void setMaximumFrequency(Frequency f) {
	    if (maximumFrequency_ != f) { maximumFrequency_ = f; needInit_ = true; }
	}
	/**
	 *   Sets a function mapping the discrete input indices to continuous values.
	 */
	void setDiscreteToContinuousFunction(Math::UnaryAnalyticFunctionRef f) {
	    require(f); discreteToContinuousFunction_ = f; needInit_ = true;
	}
	void setWarpingFunction(const std::string &name, Math::UnaryAnalyticFunctionRef f) {
	    require(f); warpingFunctionName_ = name; warpingFunction_ = f; needInit_ = true;
	}
	void setWarpDifferentialUnit(bool should) {
	    if (shouldWarpDifferenctialUnit_ != should) {
		shouldWarpDifferenctialUnit_ = should; needInit_ = true;
	    }
	}
	bool shouldWarpDifferentialUnit() const { return shouldWarpDifferenctialUnit_; }
	void setWarpCenterPositions(bool should) {
	    if (shouldWarpCenterPositions_ != should) {
		shouldWarpCenterPositions_ = should; needInit_ = true;
	    }
	}

	void setNormalizationType(NormalizationType type){
	    if (normalizationType_ != type) { normalizationType_ = type; needInit_ = true; }
	}

	void apply(const std::vector<Data>& in, std::vector<Data>& out);
	Frequency outputSampleRate();
	void dump(Core::XmlWriter &);
	bool isConfigurationAllowed() { return !needInit_ || init(); }
    };

    /**
     *   Filter Bank Node.
     *
     *   Parameters:
     *     warping-function: is a string containing a declaration of a function conform to
     *                       Math::AnalyticFunctionFactory. Example: nest(linear-2(1.2, 0.875), mel)
     *                       This node class is derived from Flow::StringExpressionNode, thus it supports
     *                       parameter resolving with incoming values one or more ports. Example:
     *                       nest(linear-2($input(warping-factor), 0.875), mel). In this example the node
     *                       creates an input port called warping-factor and the incoming values will be
     *                       substituted to the placeholder $input(warping-factor). The main input of the
     *                       node and the different "parameter-ports" are synchronized, thus time stamps
     *                       of the main port are ensured to fall within the start and end times of the
     *                       current parameter value.
     *
     */
    class FilterBankNode : public FilterBank, public Flow::StringExpressionNode {
    private:
	static const Core::Choice choiceFilterType;
	static const Core::ParameterChoice paramFilterType;

	static const Core::ParameterFloat paramFilterWidth;
	static const Core::ParameterFloat paramSpacing;

	static const Core::Choice choiceBoundaryType;
	static const Core::ParameterChoice paramBoundaryType;

	static const Core::ParameterFloat paramFilteringIntervalStart;
	static const Core::ParameterFloat paramFilteringInterval;

	static const Core::Choice choiceNormalizationType;
	static const Core::ParameterChoice paramNormalizationType;

	static const Core::ParameterString paramWarpingFunction;
	static const Core::ParameterBool paramShouldWarpDifferentialUnit;
	static const Core::ParameterBool paramShouldWarpCenterPositions;
    private:
	f64 filteringIntervalStart_;
	f64 filteringInterval_;
	u32 inputSize_;
	f64 sampleRate_;
	bool needInit_;
	Core::XmlChannel dumpChannel_;
    private:
	void setSampleRate(f64 sampleRate) {
	    if (sampleRate_ != sampleRate) { sampleRate_ = sampleRate; needInit_ = true; }
	}
	void setFilteringIntervalStart(f64 interval) {
	    if (filteringIntervalStart_ != interval) { filteringIntervalStart_ = interval; needInit_ = true; }
	}
	void setFilteringInterval(f64 interval) {
	    if (filteringInterval_ != interval) { filteringInterval_ = interval; needInit_ = true; }
	}

	void createAnalyticFunction(Math::UnaryAnalyticFunctionRef &discreteToContinuousFunction,
				    Math::UnaryAnalyticFunctionRef &warpingFunction);
	void init(u32 inputLength);
    public:
	static std::string filterName() { return "signal-filterbank"; }

	FilterBankNode(const Core::Configuration &);
	virtual ~FilterBankNode();

	Flow::PortId getInput(const std::string &name) {
	    return name.empty() ? 0 : StringExpressionNode::getInput(name);
	}
	Flow::PortId getOutput(const std::string &name) { return 0; }
	virtual bool setParameter(const std::string &name, const std::string &value);
	virtual bool configure();
	virtual bool work(Flow::PortId p);
    };
}

#endif // _SIGNAL_FILTERBANK_HH
