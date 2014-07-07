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
#include "AlignmentFromLattice.hh"
#include <Flow/DataAdaptor.hh>
#include <Flf/FlfCore/Lattice.hh>

using namespace Speech;

const Core::ParameterString AlignmentFromLatticeNode::paramSegmentId(
    "id",
    "segment identifier for caches.");

const Core::ParameterBool AlignmentFromLatticeNode::paramWriteAlphabet(
    "write-alignment-alphabet", "write alphabet information into written alignments", true);

AlignmentFromLatticeNode::AlignmentFromLatticeNode(const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c),
    writeAlphabet_(paramWriteAlphabet(c)),
    needInit_(true)
{
    addInputs(3);
    addOutputs(1);
    segmentId_ = paramSegmentId(config);
}

bool AlignmentFromLatticeNode::setParameter(const std::string &name, const std::string &value)
{
    if (paramSegmentId.match(name)) {
	segmentId_ = paramSegmentId(value);
    } else if (paramWriteAlphabet.match(name)) {
	writeAlphabet_ = paramWriteAlphabet(value);
    } else {
	return false;
    }
    return true;
}

bool AlignmentFromLatticeNode::configure()
{
    Core::Ref<Flow::Attributes> attributes(new Flow::Attributes());

    getInputAttributes(1, *attributes);
    if (!configureDatatype(attributes, Feature::FlowFeature::type())) {
	return false;
    }

    getInputAttributes(2, *attributes);
    if (!configureDatatype(attributes, Flow::DataAdaptor<ModelCombinationRef>::type())) {
	return false;
    }

    attributes->set("datatype", Flow::DataAdaptor<Alignment>::type()->name());
    return putOutputAttributes(0, attributes);
}

void AlignmentFromLatticeNode::initialize()
{
    verify(!alignmentGenerator_);
    alignmentGenerator_ = Core::Ref<PhonemeSequenceAlignmentGenerator>(
	new PhonemeSequenceAlignmentGenerator(
	    select("segmentwise-alignment"),
	    modelCombination_));
    respondToDelayedErrors();
    needInit_ = false;
}

bool AlignmentFromLatticeNode::work(Flow::PortId p)
{
    if (needInit_) {
	Flow::DataPtr<Flow::DataAdaptor<ModelCombinationRef> > in;
	getData(2, in);
	modelCombination_ = in->data();
	initialize();
    }

    verify(alignmentGenerator_);

    Flow::DataAdaptor<Alignment> *alignment = new Flow::DataAdaptor<Alignment>();
    alignment->invalidateTimestamp();

    Flow::DataPtr<Feature::FlowFeature> in;
    alignmentGenerator_->setSpeechSegmentId(segmentId_);
    SegmentwiseFeaturesRef features(new SegmentwiseFeatures);
    while (getData(1, in)) {
	Core::Ref<Feature> feature(new Feature(in));
	features->feed(feature);
	alignment->expandTimestamp(feature->timestamp());
    }
    alignmentGenerator_->setFeatures(features);

    Flow::DataPtr<Flow::DataAdaptor<Flf::ConstLatticeRef> > lin;
    Flf::ConstLatticeRef lattice;
    while (getData(0, lin)) {
	lattice = lin->data();
    }
    alignmentGenerator_->getAlignment(alignment->data(), lattice);

    if(writeAlphabet_)
	alignment->data().setAlphabet(modelCombination_->acousticModel()->allophoneStateAlphabet());

    return putData(0, alignment) && putData(0, in.get());
}
