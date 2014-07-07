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
#include "SegmentNode.hh"
#include "SegmentwiseFeatures.hh"
#include <Flow/DataAdaptor.hh>

using namespace Speech;

/** SegmentNode
 */
const Core::ParameterString SegmentNode::paramSegmentId(
    "id",
    "segment identifier for caches etc.");

SegmentNode::SegmentNode(
    const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c),
    needInit_(true),
    segmentId_(paramSegmentId(config))
{
    addInputs(1);
}

bool SegmentNode::setParameter(const std::string &name, const std::string &value)
{
    if (paramSegmentId.match(name)) {
	segmentId_ = paramSegmentId(value);
	return true;
    }
    return Precursor::setParameter(name, value);
}

bool SegmentNode::configure()
{
    Core::Ref<Flow::Attributes> attributes(new Flow::Attributes);
    getInputAttributes(0, *attributes);
    if (!configureDatatype(attributes, Flow::DataAdaptor<ModelCombinationRef>::type())) {
	return false;
    }
    return true;
}

bool SegmentNode::work(Flow::PortId)
{
    if (needInit_) {
	Flow::DataPtr<Flow::DataAdaptor<ModelCombinationRef> > in;
	if (!getData(0, in)) {
	    error("could not read port 0");
	    return false;
	}
	initialize(in->data());
	//		require(!getData(0, in));
    }
    return true;
}

/**
 * SegmentwiseFeaturesNode
 */

const Core::ParameterBool SegmentwiseFeaturesNode::paramNoDependencyCheck(
	"no-dependency-check",
	"do not check any dependencies",
	false
	);


SegmentwiseFeaturesNode::SegmentwiseFeaturesNode(
    const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c),
    noDependencyCheck_(paramNoDependencyCheck(c))
{
    addInputs(1);
    addOutputs(1);
}

bool SegmentwiseFeaturesNode::configure()
{
    Core::Ref<Flow::Attributes> attributes(new Flow::Attributes);
    getInputAttributes(1, *attributes);
    if (!configureDatatype(attributes, Feature::FlowFeature::type())) {
	return false;
    }

    attributes->set("datatype", Flow::DataAdaptor<ConstSegmentwiseFeaturesRef>::type()->name());
    return Precursor::configure() && putOutputAttributes(0, attributes);
}

bool SegmentwiseFeaturesNode::work(Flow::PortId p)
{
    if (!Precursor::work(p)) {
	return false;
    }
    Flow::DataPtr<Feature::FlowFeature> in;
    Flow::DataAdaptor<ConstSegmentwiseFeaturesRef> *out = new Flow::DataAdaptor<ConstSegmentwiseFeaturesRef>();
    out->invalidateTimestamp();
    SegmentwiseFeaturesRef features(new SegmentwiseFeatures);
    bool firstFeature = true;
    while (getData(1, in)) {
	Core::Ref<Feature> feature(new Feature(in));
	if (firstFeature) {
	    if (!noDependencyCheck_)
	    checkFeatureDependencies(*feature);
	    firstFeature = false;
	}
	features->push_back(feature);
	out->expandTimestamp(feature->timestamp());
    }
    if (features->empty()) {
	out->invalidateTimestamp();
	return false;
    } else {
	out->data() = features;
	return putData(0, out) && putData(0, in.get());
    }
}

void SegmentwiseFeaturesNode::checkFeatureDependencies(const Mm::Feature &feature) const
{
    Mm::FeatureDescription description(*this, feature);
    if (acousticModel_ && !acousticModel_->isCompatible(description)) {
	acousticModel_->respondToDelayedErrors();
    }
}

void SegmentwiseFeaturesNode::initialize(ModelCombinationRef modelCombination)
{
    Precursor::initialize(modelCombination);
    acousticModel_ = modelCombination->acousticModel();
    needInit_ = false;
}
