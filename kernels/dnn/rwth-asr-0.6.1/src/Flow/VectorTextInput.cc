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
// $Id: VectorTextInput.cc 9621 2014-05-13 17:35:55Z golik $

#include <Core/XmlParser.hh>
#include "VectorTextInput.hh"

using namespace Flow;


class VectorTextInputNodeBase::Parser :
    public Core::XmlParser
{
    VectorTextInputNodeBase *n_;
    std::string buffer_;
protected:
    virtual void startDocument() {
	n_->timeInS_ = n_->offsetInS_;
    }

    virtual void endDocument() {
	verify(!n_->out_);
    }

    virtual void startElement(const char *name, const Core::XmlAttributes atts) {
	if (std::string(name) == n_->type_->name()) {
	    const char *size = atts["size"];
	    n_->create(size != 0 ? atoi(size) : 0);

	    const char *start = atts["start"];
	    if (start)
		n_->out_->setStartTime(atof(start) + n_->offsetInS_);

	    const char *end = atts["end"];
	    if (end)
		n_->out_->setEndTime(atof(end) + n_->offsetInS_);

	    buffer_.clear();
	}
    }

    virtual void endElement(const char *name) {
	if (std::string(name) == n_->type_->name()) {
	    verify(n_->out_);

	    n_->setElements(buffer_);

	    if (n_->out_->startTime() == 0.0)
		n_->out_->setStartTime(n_->timeInS_);

	    if (n_->out_->endTime() == 0.0)
		n_->out_->setEndTime(n_->timeInS_ + n_->lengthInS_);

	    n_->timeInS_ += n_->shiftInS_;

	    n_->issue();
	}
    }

    virtual void characters(const char *ch, int length) {
	buffer_.insert(buffer_.size(), ch, length);
    }

public:
    Parser(const Core::Configuration &c, VectorTextInputNodeBase *n) :
	Core::XmlParser(c), n_(n) {}
};


const Core::ParameterString VectorTextInputNodeBase::paramFileName(
    "file", "input text file name");
const Core::ParameterFloat VectorTextInputNodeBase::paramLength(
    "length", "frame length in s", 1, 0);
const Core::ParameterFloat VectorTextInputNodeBase::paramShift(
    "shift", "frame shift in s", 0, 0);
const Core::ParameterFloat VectorTextInputNodeBase::paramOffset(
    "offset", "offset in s applied to read time marks", 0, 0);
const Core::ParameterInt VectorTextInputNodeBase::paramSampleRate(
    "sample-rate", "sample-rate");


VectorTextInputNodeBase::VectorTextInputNodeBase(
    const Core::Configuration &c, const Datatype *type) :
    Core::Component(c), SourceNode(c),
    timeInS_(0), shiftInS_(0), lengthInS_(0), sampleRate_(0),
    type_(type),
    fileinfoChannel_(c, "text-file-info")
{
    parser_ = new Parser(c, this);
    fileName_ = paramFileName(c);
    shiftInS_ = paramShift(c);
    offsetInS_ = paramOffset(c);
    lengthInS_ = paramLength(c);
    sampleRate_ = paramSampleRate(c);
}

bool VectorTextInputNodeBase::configure() {
    Core::Ref<Attributes> a(new Attributes());
    a->set("datatype", type_->name());
    a->set("frame-shift", (f32)shiftInS_);
    a->set("sample-rate", sampleRate_);
    return putOutputAttributes(0, a);
}

bool VectorTextInputNodeBase::setParameter(
    const std::string &name, const std::string &value)
{
    if (paramFileName.match(name)) fileName_ = paramFileName(value);
    else if (paramShift.match(name)) shiftInS_ = paramShift(value);
    else if (paramLength.match(name)) lengthInS_ = paramLength(value);
    else if (paramSampleRate.match(name)) sampleRate_ = paramSampleRate(value);
    else if (paramOffset.match(name)) offsetInS_ = paramOffset(value);
    else return false;
    return true;
}

void VectorTextInputNodeBase::issue() {
    putData(0, out_.get());
    out_.reset();
}

bool VectorTextInputNodeBase::work(PortId p) {
    if (!fileName_.empty()) {
	if (fileinfoChannel_.isOpen()) {
	    fileinfoChannel_ << (Core::XmlEmpty("reading") + Core::XmlAttribute("file", fileName_));
	}
	parser_->parseFile(fileName_.c_str());
	fileName_.erase();
    }
    return putEos(0);
}
