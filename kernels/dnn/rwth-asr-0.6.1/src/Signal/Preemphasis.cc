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
#include <Core/Utility.hh>
#include "Preemphasis.hh"

using namespace Signal;



// Preemphasis
//////////////

Preemphasis::Preemphasis() :
    alpha_(1.0),
    previous_(0),
    previousEndTime_(0),
    sampleRate_(0),
    needInit_(true) {
}

void Preemphasis::init(f32 initialValue) {
    previous_ = initialValue;
    previousEndTime_ = 0;
    needInit_ = false;
}


void Preemphasis::setAlpha(f32 alpha) {
    if (alpha_ != alpha) {
	alpha_ = alpha;
	needInit_ = true;
    }
}

void Preemphasis::setSampleRate(f64 sampleRate) {
    if (sampleRate_ != sampleRate) {
	sampleRate_ = sampleRate;
	needInit_ = true;
    }
}

void Preemphasis::apply(Flow::Vector<f32> &v) {
    ensure(sampleRate_);

    if (needInit_ || !v.equalsToStartTime(previousEndTime_))
	init(v.empty() ? 0 : v[0]);

    f32 current;
    if (alpha_ != 1.0) {
	for (u32 i = 0; i < v.size(); i++) {
	    current = v[i];
	    v[i] -= alpha_ * previous_;
	    previous_ = current;
	}
    } else {
	current = previous_;
	previous_ = v[v.size() - 1];
	for (u32 i = v.size() - 1; i > 0; i--) v[i] -= v[i - 1];
	v[0] -= current;
    }

    previousEndTime_ = v.endTime();
}


// PreemphasisNode
//////////////////


Core::ParameterFloat Signal::PreemphasisNode::paramAlpha
  ("alpha", "preemphasis weight", 1);


PreemphasisNode::PreemphasisNode(const Core::Configuration &c) :
    Core::Component(c), SleeveNode(c)
{
    setAlpha(paramAlpha(c));
}


bool PreemphasisNode::setParameter(const std::string &name, const std::string &value) {

    if (paramAlpha.match(name))
	setAlpha(paramAlpha(value));
    else
	return false;

    return true;
}


bool PreemphasisNode::configure() {

    Core::Ref<const Flow::Attributes> a = getInputAttributes(0);
    if (!configureDatatype(a, Flow::Vector<f32>::type()))
	return false;

    setSampleRate(atof(a->get("sample-rate").c_str()));

    reset();

    return putOutputAttributes(0, a);
}


bool PreemphasisNode::work(Flow::PortId p) {

    Flow::DataPtr<Flow::Vector<f32> > in;
    if (!getData(0, in)) {
	if (in == Flow::Data::eos())
	    Preemphasis::reset();
	return putData(0, in.get());
    }

    in.makePrivate();
    apply(*in);

    return putData(0, in.get());
}
