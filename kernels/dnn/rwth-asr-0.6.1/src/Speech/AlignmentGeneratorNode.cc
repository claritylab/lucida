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
#include "AlignmentGeneratorNode.hh"
#include <Flow/DataAdaptor.hh>

using namespace Speech;

/** AlignmentGeneratorNode
 */
AlignmentGeneratorNode::AlignmentGeneratorNode(
    const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c)
{
    addInputs(1);
    addOutputs(1);
}

bool AlignmentGeneratorNode::configure()
{
    Core::Ref<Flow::Attributes> attributes(new Flow::Attributes);
    getInputAttributes(1, *attributes);
    if (!configureDatatype(attributes, Flow::DataAdaptor<ConstSegmentwiseFeaturesRef>::type())) {
	return false;
    }

    attributes->set("datatype", Flow::DataAdaptor<AlignmentGeneratorRef>::type()->name());
    return Precursor::configure() && putOutputAttributes(0, attributes);
}

bool AlignmentGeneratorNode::work(Flow::PortId p)
{
    if (!Precursor::work(p)) {
	return false;
    }

    Flow::DataPtr<Flow::DataAdaptor<ConstSegmentwiseFeaturesRef> > in;
    if (!getData(1, in)) {
	error("could not read port 1");
	return putData(0, in.get());
    }
    alignmentGenerator_->setSpeechSegmentId(segmentId());
    alignmentGenerator_->setFeatures(in->data());
    Flow::DataAdaptor<AlignmentGeneratorRef> *out = new Flow::DataAdaptor<AlignmentGeneratorRef>();
    out->data() = alignmentGenerator_;
    require(!getData(1, in));
    return putData(0, out) && putData(0, in.get());
}

void AlignmentGeneratorNode::initialize(ModelCombinationRef modelCombination)
{
    Precursor::initialize(modelCombination);
    alignmentGenerator_ = Core::ref(
	new PhonemeSequenceAlignmentGenerator(
	    select("segmentwise-alignment"),
	    modelCombination));
    needInit_ = false;
}
