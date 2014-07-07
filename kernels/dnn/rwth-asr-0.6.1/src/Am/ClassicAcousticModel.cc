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
#include "ClassicAcousticModel.hh"
#include "ClassicTransducerBuilder.hh"
#include "Utilities.hh"
#include <Core/Choice.hh>
#include <Legacy/DecisionTree.hh>
#include <Mm/Module.hh>
#include "ClassicStateTying.hh"

using namespace Am;


ClassicAcousticModel::ClassicAcousticModel(const Core::Configuration &configuration,
					   Bliss::LexiconRef lexiconRef) :
    Core::Component(configuration),
    AcousticModel(configuration),
    lexiconRef_(lexiconRef)
{
    determineSilencePhoneme();
    loadStateModel();
}

ClassicAcousticModel::~ClassicAcousticModel() {}

void ClassicAcousticModel::determineSilencePhoneme()
{
    LexiconUtilities lexiconUtilities(config, lexiconRef_);
    silence_ = lexiconUtilities.determineSilencePhoneme();
    lexiconUtilities.respondToDelayedErrors();
}

void ClassicAcousticModel::determineSilenceAllophoneStateIndex()
{
    LexiconUtilities lexiconUtilities(config, lexiconRef_);
    const Bliss::Pronunciation *silencePronunciation =
	lexiconUtilities.determineSilencePronunciation()->pronunciation();
    if (silencePronunciation) {
	Allophone allo(stateModelRef_->phonology()(*silencePronunciation, 0),
		       Allophone::isInitialPhone | Allophone::isFinalPhone);
	silenceAllophoneStateIndex_ = allophoneStateAlphabet()->index(&allo, 0);
    } else {
	error("Could not determine silence allophone state index.");
	silenceAllophoneStateIndex_ = Fsa::InvalidLabelId;
    }
}

void ClassicAcousticModel::load(Mode mode)
{
    if ((mode & noStateTying) == 0) {
	if (!loadStateTying())
	    return;
    }
    if ((mode & noStateTransition) == 0) {
	if (!loadTransitionModel())
	    return;
    }
    if ((mode & noEmissions) == 0) {
	if (!loadMixtureSet())
	    return;
    }
}


bool ClassicAcousticModel::loadStateModel()
{
    HmmTopologySet *hmmTopologySet =
	new HmmTopologySet(
	    select("hmm"),
	    silence_);
    HmmTopologySetRef hmmTopologySetRef(hmmTopologySet);
    Phonology *phonology =
	new Phonology(
	    select("phonology"),
	    lexiconRef_->phonemeInventory());
    phonology->setCrossWord(hmmTopologySetRef->isAcrossWordModelEnabled());
    ConstPhonologyRef phonologyRef(phonology);
    AllophoneAlphabet *allophoneAlphabet =
	new AllophoneAlphabet(
	    select("allophones"),
	    phonologyRef,
	    lexiconRef_);
    ConstAllophoneAlphabetRef allophoneAlphabetRef(allophoneAlphabet);
    AllophoneStateAlphabet *allophoneStateAlphabet =
	new AllophoneStateAlphabet(
	    select("allophone-states"),
	    allophoneAlphabetRef,
	    hmmTopologySetRef);
    ConstAllophoneStateAlphabetRef allophoneStateAlphabetRef(allophoneStateAlphabet);
    stateModelRef_ = ClassicStateModelRef(
	new ClassicStateModel(
	    phonologyRef,
	    allophoneAlphabetRef,
	    allophoneStateAlphabetRef,
	    hmmTopologySetRef));
    determineSilenceAllophoneStateIndex();
    return true;
}

const Core::Choice ClassicAcousticModel::choiceStateTying(
    "none",          noTying,
    "no-tying",      noTying,
    "monophone",     monophoneTying,
    "lut",           lutTying,
    "lookup",        lutTying,
    "cart",          cartTying,
    "decision-tree", cartTying,
    "old-cart",          oldCartTying,
    "old-decision-tree", oldCartTying,
    Core::Choice::endMark());
const Core::ParameterChoice ClassicAcousticModel::paramType(
    "type",
    &choiceStateTying,
    "method used to tie states",
    // for compatibility reasons:
    //    noTying);
    oldCartTying);

bool ClassicAcousticModel::loadStateTying()
{
    switch (paramType(select("state-tying"))) {
    case oldCartTying:
	//for compatibility reasons
	stateTyingRef_ = Core::Ref<const ClassicStateTying>(new Legacy::PhoneticDecisionTree(select("decision-tree"), stateModelRef_));
	break;
    default:
	stateTyingRef_ = ClassicStateTying::createClassicStateTyingRef(select("state-tying"), stateModelRef_);
	break;
    }
    if (stateTyingRef_->hasFatalErrors()) {
	error("failed to initialize state tying.");
	stateTyingRef_.reset();
	return false;
    }
    return true;
}

bool ClassicAcousticModel::loadTransitionModel()
{
    transitionModel_ = Core::ref(new ScaledTransitionModel(select("tdp"), stateModel()));
    if (transitionModel_->load()) {
	transitionModel_->setParentScale(scale());
	return true;
    }
    return false;
}

bool ClassicAcousticModel::openMixtureSet()
{
    mixtureSet_ = Mm::Module::instance().readAbstractMixtureSet(select("mixture-set"));
    if (!mixtureSet_) {
	 error("Could not load mixture set.");
	 return false;
    }
    return true;
}

bool ClassicAcousticModel::loadMixtureSet()
{
    verify(stateTyingRef_);
    openMixtureSet();
    verify(mixtureSet_);

    Core::Ref<Mm::ScaledFeatureScorer> featureScorer =
	    Mm::Module::instance().createScaledFeatureScorer(select("mixture-set"), mixtureSet_);
    if (!featureScorer) {
	error("Could not create feature scorer.");
	return false;
    }
    return setFeatureScorer(featureScorer);
}

Core::Ref<Mm::AbstractMixtureSet> ClassicAcousticModel::mixtureSet()
{
	if(!mixtureSet_)
		openMixtureSet();
	return mixtureSet_;
}

void ClassicAcousticModel::getDependencies(Core::DependencySet &dependencies) const
{
    Core::DependencySet d;
    stateModelRef_->hmmTopologySet().getDependencies(d);
    if (stateTyingRef_)
	stateTyingRef_->getDependencies(d);
    if (featureScorer_)
	featureScorer_->getDependencies(d);
    if (transitionModel_)
	transitionModel_->getDependencies(d);

    dependencies.add(name(), d);
    Mc::Component::getDependencies(dependencies);
}


Core::Ref<TransducerBuilder>
ClassicAcousticModel::createTransducerBuilder() const {
    return Core::ref(new ClassicTransducerBuilder(Core::ref(this)));
}


Core::Ref<const Mm::ScaledFeatureScorer> ClassicAcousticModel::featureScorer()
{
    if (!featureScorer_)
	criticalError("Acoustic-model does not have a feature-scorer.");
    return featureScorer_;
}

Core::Ref<Mm::ScaledFeatureScorer> ClassicAcousticModel::mutableFeatureScorer()
{
    if (!featureScorer_)
	criticalError("Acoustic-model does not have a feature-scorer.");
    return featureScorer_;
}


bool ClassicAcousticModel::setFeatureScorer(Core::Ref<Mm::ScaledFeatureScorer> featureScorer)
{
    require(featureScorer);
    verify(stateTyingRef_);

    if (stateTyingRef_->nClasses() != featureScorer->nMixtures()) {
	error("State-tying (nClasses=%d) and mixture-set (nMixtures=%d) do not match.",
	      stateTyingRef_->nClasses(), featureScorer->nMixtures());
	return false;
    }
    featureScorer_ = featureScorer;
    featureScorer_->setParentScale(scale());
    return true;
}

AcousticModel::EmissionIndex ClassicAcousticModel::nEmissions() const
{
    verify((featureScorer_ != 0) || (stateTyingRef_) );
    return featureScorer_ ? featureScorer_->nMixtures() : stateTyingRef_->nClasses();
}

void ClassicAcousticModel::distributeScaleUpdate(const Mc::ScaleUpdate &scaleUpdate)
{
    if (featureScorer_)
	featureScorer_->updateScales(scaleUpdate);
    if (transitionModel_)
	transitionModel_->updateScales(scaleUpdate);
}
