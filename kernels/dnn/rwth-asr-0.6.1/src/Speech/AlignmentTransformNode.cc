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
#include "AlignmentTransformNode.hh"
#include "Alignment.hh"
#include "ModelCombination.hh"
#include <Flow/DataAdaptor.hh>
#include <typeinfo>

using namespace Speech;

/** AlignmentTransformNode
 */
AlignmentTransformNode::AlignmentTransformNode(
    const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c)
{
    addInputs(1);
    addOutputs(1);
}

AlignmentTransformNode::~AlignmentTransformNode()
{}

bool AlignmentTransformNode::configure() {
    Core::Ref<Flow::Attributes> attributes(new Flow::Attributes());
    getInputAttributes(0, *attributes);

    if (!configureDatatype(attributes, Flow::DataAdaptor<Alignment>::type()))
	return false;

    attributes->set("datatype", Flow::DataAdaptor<Alignment>::type()->name());
    return putOutputAttributes(0, attributes);
}

bool AlignmentTransformNode::work(Flow::PortId p) {
    Flow::DataPtr<Flow::DataAdaptor<Alignment> > in;
    while (getData(0, in)) {
	in.makePrivate();
	Alignment &a = in->data();
	transform(a);
	putData(0, in.get());
    }
    return putData(0, in.get());
}

/** AlignmentGammaCorrectionNode
 */
const Core::ParameterFloat AlignmentGammaCorrectionNode::paramExponent(
    "exponent", "alignment weights are exponentiated by this value", 0.0);

const Core::ParameterBool AlignmentGammaCorrectionNode::paramNormalize(
    "normalize", "normalize weights after exponentiation", true);

AlignmentGammaCorrectionNode::AlignmentGammaCorrectionNode(
    const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c)
{}

bool AlignmentGammaCorrectionNode::setParameter(const std::string &name, const std::string &value) {
    if (paramExponent.match(name)) {
	exponent_ = paramExponent(value);
	return true;
    } else if (paramNormalize.match(name)) {
	normalize_ = paramNormalize(value);
	return true;
    }
    return Precursor::setParameter(name, value);
}

void AlignmentGammaCorrectionNode::transform(Alignment &a) {
    a.gammaCorrection(exponent_);
    if (normalize_)
	a.normalizeWeights();
}

/** AlignmentCombineItemsNode
 */
const Core::ParameterChoice AlignmentCombineItemsNode::paramSemiringType(
    "semiring-type",
    &Fsa::SemiringTypeChoice,
    "define semiring to combine items",
    Fsa::SemiringTypeProbability);

AlignmentCombineItemsNode::AlignmentCombineItemsNode(
    const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c),
    semiring_(Fsa::getSemiring((Fsa::SemiringType)paramSemiringType(config)))
{}

void AlignmentCombineItemsNode::transform(Alignment &a) {
    a.combineItems(semiring_);
}

/** AlignmentFilterWeightsNode
 */
const Core::ParameterFloat AlignmentFilterWeightsNode::paramMinWeight(
    "min-weight", "only alignment items with a weight greater or equal than min-weight survive", 0.0);

const Core::ParameterFloat AlignmentFilterWeightsNode::paramMaxWeight(
    "max-weight", "only alignment items with a weight less or equal than max-weight survive", 1.0);

AlignmentFilterWeightsNode::AlignmentFilterWeightsNode(
    const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c)
{}

bool AlignmentFilterWeightsNode::setParameter(const std::string &name, const std::string &value) {
    if (paramMinWeight.match(name)) {
	minWeight_ = paramMinWeight(value);
	return true;
    } else if (paramMaxWeight.match(name)) {
	maxWeight_ = paramMaxWeight(value);
	return true;
    }
    return Precursor::setParameter(name, value);
}

void AlignmentFilterWeightsNode::transform(Alignment &a) {
    a.filterWeights(minWeight_, maxWeight_);
}

/** AlignmentResetWeightsNode
 */

const Core::ParameterFloat AlignmentResetWeightsNode::paramPreviousWeight(
    "previous-weight", "only alignment items with a weight smaller than this weight will be reseted", 0.0);

const Core::ParameterFloat AlignmentResetWeightsNode::paramNewWeight(
    "new-weight", "new weight for selected alignment items", 0.0);

const Core::ParameterBool AlignmentResetWeightsNode::paramRestrict(
    "restrict", "whether the transformation should be restricted to selected phones", false);

const Core::ParameterStringVector AlignmentResetWeightsNode::paramSelectPhones(
    "select-phones", "If restrict is true, only reset weights on alignment items aligned to the selected phones.\
		      Wildcards can be used at boundaries to select multiple phones.", ",");

const Core::Choice AlignmentResetWeightsNode::choiceMode(
    "larger-than", modeLargerThan,
    "smaller-than", modeSmallerThan,
    "tied-larger-than", modeTiedLargerThan,
    "tied-smaller-than", modeTiedSmallerThan,
    "nothing", modeNone,
    Core::Choice::endMark());

const Core::ParameterChoice AlignmentResetWeightsNode::paramMode(
    "mode", &choiceMode, "select function to treat alignment weights", modeNone);

AlignmentResetWeightsNode::AlignmentResetWeightsNode(
	const Core::Configuration &c) :
	Core::Component(c),
	Precursor(c),
	previousWeight_(paramPreviousWeight(config)),
	newWeight_(paramNewWeight(config)),
	mode_(paramMode(config)),
	restrict_(paramRestrict(config))
{
    if (mode_ == modeTiedSmallerThan ||
	mode_ == modeTiedLargerThan ||
	restrict_) {
	ModelCombination modelCombination(
	    select("model-combination"), ModelCombination::useAcousticModel,
	    Am::AcousticModel::noEmissions | Am::AcousticModel::noStateTransition);
	modelCombination.load();
	acousticModel_ = modelCombination.acousticModel();
    }
    if(restrict_)
    {
	std::vector<std::string> phoneSelection = paramSelectPhones(config);
	for(std::vector<std::string>::const_iterator selectionIt = phoneSelection.begin();
	    selectionIt != phoneSelection.end(); ++selectionIt)
	{
	    std::set<Bliss::Phoneme::Id> phones = acousticModel_->phonemeInventory()->parseSelection(*selectionIt);
	    selectPhones_.insert(phones.begin(), phones.end());
}
	log() << "restricting transformation to " << selectPhones_.size() << " selected phones";
    }
}

bool AlignmentResetWeightsNode::setParameter(const std::string &name, const std::string &value) {
	if (paramPreviousWeight.match(name)) {
		previousWeight_ = paramPreviousWeight(value);
		return true;
	} else if (paramNewWeight.match(name)) {
		newWeight_ = paramNewWeight(value);
		return true;
	} else if (paramMode.match(name)) {
		mode_ = (s32) paramMode(value);
		return true;
	}else if(paramRestrict.match(name) || paramSelectPhones.match(name))
	    error() << "'restrict' and 'select-phones' parameters must not be assigned dynamically";

	return Precursor::setParameter(name, value);
}

void AlignmentResetWeightsNode::transform(Alignment &a) {
    if(restrict_)
    {
	f64 transformation = 0;
	u32 selected = 0, transformed = 0, alreadyTransformed = 0, skipped = 0;
	if(mode_ != modeSmallerThan && mode_ != modeLargerThan)
	    error() << "restricted weight transformation is only implemented for 'smaller-than' and 'larger-than' modes, actual: " << mode_;

	std::cout << "mode " << mode_ << " previous " << previousWeight_ << " new " << newWeight_ << std::endl;
	std::vector<Alignment::Frame> frames;
	a.getFrames(frames);
	std::vector<Alignment::Frame>::const_iterator frame_i;
	for (frame_i = frames.begin(); frame_i != frames.end(); ++frame_i) {
	    Alignment::iterator state_i, state_i_end; //start- and end-states for timeframe i
	    for (Core::tie(state_i, state_i_end) = *frame_i; state_i != state_i_end; ++state_i) {
		Am::AllophoneState alloState = acousticModel_->allophoneStateAlphabet()->allophoneState(state_i->emission);
		if(selectPhones_.count(alloState.allophone()->central()))
		{
		    ++selected;
		    if((mode_ == modeSmallerThan && state_i->weight < previousWeight_) ||
		       (mode_ == modeLargerThan && state_i->weight > previousWeight_))
		    {
			transformation += newWeight_ - state_i->weight;
			if(state_i->weight != newWeight_)
			{
			    state_i->weight = newWeight_;
			    ++transformed;
			}else{
			    ++alreadyTransformed;
			}
		    }
		}
		++skipped;
	    }
	}
	log() << "selected alignment items: " << selected << " transformed: " << transformed <<  " were already transformed: " << alreadyTransformed << " skipped: " << skipped;
	if(transformed)
	    log() << "average alignment weight transformation: " << transformation/transformed;

	return;
    }
    if (mode_ == modeSmallerThan)
	a.resetWeightsSmallerThan(Mm::Weight(previousWeight_), Mm::Weight(newWeight_));
    else if (mode_ == modeLargerThan)
	a.resetWeightsLargerThan(Mm::Weight(previousWeight_), Mm::Weight(newWeight_));
    else if (mode_ == modeTiedSmallerThan ||
	     mode_ == modeTiedLargerThan) {
	std::vector<Alignment::Frame> frames;
	a.getFrames(frames);

	std::vector<Alignment::Frame>::const_iterator frame_i;
	for (frame_i = frames.begin(); frame_i != frames.end(); ++frame_i){
	    Alignment::iterator state_i, state_i_end; //start- and end-states for timeframe i
	    Core::hash_map<u32, f32> frameStateProb;
	    Core::hash_map<u32, u32> frameStateCounts;
	    for (Core::tie(state_i, state_i_end) = *frame_i; state_i != state_i_end; ++state_i) {
		u32 stateId = acousticModel_->emissionIndex(state_i->emission);
		if(frameStateProb.count(stateId) > 0) {
		    frameStateProb[stateId] += f32(state_i->weight);
		    frameStateCounts[stateId]++;
		} else {
		    frameStateProb[stateId] = f32(state_i->weight);
		    frameStateCounts[stateId] = 1;
		}
	    }

	    if (mode_ == modeTiedSmallerThan)
		for (Core::tie(state_i, state_i_end) = *frame_i; state_i != state_i_end; ++state_i) {
		    u32 stateId = acousticModel_->emissionIndex(state_i->emission);
		    if (frameStateProb[stateId] < previousWeight_)
			state_i->weight = newWeight_ / f32(frameStateCounts[stateId]);
		}
	    if (mode_ == modeTiedLargerThan)
		for (Core::tie(state_i, state_i_end) = *frame_i; state_i != state_i_end; ++state_i) {
		    u32 stateId = acousticModel_->emissionIndex(state_i->emission);
		    if (frameStateProb[stateId] > previousWeight_)
			state_i->weight = newWeight_ / f32(frameStateCounts[stateId]);
		}
	}
    }
}

const Core::ParameterBool AlignmentMapAlphabet::paramForceMap(
    "force-map", "enforce mapping, abort if no mapping alphabet has been stored", false);
const Core::ParameterBool AlignmentMapAlphabet::paramSkipMismatch(
    "skip-mismatch", "skip alignment items which could not be mapped into the new alphabet (leads to gappy alignment)", false);

/** AlignmentMapAlphabet */

AlignmentMapAlphabet::AlignmentMapAlphabet(
	const Core::Configuration &c) :
	Core::Component(c),
	Precursor(c),
	forceMap_(paramForceMap(c)),
	skipMismatch_(paramSkipMismatch(c))
{
    ModelCombination modelCombination(
	select("model-combination"), ModelCombination::useAcousticModel,
	Am::AcousticModel::noEmissions | Am::AcousticModel::noStateTransition | Am::AcousticModel::noStateTying);
    modelCombination.load();
    acousticModel_ = modelCombination.acousticModel();
}

void AlignmentMapAlphabet::transform(Alignment &a) {
    // std::cout << "map using allophone state alphabet with " << acousticModel_->allophoneStateAlphabet()->nClasses() << " classes" << " " << typeid(*acousticModel_->allophoneStateAlphabet()).name() << std::endl;
    bool mapped = a.setAlphabet(acousticModel_->allophoneStateAlphabet(), skipMismatch_);
    if(a.size() && !mapped && forceMap_)
	error() << "No alphabet mapping information is available in the alignment, can not map";
}

/** BinaryAlignmentTransformNode
 */
BinaryAlignmentTransformNode::BinaryAlignmentTransformNode(
    const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c)
{
    addInputs(2);
    addOutputs(1);
}

BinaryAlignmentTransformNode::~BinaryAlignmentTransformNode()
{}

bool BinaryAlignmentTransformNode::configure() {
    Core::Ref<Flow::Attributes> attributes(new Flow::Attributes());

    getInputAttributes(0, *attributes);
    if (!configureDatatype(attributes, Flow::DataAdaptor<Alignment>::type())) {
	return false;
    }
    attributes->set("datatype", Flow::DataAdaptor<Alignment>::type()->name());

    getInputAttributes(1, *attributes);
    if (!configureDatatype(attributes, Flow::DataAdaptor<Alignment>::type())) {
	return false;
    }
    attributes->set("datatype", Flow::DataAdaptor<Alignment>::type()->name());

    return putOutputAttributes(0, attributes);
}

bool BinaryAlignmentTransformNode::work(Flow::PortId p) {
    Flow::DataPtr<Flow::DataAdaptor<Alignment> > in1;
    Flow::DataPtr<Flow::DataAdaptor<Alignment> > in2;
    if (!getData(0, in1)) {
	error("could not read port 0");
	return putData(0, in1.get());
    }
    if (!getData(1, in2)) {
	error("could not read port 1");
	return putData(0, in2.get());
    }
    in1.makePrivate();
    transform(in1->data(), in2->data());
    putData(0, in1.get());
    require(!getData(0, in1));
    require(!getData(1, in2));
    return putData(0, in1.get());
}

/** AlignmentMultiplyAlignmentsNode
 */
AlignmentMultiplyAlignmentsNode::AlignmentMultiplyAlignmentsNode(
    const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c)
{}

AlignmentMultiplyAlignmentsNode::~AlignmentMultiplyAlignmentsNode()
{}

void AlignmentMultiplyAlignmentsNode::transform(Alignment &a1, Alignment &a2) {
    a1 *= a2;
}


/** AlignmentMultiplyWeightsNode
 */
const Core::ParameterFloat AlignmentMultiplyWeightsNode::paramFactor(
    "factor", "alignment weights are multiplied with this factor", 0.0);

AlignmentMultiplyWeightsNode::AlignmentMultiplyWeightsNode(
    const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c),
    factor_(paramFactor(config))
{}

bool AlignmentMultiplyWeightsNode::setParameter(const std::string &name, const std::string &value) {
    if (paramFactor.match(name)) {
	factor_ = paramFactor(value);
	return true;
    }
    return Precursor::setParameter(name, value);
}

void AlignmentMultiplyWeightsNode::transform(Alignment &a) {
    a.multiplyWeights(factor_);
}

/** AlignmentAddWeightNode
 */
const Core::ParameterFloat AlignmentAddWeightNode::paramOffset(
    "offset", "offset is added to alignment weights", 0.0);

AlignmentAddWeightNode::AlignmentAddWeightNode(
    const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c),
    offset_(paramOffset(config))
{}

bool AlignmentAddWeightNode::setParameter(const std::string &name, const std::string &value) {
    if (paramOffset.match(name)) {
	offset_ = paramOffset(value);
	return true;
    }
    return Precursor::setParameter(name, value);
}

void AlignmentAddWeightNode::transform(Alignment &a) {
    a.addWeight(offset_);
}

/** AlignmentExpmNode
 */
AlignmentExpmNode::AlignmentExpmNode(
    const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c)
{}

void AlignmentExpmNode::transform(Alignment &a) {
    a.expm();
}



/** AlignmentRemoveEmissionScoreNode
 */
AlignmentRemoveEmissionScoreNode::AlignmentRemoveEmissionScoreNode(
    const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c),
    needInit_(true)
{
    addInputs(2);
}

void AlignmentRemoveEmissionScoreNode::checkFeatureDependencies(const Mm::Feature &feature) const
{
    Mm::FeatureDescription description(*this, feature);
    if (!acousticModel_->isCompatible(description)) {
	acousticModel_->respondToDelayedErrors();
    }
}

bool AlignmentRemoveEmissionScoreNode::configure()
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

    return Precursor::configure();
}

bool AlignmentRemoveEmissionScoreNode::work(Flow::PortId p)
{
    if (needInit_) {
	Flow::DataPtr<Flow::DataAdaptor<ModelCombinationRef> > in;
	getData(2, in);
	acousticModel_ = in->data()->acousticModel();
	needInit_ = false;
    }
    return Precursor::work(p);
}

void AlignmentRemoveEmissionScoreNode::transform(Alignment &a)
{
    TimeframeIndex currentFeatureId = 0;
    Alignment::iterator currentAlignmentItem = a.begin();
    Flow::DataPtr<Feature::FlowFeature> in;
    bool firstFeature = true;
    while (getData(1, in)) {
	Core::Ref<const Feature> f(new Feature(in));
	if (firstFeature) { // try to check the dimension only once for each segment
	    checkFeatureDependencies(*f);
	    firstFeature = false;
	}
	Mm::FeatureScorer::Scorer scorer(acousticModel_->featureScorer()->getScorer(f));
	while (currentAlignmentItem != a.end() && currentAlignmentItem->time == currentFeatureId) {
	    verify(currentAlignmentItem == a.begin() ? true :
		   (currentAlignmentItem - 1)->time <= currentAlignmentItem->time);
	    f32 emScore = scorer->score(acousticModel_->emissionIndex(currentAlignmentItem->emission));
	    currentAlignmentItem->weight -= emScore;
	    ++ currentAlignmentItem;
	}
	++ currentFeatureId;
    }
}

/** SetAlignmentWeightsByTiedStateAlignmentWeightsNode
 */

SetAlignmentWeightsByTiedStateAlignmentWeightsNode::SetAlignmentWeightsByTiedStateAlignmentWeightsNode(const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c)
{
    addInputs(2);
    addOutputs(1);

    ModelCombination modelCombination(
	select("model-combination"), ModelCombination::useAcousticModel,
	Am::AcousticModel::noEmissions | Am::AcousticModel::noStateTransition);
    modelCombination.load();
    acousticModel_ = modelCombination.acousticModel();
}

bool SetAlignmentWeightsByTiedStateAlignmentWeightsNode::configure()
{
    Core::Ref<Flow::Attributes> attributes(new Flow::Attributes());

    getInputAttributes(1, *attributes);

    if(!configureDatatype(attributes, Flow::DataAdaptor<Alignment>::type())) {
	    return false;
    }

    getInputAttributes(0, *attributes);

    if (!configureDatatype(attributes, Flow::DataAdaptor<Alignment>::type())) {
	return false;
    }
    return putOutputAttributes(0, attributes);
}

bool SetAlignmentWeightsByTiedStateAlignmentWeightsNode::work(Flow::PortId p)
{

    Flow::DataPtr<Flow::DataAdaptor<Alignment> > ina;
    Flow::DataPtr<Flow::DataAdaptor<Alignment> > inb;

    while (getData(0, ina) && getData(1, inb)) {
	const Alignment &aa = ina->data();
	const Alignment &bb = inb->data();
	Alignment a(aa);
	Alignment b(bb);

	// Sometimes the aligner may create new allophones in the allophone state alphabet.
	// This works around that bug by removing allophones which don't match our current allophone alphabet (which is maintained separately).
	a.setAlphabet(acousticModel_->allophoneStateAlphabet(), true);
	b.setAlphabet(acousticModel_->allophoneStateAlphabet(), true);

	std::vector<Alignment::Frame> framesa;
	std::vector<Alignment::Frame> framesb;
	a.combineItems(Fsa::ProbabilitySemiring);
	a.getFrames(framesa);
	b.getFrames(framesb);

	std::vector<Alignment::Frame>::const_iterator i;
	Alignment::iterator s;
	s = b.begin();
	for (i = framesa.begin(); i != framesa.end() && s != b.end(); ++i) {
	    Alignment::iterator j, j_end;

	    s->weight = 0.0;
	    for (Core::tie(j,j_end) = *i; j != j_end; ++j) {
		if (j->time != s->time) {
			warning("alignment times differ: single-path=%d multi-path=%d", s->time, j->time);
		}
		else {
		      if(acousticModel_->emissionIndex(s->emission) == acousticModel_->emissionIndex(j->emission))
				s->weight += j->weight;
		}
	    }
	    s++;
	}


	Flow::DataAdaptor<Alignment> *alignment = new Flow::DataAdaptor<Alignment>(b);
	putData(0, alignment);
    }

    return putData(0, ina.get());
}
