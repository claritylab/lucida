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
#include "ClassicTransducerBuilder.hh"
#include "Utilities.hh"
#include <Core/ProgressIndicator.hh>
#include <Fsa/Basic.hh>
#include <Fsa/Output.hh>
#include <Fsa/Storage.hh>
#include <Fsa/RemoveEpsilons.hh>
#include <Fsa/Hash.hh>

using namespace Am;

// ===========================================================================
class ClassicTransducerBuilder::Statistics
{
private:
    struct PhoneBoundaryStateSetDescriptor
    {
	u8 phoneBoundaryFlag;
	bool isCoarticulated;
	PhoneBoundaryStateSetDescriptor(
		const PhoneBoundaryStateDescriptor &pbsd) :
	    phoneBoundaryFlag(pbsd.flag), isCoarticulated(
		    pbsd.isCoarticulated())
	{
	}

	Core::XmlOpen xmlAttributes(const Core::XmlOpen&) const;

	bool operator==(const PhoneBoundaryStateSetDescriptor &r) const
	{
	    return (phoneBoundaryFlag == r.phoneBoundaryFlag)
		    && (isCoarticulated == r.isCoarticulated);
	}
	struct Hash
	{
	    size_t operator()(const PhoneBoundaryStateSetDescriptor &pbssd) const
	    {
		return size_t(pbssd.phoneBoundaryFlag)
			<< (1 + size_t(pbssd.isCoarticulated));
	    }
	};
    };
    typedef Core::hash_map<PhoneBoundaryStateSetDescriptor, u32,
	    PhoneBoundaryStateSetDescriptor::Hash> NumberOfPhoneBoundaryStates;
private:
    u32 &nPhoneBoundaryStates(const PhoneBoundaryStateDescriptor &pbsd)
    {
	return nPhoneBoundaryStates_.insert(std::make_pair(
		PhoneBoundaryStateSetDescriptor(pbsd), 0)).first->second;
    }
    void writeNumberOfPhoneBoundaryStates(Core::XmlWriter&) const;
    void writeNumberOfAllophones(Core::XmlWriter&) const;
private:
    NumberOfPhoneBoundaryStates nPhoneBoundaryStates_;
    size_t nAllophones_;
public:
    Statistics() :
	nAllophones_(0)
    {
    }

    void accumulate(const PhoneBoundaryStateDescriptor &pbsd)
    {
	nPhoneBoundaryStates(pbsd) += 1;
    }
    void setNumberOfAllophones(size_t n)
    {
	nAllophones_ = n;
    }

    void reset()
    {
	nPhoneBoundaryStates_.clear();
    }
    void write(Core::XmlWriter &) const;
};

Core::XmlOpen ClassicTransducerBuilder::Statistics::PhoneBoundaryStateSetDescriptor::xmlAttributes(
	const Core::XmlOpen &xmlOpen) const
{
    std::string phoneBoundaryValue;
    if (phoneBoundaryFlag == intraWord)
	phoneBoundaryValue += "intra-word|";
    if (phoneBoundaryFlag & wordStart)
	phoneBoundaryValue += "word-start|";
    if (phoneBoundaryFlag & wordEnd)
	phoneBoundaryValue += "word-end|";
    if (!phoneBoundaryValue.empty())
	phoneBoundaryValue.erase(phoneBoundaryValue.end() - 1);

    std::string isCoarticulatedValue = isCoarticulated ? "true" : "false";

    return Core::XmlOpen(xmlOpen) + Core::XmlAttribute("boundary",
	    phoneBoundaryValue) + Core::XmlAttribute("coarticulated",
	    isCoarticulatedValue);
}

void ClassicTransducerBuilder::Statistics::write(Core::XmlWriter &os) const
{
    os << Core::XmlOpen("statistics") + Core::XmlAttribute("type",
	    "state-model-transducer");
    writeNumberOfAllophones(os);
    writeNumberOfPhoneBoundaryStates(os);
    os << Core::XmlClose("statistics");
}

void ClassicTransducerBuilder::Statistics::writeNumberOfAllophones(
	Core::XmlWriter &os) const
{
    os << Core::XmlFull("number-of-distinct-allophones", nAllophones_);
}

void ClassicTransducerBuilder::Statistics::writeNumberOfPhoneBoundaryStates(
	Core::XmlWriter &os) const
{
    for (NumberOfPhoneBoundaryStates::const_iterator i =
	    nPhoneBoundaryStates_.begin(); i != nPhoneBoundaryStates_.end(); ++i) {
	os << i->first.xmlAttributes(Core::XmlOpen("number-of-states"))
		<< i->second << Core::XmlClose("number-of-states");
    }
}

// ---------------------------------------------------------------------------
ClassicTransducerBuilder::ClassicTransducerBuilder(Core::Ref<
	const ClassicAcousticModel> model) :
    TransducerBuilder(), model_(model), silencesAndNoises_(0),
	    allophoneSuffixes_(2500, AllophoneSuffix::Hash(this),
		    AllophoneSuffix::Equality(this))
{
    allophones_ = model_->allophoneAlphabet();
    allophoneList_ = &model_->allophoneAlphabet()->allophones();
    allophoneStates_ = model_->allophoneStateAlphabet();
    phonemes_ = model_->lexicon()->phonemeInventory()->phonemeAlphabet();

    nDisambiguators_ = 0;
    shouldApplyTransitionModel_ = true;
    acceptCoarticulatedSinglePronunciation_ = false;
    inputType_ = inputAcceptsEmissionLabels;
    statistics_ = new Statistics;
}

ClassicTransducerBuilder::~ClassicTransducerBuilder()
{
    delete statistics_;
}

// ---------------------------------------------------------------------------
void ClassicTransducerBuilder::setDisambiguators(u32 nDisambiguators)
{
    nDisambiguators_ = nDisambiguators;
}

void ClassicTransducerBuilder::selectAllophonesFromLexicon()
{
    AllophoneAlphabet *aAlphabet =
	    const_cast<AllophoneAlphabet *> (model_->allophoneAlphabet().get());
    //    aAlphabet->clear();
    aAlphabet->add(model_->lexicon());
    model_->log("%zd distinct allophones found", allophoneList_->size());
}

void ClassicTransducerBuilder::selectAllAllophones()
{
    AllophoneAlphabet *aAlphabet =
	    const_cast<AllophoneAlphabet *> (model_->allophoneAlphabet().get());
    //    aAlphabet->clear();
    aAlphabet->add(model_->lexicon()->phonemeInventory());
    model_->log("%zd distinct allophones possible", allophoneList_->size());
}

void ClassicTransducerBuilder::selectNonCoarticulatedSentenceBoundaries()
{
    acceptCoarticulatedSinglePronunciation_ = false;
}

void ClassicTransducerBuilder::selectCoarticulatedSinglePronunciation()
{
    acceptCoarticulatedSinglePronunciation_ = true;
}

void ClassicTransducerBuilder::selectFlatModel()
{
    shouldApplyTransitionModel_ = false;
}

void ClassicTransducerBuilder::selectTransitionModel()
{
    shouldApplyTransitionModel_ = true;
}

void ClassicTransducerBuilder::setSilencesAndNoises(
	const PronunciationList *pronunciationList)
{
    silencesAndNoises_ = pronunciationList;
}

void ClassicTransducerBuilder::selectAllophonesAsInput()
{
    inputType_ = inputAcceptsAllophones;
}

void ClassicTransducerBuilder::selectAllophoneStatesAsInput()
{
    inputType_ = inputAcceptsAllophoneStates;
}

void ClassicTransducerBuilder::selectEmissionLabelsAsInput()
{
    require(model_->stateTyingRef_);
    inputType_ = inputAcceptsEmissionLabels;
    if (!emissions_)
	emissions_ = model_->emissionAlphabet();
}

// ---------------------------------------------------------------------------
Fsa::State* ClassicTransducerBuilder::phoneBoundaryState(
	const PhoneBoundaryStateDescriptor &pbsd)
{
    PhoneBoundaryStates::iterator i = phoneBoundaryStates_.find(pbsd);
    if (i == phoneBoundaryStates_.end()) {
	Fsa::State *s = product_->newState();
	if (pbsd.isWordStart() && pbsd.context.history.empty()
		&& pbsd.context.future.empty())
	    setupWordStart(s);
	i = phoneBoundaryStates_.insert(std::make_pair(pbsd, s)).first;
	statistics_->accumulate(pbsd);
	if (pbsd.isWordEnd())
	    setupWordEnd(s, pbsd);
    }
    return i->second;
}

void ClassicTransducerBuilder::setupWordStart(Fsa::State *s)
{
    product_->setInitialStateId(s->id());
    if (!acceptCoarticulatedSinglePronunciation_) {
	s->addTags(Fsa::StateTagFinal);
	s->weight_ = product_->semiring()->one();
    }
}

void ClassicTransducerBuilder::setupWordEnd(Fsa::State *s,
	const PhoneBoundaryStateDescriptor &pbsd)
{
    if (acceptCoarticulatedSinglePronunciation_) {
	s->addTags(Fsa::StateTagFinal);
	s->weight_ = product_->semiring()->one();
    } else {
	PhoneBoundaryStateDescriptor pbsd2(pbsd);
	pbsd2.flag &= ~wordEnd;
	pbsd2.flag |= wordStart;
	buildWordBoundaryLinks(s, phoneBoundaryState(pbsd2));
    }
}

Fsa::State* ClassicTransducerBuilder::phoneStartState(
	const Allophone *allophone)
{
    PhoneBoundaryStateDescriptor pbsd;
    pbsd.context = allophone->context();
    model_->phonology()->pushFuture(pbsd.context, allophone->phoneme());
    pbsd.flag = 0;
    if (allophone->boundary & Allophone::isInitialPhone) {
	pbsd.flag |= wordStart;
	if (!pbsd.context.history.empty()
		&& acceptCoarticulatedSinglePronunciation_)
	    pbsd.context.history.clear();
    }
    /**
     *  Note: non-coarticulated phonemes mostly occur as initial/final phoneme (e.i. word start/end)
     *  but there exit seldom non-coarticulated within word phonemes  as well (e.g. more
     *  than one subsequent noise phonemes).
     */
    if (pbsd.context.history.empty())
	pbsd.context.future.clear();
    return phoneBoundaryState(pbsd);
}

Fsa::State* ClassicTransducerBuilder::phoneEndState(const Allophone *allophone)
{
    PhoneBoundaryStateDescriptor pbsd;
    pbsd.context = allophone->context();
    model_->phonology()->pushHistory(pbsd.context, allophone->phoneme());
    pbsd.flag = 0;
    if (allophone->boundary & Allophone::isFinalPhone) {
	pbsd.flag |= wordEnd;
	if (!pbsd.context.future.empty()
		&& acceptCoarticulatedSinglePronunciation_)
	    pbsd.context.future.clear();
    }
    /**
     *  Note: non-coarticulated phonemes mostly occur as initial/final phoneme (e.i. word start/end)
     *  but there exit seldom non-coarticulated within word phonemes  as well (e.g. more
     *  than one subsequent noise phonemes).
     */
    if (pbsd.context.future.empty())
	pbsd.context.history.clear();
    return phoneBoundaryState(pbsd);
}

void ClassicTransducerBuilder::buildAllophone(const Allophone *allophone)
{
    // nDisambiguators_ effects the application of the transition model.
    require(!acceptCoarticulatedSinglePronunciation_ || nDisambiguators_ == 0);
    buildAllophone(allophone, phoneStartState(allophone), phoneEndState(
	    allophone));
}

Fsa::State *ClassicTransducerBuilder::buildAllophone(
	const Allophone *allophone, Fsa::State *start)
{
    Fsa::State *end = product_->newState();
    buildAllophone(allophone, start, end);
    return end;
}

void ClassicTransducerBuilder::buildAllophone(const Allophone *allophone,
	Fsa::State *start, Fsa::State *end)
{
    verify(allophone);
    switch (inputType_) {
    case inputAcceptsAllophones: {
	const Bliss::Phoneme *phoneme = model_->phonemeInventory()->phoneme(
		allophone->phoneme());
	verify(phoneme);
	verify(product_->inputAlphabet() == allophones_);
	start->newArc(end->id(), product_->semiring()->one(),
		allophones_->index(allophone), phoneme->id());
    }
	break;
    case inputAcceptsAllophoneStates:
    case inputAcceptsEmissionLabels: {
	buildAllophoneStates(allophone, start, end);
    }
	break;
    default:
	defect();
    }
}

Fsa::LabelId ClassicTransducerBuilder::allophoneStateLabel(
	const AllophoneState &as)
{
    switch (inputType_) {
    case inputAcceptsAllophoneStates:
	return allophoneStates_->index(as);
    case inputAcceptsEmissionLabels:
	return model_->stateTyingRef_->classify(as);
    default:
	defect();
    }
    return Fsa::InvalidLabelId;
}

size_t ClassicTransducerBuilder::hashSequence(const AllophoneSuffix &as)
{
    size_t result = as.target;
    for (s16 state = as.allophoneState.state(); state
	    < as.hmmTopology->nPhoneStates(); ++state) {
	result = (result << 11) ^ (result >> 21);
	result ^= size_t(allophoneStateLabel(allophoneStates_->allophoneState(
		as.allophoneState.allophone(), state)));
    }
    result += as.hmmTopology->nSubStates();
    result += as.subState;
    return result;
}

int ClassicTransducerBuilder::compareSequences(const AllophoneSuffix &ll,
	const AllophoneSuffix &rr)
{
    if (ll.target != rr.target)
	return (ll.target < rr.target) ? -1 : 1;
    if (ll.subState != rr.subState)
	return (ll.subState < rr.subState) ? -1 : 1;
    if (ll.hmmTopology->nSubStates() != rr.hmmTopology->nSubStates())
	return (ll.hmmTopology->nSubStates() < rr.hmmTopology->nSubStates()) ? -1
		: 1;
    s16 lstate = ll.allophoneState.state();
    s16 rstate = rr.allophoneState.state();
    for (;;) {
	if (lstate == ll.hmmTopology->nPhoneStates())
	    return (rstate == rr.hmmTopology->nPhoneStates()) ? 0 : 1;
	if (rstate == rr.hmmTopology->nPhoneStates())
	    return -1;
	Fsa::LabelId llLabel = allophoneStateLabel(
		allophoneStates_->allophoneState(ll.allophoneState.allophone(),
			lstate));
	Fsa::LabelId rrLabel = allophoneStateLabel(
		allophoneStates_->allophoneState(rr.allophoneState.allophone(),
			rstate));
	if (llLabel != rrLabel)
	    return (llLabel < rrLabel) ? -1 : 1;
	++lstate;
	++rstate;
    }
    return 0;
}

void ClassicTransducerBuilder::buildAllophoneStates(const Allophone *allophone,
	Fsa::State *start, Fsa::State *end)
{
    verify((product_->outputAlphabet() == phonemes_)
	    || (product_->outputAlphabet() == allophones_));
    verify((product_->inputAlphabet() == allophoneStates_)
	    || (product_->inputAlphabet() == emissions_));

    const Bliss::Phoneme *phoneme = model_->phonemeInventory()->phoneme(
	    allophone->phoneme());
    verify(phoneme);

    AllophoneSuffix as;
    as.hmmTopology = model_->hmmTopology(phoneme->id());
    as.allophoneState = allophoneStates_->allophoneState(allophone, 0);
    as.subState = 0;
    as.target = end->id();

    Fsa::LabelId output = Fsa::Epsilon;
    if (product_->outputAlphabet() == phonemes_)
	output = phoneme->id();
    else if (product_->outputAlphabet() == allophones_)
	output = allophones_->index(allophone);

    Fsa::State *from = start;
    while (from) {
	Fsa::Arc *aa = from->newArc();
	aa->weight_ = product_->semiring()->one();
	aa->input_ = allophoneStateLabel(as.allophoneState);
	aa->output_ = output;
	output = Fsa::Epsilon;
	if (++as.subState >= as.hmmTopology->nSubStates()) {
	    as.subState = 0;
	    as.allophoneState = allophoneStates_->allophoneState(
		    as.allophoneState.allophone(), as.allophoneState.state()
			    + 1);
	}
	if (as.allophoneState.state() >= as.hmmTopology->nPhoneStates()) {
	    aa->target_ = end->id();
	    break;
	} else {
	    AllophoneSuffixMap::const_iterator ti = allophoneSuffixes_.find(as);
	    if (ti != allophoneSuffixes_.end()) {
		aa->target_ = ti->second;
		break;
	    }
	}
	aa->target_ = (from = product_->newState())->id();
	allophoneSuffixes_.insert(std::make_pair(as, aa->target_));
    }
}

void ClassicTransducerBuilder::buildWordBoundaryLinks(Fsa::State* from,
	Fsa::State* to)
{
    verify((product_->outputAlphabet() == phonemes_)
	    || (product_->outputAlphabet() == allophones_));
    if (nDisambiguators_) {
	for (u32 d = 0; d < nDisambiguators_; ++d) {
	    Fsa::Arc *a = from->newArc();
	    a->target_ = to->id();
	    a->weight_ = product_->semiring()->one();
	    switch (inputType_) {
	    case inputAcceptsAllophones:
		verify(product_->inputAlphabet() == allophones_);
		a->input_ = allophones_->disambiguator(d);
		break;
	    case inputAcceptsAllophoneStates:
		verify(product_->inputAlphabet() == allophoneStates_);
		a->input_ = allophoneStates_->disambiguator(d);
		break;
	    case inputAcceptsEmissionLabels:
		verify(product_->inputAlphabet() == emissions_);
		a->input_ = emissions_->disambiguator(d);
		break;
	    default:
		defect();
	    }
	    if (product_->outputAlphabet() == phonemes_)
		a->output_ = phonemes_->disambiguator(d);
	    else if (product_->outputAlphabet() == allophones_)
		a->output_ = allophones_->disambiguator(d);
	}
    } else {
	from->newArc(to->id(), product_->semiring()->one(), Fsa::Epsilon,
		Fsa::Epsilon);
    }
}

void ClassicTransducerBuilder::createEmptyTransducer()
{
    product_ = Core::ref(new Fsa::StaticAutomaton);
    product_->setType(Fsa::TypeTransducer);
    product_->setSemiring(Fsa::TropicalSemiring);
    switch (inputType_) {
    case inputAcceptsAllophones:
	product_->setInputAlphabet(allophones_);
	break;
    case inputAcceptsAllophoneStates:
	product_->setInputAlphabet(allophoneStates_);
	break;
    case inputAcceptsEmissionLabels:
	product_->setInputAlphabet(emissions_);
	break;
    default:
	defect();
    }
    product_->setOutputAlphabet(phonemes_);
    verify(phoneBoundaryStates_.size() == 0);
    verify(allophoneSuffixes_.size() == 0);
}

void ClassicTransducerBuilder::buildPhoneLoop()
{
    createEmptyTransducer();
    Core::Channel ach(model_->getConfiguration(), "allophones");
    Core::ProgressIndicator pi("building state model transducer", "phones");
    pi.start(allophoneList_->size());
    for (AllophoneAlphabet::AllophoneList::const_iterator ai =
	    allophoneList_->begin(); ai != allophoneList_->end(); ++ai) {
	buildAllophone(*ai);
	ach << allophones_->toString(**ai) << "\n";
	pi.notify();
    }
    pi.finish();

    statistics_->setNumberOfAllophones(allophoneList_->size());
    statistics_->write(model_->log());
    statistics_->reset();

    phoneBoundaryStates_.clear();
    allophoneSuffixes_.clear();
}

Fsa::State* ClassicTransducerBuilder::buildSilenceAndNoiseLoops(
	const Bliss::PhonemeInventory &phonemeInventory,
	const Phonology &phonology, Fsa::State *s)
{
    allophoneStates_->disambiguator(0);
    setDisambiguators(allophoneStates_->nDisambiguators());

    Fsa::State *finalState = product_->newState();
    buildWordBoundaryLinks(s, finalState);

    PronunciationList::const_iterator silencePronunciationIt =
	    silencesAndNoises_->begin();
    for (; silencePronunciationIt != silencesAndNoises_->end(); ++silencePronunciationIt) {
	if ((*silencePronunciationIt)->length() == 1) {
	    Allophone allophone(phonology(**silencePronunciationIt, 0),
		    Allophone::isFinalPhone | Allophone::isInitialPhone);
	    verify(
		    !phonemeInventory.phoneme(allophone.phoneme())->isContextDependent());
	    buildAllophone(allophones_->allophone(allophone), finalState, s);
	} else {
	    require((*silencePronunciationIt)->length() == 0);
	}
    }
    return finalState;
}

void ClassicTransducerBuilder::buildPronunciation(const Bliss::Coarticulated<
	Bliss::Pronunciation> &coarticulatedPronunciation)
{
    //    require(nDisambiguators_ == 0); // nDisambiguators_ effects the application of the transition model.
    setDisambiguators(0);

    const Bliss::Pronunciation &p(coarticulatedPronunciation.object());
    Bliss::Phoneme::Id leftContext = coarticulatedPronunciation.leftContext();
    Bliss::Phoneme::Id rightContext = coarticulatedPronunciation.rightContext();

    const Phonology &phonology = *(model_->phonology().get());
    const Bliss::PhonemeInventory &phonemeInventory =
	    *(model_->phonemeInventory().get());

    createEmptyTransducer();
    Fsa::State *s = product_->newState();
    product_->setInitialStateId(s->id());

    if (p.length() == 1) {
	Allophone allophone(phonology(p, 0), Allophone::isInitialPhone
		| Allophone::isFinalPhone);
	if (model_->isAcrossWordModelEnabled() && phonemeInventory.phoneme(
		allophone.phoneme())->isContextDependent()) {
	    if (leftContext != Bliss::Phoneme::term)
		phonology.appendHistory(allophone, leftContext);
	    if (rightContext != Bliss::Phoneme::term)
		phonology.appendFuture(allophone, rightContext);
	}
	s = buildAllophone(allophones_->allophone(allophone), s);
    } else if (p.length() > 1) {
	{
	    Allophone initialAllophone(phonology(p, 0),
		    Allophone::isInitialPhone);
	    if (leftContext != Bliss::Phoneme::term
		    && model_->isAcrossWordModelEnabled()
		    && phonemeInventory.phoneme(initialAllophone.phoneme())->isContextDependent()) {
		phonology.appendHistory(initialAllophone, leftContext);
	    }
	    s = buildAllophone(allophones_->allophone(initialAllophone), s);
	}

	for (u32 i = 1; i < p.length() - 1; ++i) {
	    Allophone allophone(phonology(p, i), intraWord);
	    s = buildAllophone(allophones_->allophone(allophone), s);
	}

	{
	    Allophone finalAllophone(phonology(p, p.length() - 1),
		    Allophone::isFinalPhone);
	    if (rightContext != Bliss::Phoneme::term
		    && model_->isAcrossWordModelEnabled()
		    && phonemeInventory.phoneme(finalAllophone.phoneme())->isContextDependent()) {
		phonology.appendFuture(finalAllophone, rightContext);
	    }
	    s = buildAllophone(allophones_->allophone(finalAllophone), s);
	}
    }
    if (silencesAndNoises_ && rightContext == Bliss::Phoneme::term) {
	s = buildSilenceAndNoiseLoops(phonemeInventory, phonology, s);
    }
    s->addTags(Fsa::StateTagFinal);
    s->weight_ = product_->semiring()->one();
}

Fsa::ConstAutomatonRef ClassicTransducerBuilder::applyTransitionModel(
	Fsa::ConstAutomatonRef ff)
{
    require(model_->transitionModel_);
    require((ff->inputAlphabet() == allophoneStates_) || (ff->inputAlphabet()
	    == emissions_));

    const Allophone *silenceAllophone = allophones_->allophone(Allophone(
	    Phonology::Allophone(model_->silence_), Allophone::isInitialPhone
		    | Allophone::isFinalPhone));
    AllophoneState silenceState = allophoneStates_-> allophoneState(
	    silenceAllophone, 0);

    Fsa::LabelId silenceLabel = Fsa::InvalidLabelId;
    if (ff->inputAlphabet() == allophoneStates_)
	silenceLabel = allophoneStates_->index(silenceState);
    else if (ff->inputAlphabet() == emissions_)
	silenceLabel = model_->stateTyingRef_->classify(silenceState);

    return model_->transitionModel_->apply(ff, silenceLabel, (nDisambiguators_
	    == 0));
}

void ClassicTransducerBuilder::applyStateTying()
{
    // This function not used at the moment.
    require(model_->stateTyingRef_);
    verify(product_->inputAlphabet() == allophoneStates_);
    verify(emissions_);
    Core::ProgressIndicator pi("applying state tying", "states");
    pi.start(product_->size());
    for (Fsa::StateId s = 0; s < product_->size(); ++s) {
	Fsa::StateRef sp = product_->state(s);
	if (sp) {
	    for (Fsa::State::iterator a = sp->begin(); a != sp->end(); ++a) {
		if (a->input() != Fsa::Epsilon) {
		    if (!allophoneStates_->isDisambiguator(a->input()))
			a->input_ = model_->stateTyingRef_->classifyIndex(
				a->input());
		    else
			a->input_ = emissions_->disambiguator(a->input()
				- allophoneStates_->nClasses() - 1);
		}
	    }
	}
	pi.notify(s);
    }
    pi.finish();
    product_->setInputAlphabet(emissions_);
}

void ClassicTransducerBuilder::finalize()
{
    /*
     * Use statistics only for debugging purposes.
     * Remove it for real applications because it
     * causes a significant memory leak.
     */


    if (shouldApplyTransitionModel_)
	product_ = Fsa::staticCopy(applyTransitionModel(product_));
    product_ = Fsa::staticCompactCopy(Fsa::trim(product_, true));
}

Fsa::ConstAutomatonRef ClassicTransducerBuilder::createPhonemeLoopTransducer()
{
    buildPhoneLoop();
    finalize();
    Fsa::ConstAutomatonRef result = product_;
    product_.reset();
    return result;
}

Fsa::ConstAutomatonRef ClassicTransducerBuilder::createPronunciationTransducer(
	const Bliss::Coarticulated<Bliss::Pronunciation> &p)
{
    buildPronunciation(p);
    finalize();
    Fsa::ConstAutomatonRef result = product_;
    product_.reset();
    return result;
}

Fsa::ConstAutomatonRef ClassicTransducerBuilder::createAllophoneLoopTransducer()
{
    createEmptyTransducer();
    require(inputType_ != inputAcceptsAllophones); // refuse meaningless request?
    product_->setOutputAlphabet(allophones_);

    Fsa::State *initial = product_->newState();
    product_->setInitialStateId(initial->id());
    product_->setStateFinal(initial);
    for (AllophoneAlphabet::AllophoneList::const_iterator ai =
	    allophoneList_->begin(); ai != allophoneList_->end(); ++ai)
	buildAllophone(*ai, initial, initial);
    allophoneSuffixes_.clear();
    buildWordBoundaryLinks(initial, initial);
    finalize();
    Fsa::ConstAutomatonRef result = product_;
    product_.reset();
    return result;
}

namespace
{
struct MinimizedState
{
    std::vector<Mm::MixtureIndex> mixtures_;
    MinimizedState(const std::vector<Mm::MixtureIndex> &mixtures) :
	mixtures_(mixtures)
    {
    }
    struct Equality
    {
	bool operator()(const MinimizedState &s1, const MinimizedState &s2) const
	{
	    return s1.mixtures_ == s2.mixtures_;
	}
    };
    struct Hash
    {
	size_t operator()(const MinimizedState &s) const
	{
	    size_t key = 0;
	    for (std::vector<Mm::MixtureIndex>::const_iterator m =
		    s.mixtures_.begin(); m != s.mixtures_.end(); ++m)
		key = (key << 16) ^ *m;
	    return key;
	}
    };
};
typedef Core::hash_map<MinimizedState, Fsa::State*, MinimizedState::Hash,
	MinimizedState::Equality> MinimizedStateMap;
}

/**
 *  - union of closure of reduced HMM state sequences for each (used, unique) allophone
 *  - add disambiguation symbols
 *  - still not determinized on state level! (would result in less efficient composition)
 *
 * state labels and their corresponding outgoing transition probabilities:
 *  entry-m1   the (hidden) entry state of word models
 *  silence	   the single, unique state of the silence model
 *  phone0	   the first state of a phone segment
 *  phone1	   the second state of a phone segment
 *
 * silence.exit = silence.forward - phone?.forward   but why?
 **/
Fsa::ConstAutomatonRef ClassicTransducerBuilder::createEmissionLoopTransducer(
	bool transitionModel)
{
    Fsa::StaticAutomaton *a = new Fsa::StaticAutomaton();
    a->setType(Fsa::TypeTransducer);
    a->setSemiring(Fsa::TropicalSemiring);
    Core::Ref<const AllophoneStateAlphabet> asa =
	    model_->allophoneStateAlphabet();
    EmissionAlphabet *ea = 0;
    if (transitionModel) {
	ea = new EmissionAlphabet(model_->nEmissions());
	a->setInputAlphabet(Fsa::ConstAlphabetRef(ea));
    } else
	a->setInputAlphabet(Fsa::ConstAlphabetRef(asa));
    Core::Ref<const AllophoneAlphabet> aa = model_->allophoneAlphabet();
    a->setOutputAlphabet(aa);
    Fsa::State *initial = a->newState();
    a->setInitialStateId(initial->id());
    initial->setFinal(a->semiring()->one());
    MinimizedStateMap minimizedStates;
    for (AllophoneAlphabet::AllophoneList::const_iterator ai =
	    allophoneList_->begin(); ai != allophoneList_->end(); ++ai) {
	Fsa::State *from = initial, *to = 0, *fromSkip = 0;
	const ClassicHmmTopology *hmmTopology = model_->hmmTopology((*ai)->central());
	require(hmmTopology != 0);
	int nPhoneStates = hmmTopology->nPhoneStates();
	int nSubStates = hmmTopology->nSubStates();
	int nStates = nPhoneStates * nSubStates;
	// std::cout << aa->symbol(aa->index(*ai)) << " " << nStates << std::endl;

	const StateTransitionModel *tdp = 0;
	if ((*ai)->central() == model_->silence_)
	    tdp = (*model_->transitionModel_)[TransitionModel::silence];
	else
	    tdp = (*model_->transitionModel_)[TransitionModel::phone0];

	f32 fw = a->semiring()->one(); // constant in case of no transition model
	for (int s = 0; s < nStates; ++s) {
	    AllophoneState as = asa->allophoneState(*ai, s / nSubStates);
	    bool isFirstArc = (s == 0);
	    bool isLastArc  = (s == nStates - 1);
	    Mm::MixtureIndex input;
	    if (transitionModel)
		input = model_->stateTyingRef_->classify(as);
	    else
		input = asa->index(as);

	    std::vector<Mm::MixtureIndex> hmm;
	    for (int i = s; i < nStates; ++i) {
		AllophoneState _as = asa->allophoneState(*ai, i / nSubStates);
		if (transitionModel)
		    hmm.push_back(model_->stateTyingRef_->classify(_as));
		else
		    hmm.push_back(asa->index(_as));
	    }
	    MinimizedState ms(hmm);
	    MinimizedStateMap::iterator j = minimizedStates.find(ms);


	    if (transitionModel) {
		// find forward state
		if (j == minimizedStates.end()) {
		    minimizedStates[ms] = to = a->newState();
		    // loop transition
		    to->newArc(to->id(), Fsa::Weight((*tdp)[StateTransitionModel::loop]), input, Fsa::Epsilon);
		    if (isLastArc)
			to->newArc(initial->id(), a->semiring()->one(),	Fsa::Epsilon, Fsa::Epsilon);
		} else {
		    to = j->second;
		    s = nStates;
		}
		if (isFirstArc) {
		    fw = (*(*model_->transitionModel_)[TransitionModel::entryM1])[StateTransitionModel::forward];
		    if ((*ai)->boundary & Allophone::isFinalPhone)
			fw += (*tdp)[StateTransitionModel::exit];
		} else {
		    fw = (*tdp)[StateTransitionModel::forward];
		}
		// skip transition
		if (((*tdp)[StateTransitionModel::skip] < Core::Type<StateTransitionModel::Score>::max) && (fromSkip)) {
		    fromSkip->newArc(to->id(), Fsa::Weight((*tdp)[StateTransitionModel::skip]), input, (fromSkip == initial) ? aa->index(*ai) : Fsa::Epsilon);
		}
	    } else {
		if (j == minimizedStates.end()) {
		    if (isLastArc)
			to = initial;
		    else
			minimizedStates[ms] = to = a->newState();
		} else {
		    to = j->second;
		    s = nStates;
		}
	    }
	    // forward
	    from->newArc(to->id(), Fsa::Weight(fw), input, isFirstArc ? aa->index(*ai) : Fsa::Epsilon);
	    fromSkip = from;
	    from = to;
	}
    }
    for (u32 d = 0; d < nDisambiguators_; ++d)
	if (transitionModel)
	    initial->newArc(initial->id(), a->semiring()->one(), ea->disambiguator(d), aa->disambiguator(d));
	else
	    initial->newArc(initial->id(), a->semiring()->one(), asa->disambiguator(d), aa->disambiguator(d));
    return Fsa::ConstAutomatonRef(a);
}

namespace
{

typedef std::vector<Bliss::Phoneme::Id> Phones;
struct PhoneContext: public Phones
{
    // across-word phones needs the following:
    bool boundary_;
    Fsa::LabelId disambiguator_;
    PhoneContext(size_t n, bool boundary = false, Fsa::LabelId disambiguator =
	    Fsa::Epsilon) :
	Phones(n, Bliss::Phoneme::term), boundary_(boundary), disambiguator_(
		disambiguator)
    {
    }
    PhoneContext(const Phones &phones, bool boundary = false,
	    Fsa::LabelId disambiguator = Fsa::Epsilon) :
	Phones(phones), boundary_(boundary), disambiguator_(disambiguator)
    {
    }
    PhoneContext(Phones::const_iterator pBegin, Phones::const_iterator pEnd,
	    bool boundary = false, Fsa::LabelId disambiguator = Fsa::Epsilon) :
	Phones(pBegin, pEnd), boundary_(boundary),
		disambiguator_(disambiguator)
    {
    }
};

struct PhoneContextHash
{
    size_t operator()(const PhoneContext &p) const
    {
	size_t result = 0;
	for (PhoneContext::const_iterator s = p.begin(); s != p.end(); ++s)
	    result = 101 * result + size_t(*s);
	result = 3 * result + (p.boundary_ ? 1 : 0);
	result = 101 * result + size_t(p.disambiguator_);
	return result;
    }
};

struct NoCoartBoundary
{
    Fsa::State *state_;
    Fsa::LabelId input_, phoneDisambiguator_;
    NoCoartBoundary(Fsa::State *state, Fsa::LabelId input,
	    Fsa::LabelId phoneDisambiguator) :
	state_(state), input_(input), phoneDisambiguator_(phoneDisambiguator)
    {
    }
};

}

Fsa::ConstAutomatonRef ClassicTransducerBuilder::createMinimizedContextDependencyTransducer(
	Fsa::LabelId initialPhoneAndSilenceOffset)
{
    // bool coartSilence = false; // not used yet, but enables the support for co-articulated silence

    // find max history and future context lengths
    // collect all non-coarticulated phones
    size_t maxHistory = 0, maxFuture = 0;
    Core::hash_map<Bliss::Phoneme::Id, const Allophone*> nonCoartPhones;
    std::vector<Bliss::Phoneme::Id> initialCoartPhones, initialNonCoartPhones;
    for (AllophoneAlphabet::AllophoneList::const_iterator ai =
	    allophoneList_->begin(); ai != allophoneList_->end(); ++ai) {
	const Allophone *a = *ai;
	maxHistory = std::max(maxHistory, a->history().size());
	maxFuture = std::max(maxFuture, a->future().size());
	const Bliss::Phoneme *phoneme = model_->phonemeInventory()->phoneme(
		a->phoneme());
	if (!phoneme->isContextDependent() && ((*ai)->boundary
		& (Allophone::isInitialPhone | Allophone::isFinalPhone)))
	    nonCoartPhones[phoneme->id()] = a;
	if (a->history().size() == 0) {
	    if (a->boundary & Allophone::isInitialPhone)
		initialNonCoartPhones.push_back(phoneme->id());
	    else
		initialCoartPhones.push_back(phoneme->id());
	}
    }
    std::sort(initialCoartPhones.begin(), initialCoartPhones.end());
    initialCoartPhones.erase(std::unique(initialCoartPhones.begin(),
	    initialCoartPhones.end()), initialCoartPhones.end());
    std::sort(initialNonCoartPhones.begin(), initialNonCoartPhones.end());
    initialNonCoartPhones.erase(std::unique(initialNonCoartPhones.begin(),
	    initialNonCoartPhones.end()), initialNonCoartPhones.end());

    const Allophone *silenceAllophone = allophones_->allophone(Allophone(
	    Phonology::Allophone(model_->silence_), 0));

    // ensure that code only runs with triphones
    require(maxHistory <= 1);
    require(maxFuture <= 1);

    Fsa::StaticAutomaton *_c = new Fsa::StaticAutomaton(Fsa::TypeTransducer);
    Fsa::ConstSemiringRef semiring = Fsa::TropicalSemiring;
    _c->setSemiring(semiring);
    _c->setInputAlphabet(allophones_);
    _c->setOutputAlphabet(phonemes_);

    Fsa::Hash<PhoneContext, PhoneContextHash> stateMap;
    PhoneContext pc(maxHistory + maxFuture, true);
    Fsa::State *initial = new Fsa::State(stateMap.insert(pc),
	    model_->isAcrossWordModelEnabled() ? Fsa::StateTagFinal
		    : Fsa::StateTagNone, semiring->one());
    _c->setState(initial);
    _c->setInitialStateId(initial->id());
    if (model_->isAcrossWordModelEnabled()) {
	// generic disambiguator loop
	for (size_t d = 0; d < nDisambiguators_; ++d)
	    initial->newArc(initial->id(), semiring->one(),
		    allophones_->disambiguator(d), phonemes_->disambiguator(d));

	// non-coart phones
	for (Core::hash_map<Bliss::Phoneme::Id, const Allophone *>::const_iterator
		i = nonCoartPhones.begin(); i != nonCoartPhones.end(); ++i)
	    initial->newArc(initial->id(), semiring->one(), allophones_->index(
		    i->second), initialPhoneAndSilenceOffset + Fsa::LabelId(
		    i->first));
    }

    // The construction of C for within- and across-word models
    // differs slightly. In both cases we shift the allophone input by
    // one phone/arc in order to make the FSA deterministic right
    // away. The main difference occurs at word boundaries.
    //
    // Using within-word models only, always uses the open context at
    // word boundaries. This would lead to a major source of
    // non-determinism. As disambiguation symbols only appear at word
    // boundaries, we output the phone disambiguators first and then
    // consume the appropriate allophone disambiguator at an arc of
    // the target state.
    //
    // Using across-word models, problems at the word boundaries are
    // more severe. For a particular state a-b we can have 3 possible
    // extensions: a-b-c, a-b|c and a-b|#. Using the same
    // disambiguator trick from above results in a-b-c:c, a-b|c:#1 and
    // a-b|#:#1, but we can't easily disambiguate the later two
    // cases. Instead, we can use loops at each state to pass through
    // disambiguators. However, then the 3 cases become a-b-c:c,
    // a-b|c:c and a-b|#:c. This looks even more critical, but a-b-c
    // and a-b|c are easy to disambiguate by using special phone
    // symbols for the first phone of a word. So, we already have
    // a-b-c:c and a-b|c:c'. For a-b|# we can utilize the
    // approximation of a minimum of 1 silence frame in case of no
    // coarticulation. Note that this approximation doesn't help when
    // using the disambiguator-in-sequence trick. Another trick is to
    // not use the coarticulated silence case.
    //
    // Construction:
    //
    // States are labeled either with a-b or a|b, so for triphones we
    // have to hash over 2 phones + 1 flag per state, indicating
    // whether there is a word boundary between the phones or not.
    // Then there is 1 state per non-coarticulated phone p, marked by
    // #-p. Additionally, for within-word models we have states that
    // carry over disambiguator symbols. The final state is the
    // (non-coarticulated) silence history #|si.
    //
    // Each allophone a*b*c generates exactly one arc from a*b to b*c
    // usually labeled with a*b*c:c. Special cases are the entry
    // structure and word boundaries as described above.

    // we create as many final states as there are disambiguators
    // passing disambiguators from phone to allophones is integrated
    // in the construction disambiguators on phone sequences are used
    // to detect word boundaries
    //
    // ASSUMES: phones still completely set to Bliss::Phoneme::term
    // and boundary set to true!!!
    std::vector<NoCoartBoundary> nonCoartBoundaries;

    // cycle and exit:
    // for each allophone a-b-c:
    // - if c == #, we draw an arc to the final state f and an additional silence arc to #,si
    // - otherwise, we draw an arc from a,b to b,c
    for (AllophoneAlphabet::AllophoneList::const_iterator ai =
	    allophoneList_->begin(); ai != allophoneList_->end(); ++ai) {
	// create phone sequence from allophone
	Phones phones(maxHistory + 1 + maxFuture, Bliss::Phoneme::term);
	std::copy((*ai)->history().begin(), (*ai)->history().end(),
		phones.begin() + maxHistory - (*ai)->history().size());
	phones[maxHistory] = (*ai)->central();
	std::copy((*ai)->future().begin(), (*ai)->future().end(),
		phones.begin() + maxHistory + 1);

	// create arcs (and states) for allophone
	if (model_->isAcrossWordModelEnabled()) {
	    // filter out:
	    // - unsupported silences (none of #-si-#, #|si|#)
	    // - non-coart phones
	    if ((*ai)->central() == silenceAllophone->central()) {
		if ((((*ai)->history().size() > 0)  && ((*ai)->future().size() == 0)) ||
		    (((*ai)->history().size() == 0) && ((*ai)->future().size() > 0)))
		    continue;
		if ((((*ai)->boundary & Allophone::isInitialPhone)  && !((*ai)->boundary & Allophone::isFinalPhone)) ||
		    (!((*ai)->boundary & Allophone::isInitialPhone) && ((*ai)->boundary & Allophone::isFinalPhone)))
		    continue;
	    }
	    if (((*ai)->boundary & Allophone::isInitialPhone)  && (nonCoartPhones.find((*ai)->central()) != nonCoartPhones.end()))
		continue;

	    // create "from" state
	    Fsa::StateId sFrom = stateMap.insert(PhoneContext(phones.begin(),  phones.end() - 1, (*ai)->boundary & Allophone::isInitialPhone));
	    if (!_c->state(sFrom)) {
		_c->setState(new Fsa::State(sFrom));

		// generic disambiguator loop
		for (size_t d = 0; d < nDisambiguators_; ++d)
		    _c->state(sFrom)->newArc(sFrom, semiring->one(),
			    allophones_->disambiguator(d),
			    phonemes_->disambiguator(d));

		// special treatment for coart (within-word) silence
		if ((!(*ai)->boundary & Allophone::isInitialPhone) && (*ai)->central() == silenceAllophone->central() && ((*ai)->history().size() == 0)) {
		    for (std::vector<Bliss::Phoneme::Id>::const_iterator p = initialCoartPhones.begin(); p != initialCoartPhones.end(); ++p) {
			PhoneContext pc(maxHistory + maxFuture, false);
			pc[maxHistory] = Fsa::LabelId(*p);
			_c->state(sFrom)->newArc(stateMap.insert(pc), semiring->one(), allophones_->index(silenceAllophone), Fsa::LabelId(*p));
		    }
		}
	    }
	    Fsa::State *sp = _c->state(sFrom).get();

	    // create "to" states and arcs
	    if ((*ai)->boundary & Allophone::isFinalPhone) {
		// Two cases here: right context is either term or
		// phone. In case of term we output non-coarticulated
		// phones, in case of phone we mark it as word-initial
		// phone. The third case mentioned above is handled
		// already by the outer else case.
		if (((*ai)->future().size() == 0) || ((*ai)->future()[0] == Bliss::Phoneme::term))
		    sp->newArc(initial->id(), semiring->one(),
			    allophones_->index(*ai), Fsa::Epsilon);
		else
		    sp->newArc(stateMap.insert(PhoneContext(phones.begin() + 1, phones.end(), true)),
			    semiring->one(),
			    allophones_->index(*ai),
			    initialPhoneAndSilenceOffset + phones[maxHistory+1]);
	    } else {
		// within-phrase word boundary
		if (((*ai)->future().size() == 0) || ((*ai)->future()[0] == Bliss::Phoneme::term)) {
		    // to state marked #-si
		    if ((*ai)->central() != silenceAllophone->central()) {
			PhoneContext pc(maxHistory + maxFuture, false);
			pc[maxHistory] = silenceAllophone->central();
			sp->newArc(stateMap.insert(pc), semiring->one(),
				allophones_->index(*ai),
				silenceAllophone->central());
		    } else
			sp->newArc(sFrom, semiring->one(), allophones_->index(
				*ai), silenceAllophone->central());
		} else
		    sp->newArc(stateMap.insert(PhoneContext(phones.begin() + 1,
			    phones.end())), semiring->one(),
			    allophones_->index(*ai), phones[maxHistory + 1]);
	    }
	} else { // within-word only
	    // create "from" state
	    Fsa::StateId sFrom = stateMap.insert(PhoneContext(phones.begin(),
		    phones.end() - 1, (*ai)->boundary
			    & Allophone::isInitialPhone));
	    if (!_c->state(sFrom))
		_c->setState(new Fsa::State(sFrom));
	    Fsa::State *sp = _c->state(sFrom).get();

	    // create "to" state and arcs
	    if ((*ai)->boundary & Allophone::isFinalPhone) {
		Fsa::StateId sTo;
		for (size_t d = 0; d < nDisambiguators_; ++d) {
		    require((*ai)->future()[0] == Bliss::Phoneme::term); // allow only for boundary phone
		    sTo = stateMap.insert(PhoneContext(pc, true, d + 1)); // use generic phone context of all term symbols
		    if (!_c->state(sTo)) {
			Fsa::State *state = new Fsa::State(sTo,
				Fsa::StateTagFinal, semiring->one());
			_c->setState(state);
			nonCoartBoundaries.push_back(NoCoartBoundary(state,
				allophones_->disambiguator(d),
				phonemes_->disambiguator(d)));
			/*std::cout << "disambiguator d=" << d << "  -> " << allophones_->disambiguator(d) << " "
				  << allophones_->isDisambiguator(allophones_->disambiguator(d)) << " symbol="
				  << allophones_->symbol(allophones_->disambiguator(d)) << std::endl; */
		    }
		    sp->newArc(sTo, semiring->one(), allophones_->index(*ai),
			    phonemes_->disambiguator(d));
	    /*
		    std::cout << "(A) from=" << sp->id() << " to=" <<  sTo
			      << " input=" << allophones_->index(*ai)
			      << " " << allophones_->symbol(allophones_->index(*ai))
			      << " output=" << phonemes_->disambiguator(d) << " "
			      << phonemes_->symbol(phonemes_->disambiguator(d)) << std::endl;
	    */
		}
	    } else
		sp->newArc(stateMap.insert(PhoneContext(phones.begin() + 1,
			phones.end())), semiring->one(),
			allophones_->index(*ai), phones[maxHistory + 1]);
		/*
	std::cout << "(B) from=" << sp->id() << " to=" <<  stateMap.insert(PhoneContext(phones.begin() + 1, phones.end()))
		      << " input=" << allophones_->index(*ai)
		      << " " << allophones_->symbol(allophones_->index(*ai))
		      << " output=" << phones[maxHistory + 1] << " "
		      << phonemes_->symbol(phones[maxHistory + 1]) << std::endl;
	*/

	}
    }

    // entry & non-coarticulated word boundaries:
    // for each phone p we draw:
    // 1. an arc from #|# to #|p
    // 2. an arc from each non-coarticulated word boundary state to #|p
    for (std::vector<Bliss::Phoneme::Id>::const_iterator p =
	    initialNonCoartPhones.begin(); p != initialNonCoartPhones.end(); ++p) {
	PhoneContext pc(maxHistory + maxFuture, true);
	pc[maxHistory] = Fsa::LabelId(*p);
	Fsa::StateId s = stateMap.insert(pc);
	if (_c->state(s)) {
	    if (model_->isAcrossWordModelEnabled()) {
		if (nonCoartPhones.find(*p) == nonCoartPhones.end()) {
		    initial->newArc(s, semiring->one(), Fsa::Epsilon,
			    initialPhoneAndSilenceOffset + Fsa::LabelId(*p));
		    /* std::cout << "(C) from=" << initial->id() << " to=" <<  s
			      << " input=*EPS*" << std::endl; */
		}
	    } else {
		initial->newArc(s, semiring->one(), Fsa::Epsilon, Fsa::LabelId(*p));

	/*std::cout << "(D) from=" << initial->id() << " to=" <<  s
			      << " input=*EPS*"
			      << " output=" << Fsa::LabelId(*p) << " "
			      << phonemes_->symbol(Fsa::LabelId(*p)) << std::endl;
	*/
	    }
	    for (std::vector<NoCoartBoundary>::const_iterator wb =
		    nonCoartBoundaries.begin(); wb != nonCoartBoundaries.end(); ++wb) {
		// std::cout << "nonCoartBoundary: input=" << wb->input_ << " -> " << allophones_->symbol(wb->input_) << "  phoneDisambiguator=" << wb->phoneDisambiguator_ << std::endl;
		wb->state_->newArc(s, semiring->one(), wb->input_,
			Fsa::LabelId(*p));
	/*
		std::cout << "(E) from=" << wb->state_->id() << " to=" <<  s << " input="
			  << wb->input_ << " " << allophones_->symbol(wb->input_)
			  << " output=" << Fsa::LabelId(*p) << " "
			  << phonemes_->symbol(Fsa::LabelId(*p)) << std::endl;
	*/

	    }
	}
    }
    /*
    Core::XmlWriter _xml(std::cout);
    _c->inputAlphabet()->writeXml(_xml);
    std::cout << "nDis = " << allophones_->nDisambiguators() << std::endl;
    */
    Fsa::ConstAutomatonRef c = Fsa::ConstAutomatonRef(_c);
    /*
    Fsa::write(c, "xml:C.xml", Fsa::storeAll, true);
    */
    c = Fsa::trim(c);
    Core::Ref<Fsa::StaticAutomaton> cs = Fsa::staticCopy(c);

    return Fsa::ConstAutomatonRef(cs);
}
