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
#include "DataSource.hh"

using namespace Speech;

const Core::ParameterString DataSource::paramMainStreamName(
	"main-port-name", "name of the main data source port", "features");
const Core::ParameterBool DataSource::paramNoProgressIndication(
	"no-progress-indication", "disable progress indication for usage in embedded flow networks", false);
const Core::ParameterBool DataSource::paramPushSinks(
	"push-sinks", "whether data should be pushed into all network sinks", true);

DataSource::DataSource(const Core::Configuration &c, bool loadFromFile) :
    Component(c),
    Precursor(c, loadFromFile),
    mainPortId_(Flow::IllegalPortId),
    startTime_(Core::Type<Flow::Time>::max),
    endTime_(Core::Type<Flow::Time>::min),
    progressIndicator_("", "ms"),
    noProgressIndication_(paramNoProgressIndication(c)),
    pushSinks_(paramPushSinks(c))
{
    std::string name(paramMainStreamName(c));
    mainPortId_ = getOutput(name);

    if (mainPortId_ == Flow::IllegalPortId){
		error("Flow network does not have an output named \"%s\"", name.c_str());
    }
}

DataSource::~DataSource()
{}

void DataSource::initialize(Bliss::Segment *s)
{
	if(pushSinks_)
	    go();
	if(!noProgressIndication_){
		progressIndicator_.setTask(s->fullName());
		progressIndicator_.start();
		progressIndicator_.setTotal(int(segmentDuration(s) * 1000.0));
	}

    std::fill(nFrames_.begin(), nFrames_.end(), 0);

    startTime_ = Core::Type<Flow::Time>::max;
    endTime_ = Core::Type<Flow::Time>::min;
}

Flow::Time DataSource::segmentDuration(Bliss::Segment *s)
{
    std::istringstream durationAttr(getAttribute(mainPortId_, "total-duration"));
    Flow::Time duration;
    if (durationAttr >> duration)
		return std::min(s->end(), duration) - s->start();
    return s->end() - s->start();
}

void DataSource::finalize()
{
	if(pushSinks_)
	    go();
	if(!noProgressIndication_){
		progressIndicator_.finish(false);
	}
}

void DataSource::updateProgressStatus(Flow::PortId portId, const Flow::DataPtr<Flow::Data> d)
{
    if (portId == mainPortId_) {
		Flow::DataPtr<Flow::Timestamp> t(d);
		if (t) {
			if (startTime_ > t->startTime())
				startTime_ =  t->startTime();
			if (endTime_ <  t->endTime())
				endTime_ =  t->endTime();
			if(!noProgressIndication_){
				progressIndicator_.notify(int(roundf((endTime_ - startTime_) * 1000.0)));
			}
		}
    }
    if (portId >= (Flow::PortId)nFrames_.size())
		nFrames_.resize(portId + 1, 0);
    ++ nFrames_[portId];
}

bool DataSource::getData(Flow::PortId portId, Core::Ref<Feature> &feature)
{
    Flow::DataPtr<Flow::Timestamp> out;
    while (getData(portId, out)) {
	if (convert(out, feature))
	    return true;
    }
    return false;
}

bool DataSource::convert(Flow::DataPtr<Flow::Timestamp> from, Core::Ref<Feature> &to)
{
    to = Core::ref(new Feature());
    if (to->take(from))
		return true;
    error("Received Flow packet has an unknown type. It will be ignored.");
    return false;
}
