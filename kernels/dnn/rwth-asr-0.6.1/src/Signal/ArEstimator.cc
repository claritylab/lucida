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
#include "ArEstimator.hh"

using namespace Signal;

SegmentwiseArEstimator::SegmentwiseArEstimator() :
    amplitudeLength_(0)
{}

SegmentwiseArEstimator::~SegmentwiseArEstimator()
{}

bool SegmentwiseArEstimator::setSignal(const std::vector<Data> &y)
{
    amplitudeLength_ = y.size();
    return autocorrelation_.init(autocorrelationFunction_.size(), y);
}

bool SegmentwiseArEstimator::work(Data* estimationError, std::vector<Data>* ATilde, Data* energy)
{
    if (checkSegment()) {
	if (autocorrelation_.work(current_segment_begin_, current_segment_end_, autocorrelationFunction_)) {
	    if (leastSquares_.work(autocorrelationFunction_)) {
		if (estimationError) *estimationError = leastSquares_.predictionError();
		if (ATilde) leastSquares_.a(*ATilde);
		if (energy) *energy = autocorrelationFunction_[0];
		return true;
	    }
	}
    }
    return false;
}

//============================================================================

Core::XmlWriter& AutoregressiveCoefficients::dump(Core::XmlWriter &o) const
{
    o << Core::XmlOpen(datatype()->name())
	+ Core::XmlAttribute("start", startTime()) + Core::XmlAttribute("end", endTime());

    o << Core::XmlOpen("gain") << gain_ << Core::XmlClose("gain");

    o << Core::XmlOpen("a") + Core::XmlAttribute("size", a_.size());
    if (!a_.empty()) {
	for (std::vector<Coefficient>::const_iterator i = a_.begin(); i != a_.end() - 1; ++ i)
	    o << *i << " ";
	o << a_.back();
    }
    o << Core::XmlClose("a");

    o << Core::XmlClose(datatype()->name());
    return o;
}

bool AutoregressiveCoefficients::read(Core::BinaryInputStream &i)
{
    i >> gain_;
    u32 s; i >> s; a_.resize(s);
    for(std::vector<Coefficient>::iterator it = a_.begin(); it != a_.end(); it ++) i >> (*it);
    return Timestamp::read(i);
}

bool AutoregressiveCoefficients::write(Core::BinaryOutputStream &o) const
{
    o << gain_;
    o << (u32)a_.size();
    std::copy(a_.begin(), a_.end(), Core::BinaryOutputStream::Iterator<Coefficient>(o));
    return Timestamp::write(o);
}

//============================================================================

bool AutocorrelationToAutoregressionNode::configure()
{
    Core::Ref<Flow::Attributes> attributes(new Flow::Attributes());
    getInputAttributes(0, *attributes);
    if (!configureDatatype(attributes, Flow::Vector<f32>::type()))
	return false;
    attributes->set("datatype", AutoregressiveCoefficients::type()->name());
    return putOutputAttributes(0, attributes);
}

bool AutocorrelationToAutoregressionNode::work(Flow::PortId p)
{
    Flow::DataPtr<Flow::Vector<f32> > autocorrelation;
    if (getData(0, autocorrelation)) {
	Flow::DataPtr<AutoregressiveCoefficients> out;
	if (!autocorrelation->empty()) {
	    if (levinsonLeastSquares_.work(*autocorrelation)) {
		out = Flow::dataPtr(new AutoregressiveCoefficients());
		out->gain() = levinsonLeastSquares_.gain();
		levinsonLeastSquares_.a(out->a());
		out->setTimestamp(*autocorrelation);
	    } else {
		error("Failed to calculate the autoregression coefficients.");
	    }
	} else {
	    error("Input autocorrelation is empty.");
	}
	return putData(0, out.get());
    }
    return putData(0, autocorrelation.get());
}
