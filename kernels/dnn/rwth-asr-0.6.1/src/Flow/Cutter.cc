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
#include "Cutter.hh"

using namespace Flow;

CutterNode::CutterNode(const Core::Configuration &c) :
    Core::Component(c),
    SleeveNode(c) {
    id_ = "";
    featureSequence_.resize(0);
}


Core::ParameterFloat CutterNode::paramStartTime
("start-time", "start-time of output", 0.0);
Core::ParameterFloat CutterNode::paramEndTime
("end-time", "end-time of output", Core::Type<f32>::max);
Core::ParameterString CutterNode::paramId
("id", "changing the id resets the cached features");

bool CutterNode::setParameter(const std::string &name, const std::string &value) {
    if (paramStartTime.match(name)) setStartTime(paramStartTime(value));
    else if (paramEndTime.match(name)) setEndTime(paramEndTime(value));
    else if (paramId.match(name)) setId(paramId(value));
    else
	return false;
    return true;
}

void CutterNode::setId(const std::string &id) {
    if (id != id_) {
	// clear cache
	featureSequence_.resize(0);
	position_ = 0;
	id_ = id;
    }
}

void CutterNode::fillCache()
{
    DataPtr<Data> d;
    getData(0, d);
    while( d && d.get() != Data::eos() ) {
	featureSequence_.push_back(d);
	getData(0, d);
    }
}

void CutterNode::seekToStartTime()
{
    if (featureSequence_.size() == 0) {
	// empty cache -> nothing to seek
	return;
    }

    DataPtr<Timestamp> currentItem(featureSequence_[position_]);
    if (currentItem->startTime() > startTime_) {
	while (position_ > 0 && DataPtr<Timestamp>(featureSequence_[position_])->startTime() > startTime_)
	    position_--;
    } else if (currentItem->startTime() < startTime_) {
	while (position_ < featureSequence_.size() && DataPtr<Timestamp>(featureSequence_[position_])->startTime() < startTime_)
	    position_++;
    }
}

bool CutterNode::configure()
{
    seekToStartTime();
    return SleeveNode::configure();
}

bool CutterNode::work(PortId output) {
    if ( featureSequence_.size() == 0 ) {
	fillCache();
	seekToStartTime();
    }
    size_t readPosition = position_;
    position_++;
    if (readPosition < featureSequence_.size() &&
	DataPtr<Timestamp>(featureSequence_[readPosition])->startTime() <= endTime_ )
	return putData(0, featureSequence_[readPosition].get());
    return putData(0, Data::eos());
}
