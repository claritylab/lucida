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
#include "PeakDetection.hh"
#include <cmath>
#include <numeric>
#include <Flow/DataAdaptor.hh>

using namespace Core;
using namespace Signal;

// PeakDetection
////////////////


PeakDetection::PeakDetection() :
    minPosition_(0), maxPosition_(0),
    maxWidthHalf_(0), heightAverageWidthHalf_(0),
    widthHeightFactor_(.5), mountainWidth_(false),
    sampleRate_(0), needInit_(true)
{}

u32 PeakDetection::getMaximumIndex(const std::vector<Amplitude> &v) const
{
    ensure(minPosition_ < maxPosition_ && maxPosition_ < v.size());

    return std::max_element(v.begin() + minPosition_,
			    v.begin() + maxPosition_ + 1) - v.begin();
}

u32 PeakDetection::getMaximalPeakIndex(const std::vector<Amplitude> &v) const
{
    Amplitude maximumPeakValue = Core::Type<Amplitude>::min;
    u32 maximumPeakPosition = Core::Type<u32>::max;

    for(u32 peakBegin = 1; peakBegin + 1 < v.size(); ++ peakBegin) {

	if (v[peakBegin - 1] < v[peakBegin] && v[peakBegin] >= v[peakBegin + 1]) {

	    // seekto the right end of the inflextion
	    u32 peakEnd = peakBegin;
	    for(; peakEnd + 1 < v.size() && v[peakEnd] == v[peakEnd + 1];
		++ peakEnd);

	    if (peakEnd + 1 < v.size() && v[peakEnd] > v[peakEnd + 1] &&
		v[peakEnd] > maximumPeakValue &&
		(peakBegin <= maxPosition_ && peakEnd >= minPosition_)) {

		maximumPeakValue = v[peakEnd];

		maximumPeakPosition = (peakBegin + peakEnd) / 2;
		maximumPeakPosition = std::min(std::max(maximumPeakPosition, minPosition_),
					       maxPosition_);
	    }

	    peakBegin = peakEnd;
	}
    }

    return maximumPeakPosition;
}

f32 PeakDetection::getMaximumPosition(const std::vector<Amplitude> &v)
{
    if (needInit_)
	init();

    return (f32)(getMaximumIndex(v) / sampleRate_);
}

f32 PeakDetection::getMaximalPeakPosition(const std::vector<Amplitude> &v)
{
    if (needInit_)
	init();

    u32 index = getMaximalPeakIndex(v);
    return index < Core::Type<u32>::max ? (f32)(index / sampleRate_) : -1;
}

PeakDetection::Amplitude PeakDetection::getMaximum(const std::vector<Amplitude> &v)
{
    if (needInit_)
	init();

    return v[getMaximumIndex(v)];
}

PeakDetection::Amplitude PeakDetection::getMaximalPeakValue(const std::vector<Amplitude> &v)
{
    if (needInit_)
	init();

    u32 index = getMaximalPeakIndex(v);
    return index < Core::Type<u32>::max ? v[index] : 0;
}

PeakDetection::Amplitude
PeakDetection::getAverageHeight(const std::vector<Amplitude> &v, const u32 peakIndex) const
{
    if (continuousHeightAverageWidth_ > 0) {
	u32 width = heightAverageWidthHalf_ - 1;

	// sum over [peakIndex - width .. peakIndex - 2] within [minPosition_..maxPosition_]
	u32 firstLeft = peakIndex > minPosition_ + width ? peakIndex - width : minPosition_;
	u32 lastLeft = peakIndex > minPosition_ + 1 ? peakIndex - 1 : minPosition_;
	Amplitude sumLeft = std::accumulate(v.begin() + firstLeft, v.begin() + lastLeft, 0.0);

	// sum over [peakIndex + 2 .. peakIndex + width] within [minPosition_..maxPosition_]
	u32 firstRight = std::min(peakIndex + 2, maxPosition_ + 1);
	u32 lastRight = std::min(peakIndex + width, maxPosition_) + 1;
	Amplitude sumRight = std::accumulate(v.begin() + firstRight, v.begin() + lastRight, 0.0);

	return (sumLeft + sumRight) / Amplitude(lastLeft - firstLeft + lastRight - firstRight);

    } else if (continuousHeightAverageWidth_ == 0) {
	return std::accumulate(v.begin() + minPosition_, v.begin() + maxPosition_ + 1, 0.0) /
	    Amplitude(maxPosition_ - minPosition_ + 1);
    } else if (continuousHeightAverageWidth_ == -1) {
	return std::accumulate(v.begin(), v.end(), 0.0) / Amplitude(v.size());
    } else {
	hope(v.size() >= 1);
	return v[1];
    }
}

PeakDetection::Amplitude PeakDetection::getNormalizedMaximum(const std::vector<Amplitude> &v)
{
    if (needInit_)
	init();

    u32 peakIndex = getMaximumIndex(v);
    Amplitude peakValue = v[peakIndex];
    Amplitude averageHeight = getAverageHeight(v, peakIndex);

    return peakValue / averageHeight;
}

f32 PeakDetection::getHeightConfidence(const Amplitude peakValue,
				       const Amplitude averageHeight) const
{
    f32 result = peakValue / averageHeight - 1.0;

    if (result < 0.0)
	return 0.0;
    else if (result > 1.0)
	return 1.0;

    return result;
}

f32 PeakDetection::getHeightConfidence(const std::vector<Amplitude> &v)
{
    if (needInit_)
	init();

    u32 peakIndex = getMaximumIndex(v);
    return getHeightConfidence(v[peakIndex], getAverageHeight(v, peakIndex));
}

u32 PeakDetection::getPeakWidth(const std::vector<Amplitude> &v,
				const u32 peakIndex,
				const Amplitude widthHeight) const
{
    ensure(v[peakIndex] > widthHeight);
    ensure(!isMalformed(v[peakIndex]));
    ensure(peakIndex >= maxWidthHalf_ && (peakIndex + maxWidthHalf_) < v.size());


    u32 width, result = 0;

    for(width = 1; width <= maxWidthHalf_; ++ width) {
	if (v[peakIndex - width] <= widthHeight)
	    break;
    }
    result += (width - 1);

    for(width = 1; width <= maxWidthHalf_; ++ width) {
	if (v[peakIndex + width] <= widthHeight)
	    break;
    }
    result += (width - 1);

    return result;
}

u32 PeakDetection::getMountainWidth(const std::vector<Amplitude> &v,
				    const u32 peakIndex,
				    const Amplitude widthHeight) const
{
    require(v[peakIndex] > widthHeight);
    require(!Core::isMalformed(v[peakIndex]));
    require(peakIndex >= maxWidthHalf_ && (peakIndex + maxWidthHalf_) < v.size());

    u32 width;
    for(width = maxWidthHalf_; width > 0; -- width) {
	if (v[peakIndex - width] >= widthHeight ||
	    v[peakIndex + width] >= widthHeight)
	    break;
    }

    return width * 2;
}

PeakDetection::Amplitude
PeakDetection::getWidthHeight(const Amplitude peakValue, const Amplitude averageHeight) const
{
    ensure(peakValue >= averageHeight);
    hope(widthHeightFactor_ >= 0.0 && widthHeightFactor_ <= 1.0);

    if (peakValue > averageHeight && widthHeightFactor_ < 1.0)
	return widthHeightFactor_ * peakValue + (1.0 - widthHeightFactor_) * averageHeight;

    // widthHeight must be less peakValue.
    // In case of constant signal or widthHeightFactor_ == 1.0 take the 3/4 of peakValue.
    return peakValue * 3.0 / 4.0;
}

f32 PeakDetection::getWidthConfidence(const std::vector<Amplitude> &v,
				      const u32 peakIndex,
				      const Amplitude peakValue,
				      const Amplitude averageHeight) const
{
    hope(maxWidthHalf_ > 0);

    if (Core::isMalformed(peakValue))
	return 0;

    Amplitude widthHeight = getWidthHeight(peakValue, averageHeight);

    u32 width = mountainWidth_ ? getMountainWidth(v, peakIndex, widthHeight) :
	getPeakWidth(v, peakIndex, widthHeight);

    return f32(maxWidthHalf_ * 2 - width) / f32(maxWidthHalf_ * 2);
}

f32 PeakDetection::getWidthConfidence(const std::vector<Amplitude> &v)
{
    if (needInit_)
	init();

    u32 peakIndex = getMaximumIndex(v);
    Amplitude averageHeight = getAverageHeight(v, peakIndex);
    return getWidthConfidence(v, peakIndex, v[peakIndex], averageHeight);
}

f32 PeakDetection::getConfidence(const std::vector<Amplitude> &v)
{
    if (needInit_)
	init();

    u32 peakIndex = getMaximumIndex(v);
    Amplitude peakValue = v[peakIndex];
    Amplitude averageHeight = getAverageHeight(v, peakIndex);

    return getHeightConfidence(peakValue, averageHeight) *
	getWidthConfidence(v, peakIndex, peakValue, averageHeight);
}

void PeakDetection::init()
{
    verify(sampleRate_ > 0.0);

    minPosition_ = (u32)rint(continuousMinPosition_ * sampleRate_);
    maxPosition_ = (u32)rint(continuousMaxPosition_ * sampleRate_);
    maxWidthHalf_ = (u32)rint(continuousMaxWidth_ * sampleRate_ / 2.0);
    heightAverageWidthHalf_ = (u32)rint(continuousHeightAverageWidth_ * sampleRate_ / 2.0);

    needInit_ = false;
}

u32 PeakDetection::minPosition()
{
    if (needInit_)
	init();

    return minPosition_;
}

u32 PeakDetection::maxPosition()
{
    if (needInit_)
	init();

    return maxPosition_;
}

// PeakDetectionNode
////////////////////

ParameterFloat PeakDetectionNode::paramMinPosition
("min-position", "min peak position in continous units depending on previous nodes");

ParameterFloat PeakDetectionNode::paramMaxPosition
("max-position", "max peak position in continous units depending on previous nodes");

ParameterFloat PeakDetectionNode::paramMaxWidth
("max-width", "width with which bandwith is normed (in continous units depending on previous nodes)");

ParameterFloat PeakDetectionNode::paramHeightAverageWidth
("height-average-width",
 "width over which heigth average is taken (in cont. units depending on prev. nodes, 0: [min-peak-pos..max-peak=pos])",
 0, -2);

ParameterFloat PeakDetectionNode::paramWidthHeightFactor
("width-height-factor",
 "widthHeight = factor * peakValue + (1 - factor) * averageHeight (factor = 1: peakValue * 3/4)", 1, 0, 1);

ParameterBool PeakDetectionNode::paramMountainWidth
("use-mountain-width", "peak/mountain (includes close peaks) width for width confidence", true);


PeakDetectionNode::PeakDetectionNode(const Configuration &c) :
    Core::Component(c), Node(c)
{
    setContinuousMinPosition(paramMinPosition(c));
    setContinuousMaxPosition(paramMaxPosition(c));
    setContinuousMaxWidth(paramMaxWidth(c));
    setContinuousHeightAverageWidth(paramHeightAverageWidth(c));
    setWidthHeightFactor(paramWidthHeightFactor(c));
    setMountainWidth(paramMountainWidth(c));

    addInput(0);
    addOutputs(8);
}

bool PeakDetectionNode::setParameter(const std::string &name, const std::string &value)
{
    if (paramMinPosition.match(name))
	setContinuousMinPosition(paramMinPosition(value));
    else if (paramMaxPosition.match(name))
	setContinuousMaxPosition(paramMaxPosition(value));
    else if (paramMaxWidth.match(name))
	setContinuousMaxWidth(paramMaxWidth(value));
    else if (paramHeightAverageWidth.match(name))
	setContinuousHeightAverageWidth(paramHeightAverageWidth(value));
    else if (paramWidthHeightFactor.match(name))
	setWidthHeightFactor(paramWidthHeightFactor(value));
    else if (paramMountainWidth.match(name))
	setMountainWidth(paramMountainWidth(value));
    else
	return false;

    return true;
}

bool PeakDetectionNode::configure()
{
    Core::Ref<Flow::Attributes> a(new Flow::Attributes());
    getInputAttributes(0, *a);
    if (!configureDatatype(a, Flow::Vector<f32>::type()))
	return false;

    setSampleRate(atof(a->get("sample-rate").c_str()));

    if (sampleRate() <= 0.0)
	error("Sample rate (%f) is smaller or equal to 0.", sampleRate());

    if (continuousMinPosition() >= continuousMaxPosition()) {
	error("min-position (%f) is larger or equal to max-position (%f).",
	      continuousMinPosition(), continuousMaxPosition());
    }

    a->set("datatype", Flow::Float32::type()->name());
    a->remove("sample-rate");

    bool status = true;
    for(Flow::PortId port = 0; port < nOutputs(); ++ port) {
	if (!Node::putOutputAttributes(port, a))
	    status = false;
    }
    return status;
}

Flow::PortId PeakDetectionNode::getOutput(const std::string &name)
{
    if (name == "maximum-position")
	return 0;
    if (name == "maximal-peak-position")
	return 1;
    if (name == "maximum")
	return 2;
    if (name == "maximal-peak-value")
	return 3;
    if (name == "normalized-maximum")
	return 4;
    if (name == "confidence")
	return 5;
    if (name == "height-confidence")
	return 6;
    if (name == "width-confidence")
	return 7;
    return Flow::IllegalPortId;
}

bool PeakDetectionNode::putData(const Flow::PortId port, const Flow::Timestamp &timestamp, const f32 v)
{
    Flow::Float32 *out = new Flow::Float32(v);
    out->setTimestamp(timestamp);
    return Node::putData(port, out);
}

bool PeakDetectionNode::putData(Flow::Data *d)
{
    bool success = false;
    for(Flow::PortId port = 0; port < nOutputs(); ++ port) {
	if (Node::putData(port, d))
	    success = true;
    }
    return success;
}

bool PeakDetectionNode::work(Flow::PortId p)
{
    Flow::DataPtr<Flow::Vector<f32> > in;

    if (!getData(0, in))
	return putData(in.get());

    if (in->size() <= maxPosition()) {
	criticalError("Input size (%zd) is smaller or equal to max-position (%d).",
		      in->size(), maxPosition());
    }

    bool success = false;
    if (nOutputLinks(0) > 0 && putData(0, *in, getMaximumPosition(*in)))
	success = true;
    if (nOutputLinks(1) > 0 && putData(1, *in, getMaximalPeakPosition(*in)))
	success = true;
    if (nOutputLinks(2) > 0 && putData(2, *in, getMaximum(*in)))
	success = true;
    if (nOutputLinks(3) > 0 && putData(3, *in, getMaximalPeakValue(*in)))
	success = true;
    if (nOutputLinks(4) > 0 && putData(4, *in, getNormalizedMaximum(*in)))
	success = true;
    if (nOutputLinks(5) > 0 && putData(5, *in, getConfidence(*in)))
	success = true;
    if (nOutputLinks(6) > 0 && putData(6, *in, getHeightConfidence(*in)))
	success = true;
    if (nOutputLinks(7) > 0 && putData(7, *in, getWidthConfidence(*in)))
	success = true;

    return success;
}
