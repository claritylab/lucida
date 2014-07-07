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
#include <numeric>
#include "Accuracy.hh"
#include "Arithmetic.hh"
#include "Compose.hh"
#include <Fsa/Arithmetic.hh>
#include <Fsa/Basic.hh>
#include <Fsa/Cache.hh>
#include <Fsa/Compose.hh>
#include <Fsa/Determinize.hh>
#include <Fsa/Levenshtein.hh>
#include <Fsa/Minimize.hh>
#include <Fsa/Project.hh>
#include <Fsa/RemoveEpsilons.hh>
#include <Fsa/Static.hh>
#include <Core/Vector.hh>
#include <Core/Assertions.hh>
#include <Core/Hash.hh>
#include <Core/Types.hh>
#include <Bliss/Evaluation.hh>
#include <Bliss/Orthography.hh>
#include <Am/AcousticModel.hh>
#include <Speech/PhonemeSequenceAlignmentGenerator.hh>
#include <Core/Utility.hh>
#include <Speech/Confidences.hh>

namespace Lattice {

    /**
     * ExactAccuracyGenerator: base class
     */
    class ExactAccuracyGenerator
    {
    protected:
	Fsa::ConstAutomatonRef correct_;
	Fsa::ConstAutomatonRef lemmaPronToLemma_;
	Fsa::ConstAutomatonRef lemmaToEval_;
	f32 delCost_, insCost_, subCost_, corCost_;
    public:
	ExactAccuracyGenerator(f32 delCost, f32 insCost, f32 subCost, f32 corCost);
	virtual ~ExactAccuracyGenerator() {}

	virtual ConstWordLatticeRef getWordLattice(ConstWordLatticeRef) = 0;
    };

    ExactAccuracyGenerator::ExactAccuracyGenerator(
	f32 delCost, f32 insCost, f32 subCost, f32 corCost)
	:
	delCost_(delCost),
	insCost_(insCost),
	subCost_(subCost),
	corCost_(corCost)
    {}

    /**
     * ExactWordAccuracyGenerator: on word level
     */
    class ExactWordAccuracyGenerator : public ExactAccuracyGenerator
    {
	typedef ExactAccuracyGenerator Precursor;
    private:
	Fsa::ConstAutomatonRef lemmaPronToLemma_;
	Fsa::ConstAutomatonRef lemmaToEval_;
    private:
	Fsa::ConstAutomatonRef mapLemmaPronToEval(Fsa::ConstAutomatonRef);
	Fsa::ConstAutomatonRef mapEvalToLemmaPron(Fsa::ConstAutomatonRef);
    public:
	ExactWordAccuracyGenerator(const std::string &orth,
				   Bliss::OrthographicParser *orthToLemma,
				   Fsa::ConstAutomatonRef lemmaPronToLemma,
				   Fsa::ConstAutomatonRef lemmaToEval,
				   f32 delCost, f32 insCost, f32 subCost, f32 corCost);
	virtual ~ExactWordAccuracyGenerator() {}

	virtual ConstWordLatticeRef getWordLattice(ConstWordLatticeRef);
    };

    ExactWordAccuracyGenerator::ExactWordAccuracyGenerator(
	const std::string &orth,
	Bliss::OrthographicParser *orthToLemma,
	Fsa::ConstAutomatonRef lemmaPronToLemma,
	Fsa::ConstAutomatonRef lemmaToEval,
	f32 delCost, f32 insCost, f32 subCost, f32 corCost)
	:
	Precursor(delCost, insCost, subCost, corCost),
	lemmaPronToLemma_(lemmaPronToLemma),
	lemmaToEval_(lemmaToEval)
    {
	require(orthToLemma);
	correct_ =
	    Fsa::staticCopy(
		Fsa::minimize(
		    Fsa::determinize(
			Fsa::removeEpsilons(
			    Fsa::projectOutput(
				Fsa::composeMatching(
				    orthToLemma->createLemmaAcceptor(orth),
				    lemmaToEval_))))));
    }

    Fsa::ConstAutomatonRef ExactWordAccuracyGenerator::mapLemmaPronToEval(
	Fsa::ConstAutomatonRef pronLemma)
    {
	return
	    Fsa::minimize(
		Fsa::determinize(
		    Fsa::removeEpsilons(
			Fsa::cache(
			    Fsa::multiply(
				Fsa::projectOutput(
				    Fsa::composeMatching(
					Fsa::composeMatching(
					    Fsa::projectInput(pronLemma),
					    lemmaPronToLemma_),
					lemmaToEval_)),
				Fsa::Weight(f32(0)))))));
    }

    Fsa::ConstAutomatonRef ExactWordAccuracyGenerator::mapEvalToLemmaPron(Fsa::ConstAutomatonRef eval)
    {
	return Fsa::projectInput(
	    Fsa::composeMatching(
		Fsa::multiply(
		    lemmaPronToLemma_,
		    Fsa::Weight(f32(0))),
		Fsa::projectInput(
		    Fsa::composeMatching(
			Fsa::multiply(
			    lemmaToEval_,
			    Fsa::Weight(f32(0))),
			eval))));
    }

    ConstWordLatticeRef ExactWordAccuracyGenerator::getWordLattice(
	ConstWordLatticeRef lattice)
    {
	require(lattice->nParts() > 0);
	Fsa::ConstAutomatonRef fsa = Fsa::projectInput(lattice->part(0));
	if (fsa->getInputAlphabet() == lemmaPronToLemma_->getInputAlphabet()) {
	    fsa = mapLemmaPronToEval(fsa);
	} else {
	    Fsa::minimize(
		Fsa::determinize(
		    Fsa::removeEpsilons(
			Fsa::cache(fsa))));
	}
	fsa = Fsa::cache(
	    Fsa::levenshtein(
		correct_, fsa, delCost_, insCost_, subCost_, corCost_));
	if (lattice->part(0)->getInputAlphabet() == lemmaPronToLemma_->getInputAlphabet()) {
	    fsa = mapEvalToLemmaPron(fsa);
	}
	// for Fsa::minimize see comment in Lattice/Compose.hh
	fsa = staticCopy(minimize(determinize(removeEpsilons(staticCopy(fsa)))));
	return composeMatching(
	    Fsa::multiply(fsa, Fsa::Weight(f32(-1))),
	    multiply(lattice, Fsa::Weight(f32(0))));
    }

    ConstWordLatticeRef getExactWordAccuracy(
	ConstWordLatticeRef lattice,
	const std::string &orth,
	Bliss::OrthographicParser *orthToLemma,
	Fsa::ConstAutomatonRef lemmaPronToLemma,
	Fsa::ConstAutomatonRef lemmaToEval)
    {
	// accuracy: 0 1 0 -1
	// levenshtein: 1 1 1 0
	ExactWordAccuracyGenerator generator(
	    orth, orthToLemma, lemmaPronToLemma, lemmaToEval,
	    0, 1, 0, -1);
	ConstWordLatticeRef latticeWithAccuracy =
	    generator.getWordLattice(lattice);
	WordLattice *result = new WordLattice;
	result->setWordBoundaries(latticeWithAccuracy->wordBoundaries());
	result->setFsa(latticeWithAccuracy->part(0), WordLattice::accuracyFsa);
	return ConstWordLatticeRef(result);
    }

    /**
     * ExactPhonemeAccuracyGenerator
     */
    class ExactPhonemeAccuracyGenerator : public ExactAccuracyGenerator
    {
	typedef ExactAccuracyGenerator Precursor;
    private:
	Fsa::ConstAutomatonRef lemmaPronToPhoneme_;
    private:
	Fsa::ConstAutomatonRef mapLemmaPronToPhon(Fsa::ConstAutomatonRef);
	Fsa::ConstAutomatonRef mapPhonToLemmaPron(Fsa::ConstAutomatonRef);
    public:
	ExactPhonemeAccuracyGenerator(const std::string &orth,
				      Bliss::OrthographicParser *orthToLemma,
				      Fsa::ConstAutomatonRef lemmaPronToPhoneme,
				      Fsa::ConstAutomatonRef lemmaToPhoneme,
				      f32 delCost, f32 insCost, f32 subCost, f32 corCost);
	virtual ~ExactPhonemeAccuracyGenerator() {}

	virtual ConstWordLatticeRef getWordLattice(ConstWordLatticeRef);
    };

    ExactPhonemeAccuracyGenerator::ExactPhonemeAccuracyGenerator(
	const std::string &orth,
	Bliss::OrthographicParser *orthToLemma,
	Fsa::ConstAutomatonRef lemmaPronToPhoneme,
	Fsa::ConstAutomatonRef lemmaToPhoneme,
	f32 delCost, f32 insCost, f32 subCost, f32 corCost)
	:
	Precursor(delCost, insCost, subCost, corCost),
	lemmaPronToPhoneme_(lemmaPronToPhoneme)
    {
	correct_ =
	    Fsa::staticCopy(
		Fsa::minimize(
		    Fsa::determinize(
			Fsa::removeEpsilons(
			    Fsa::projectOutput(
				Fsa::composeMatching(
				    orthToLemma->createLemmaAcceptor(orth),
				    lemmaToPhoneme))))));
    }

    Fsa::ConstAutomatonRef ExactPhonemeAccuracyGenerator::mapLemmaPronToPhon(Fsa::ConstAutomatonRef lemmaPron)
    {
	return Fsa::cache(
	    Fsa::projectOutput(
		Fsa::composeMatching(
		    lemmaPron,
		    lemmaPronToPhoneme_)));
    }

    Fsa::ConstAutomatonRef ExactPhonemeAccuracyGenerator::mapPhonToLemmaPron(Fsa::ConstAutomatonRef phon)
    {
	return Fsa::removeEpsilons(
	    Fsa::cache(
		Fsa::projectInput(
		    Fsa::composeMatching(
			lemmaPronToPhoneme_,
			phon))));
    }


    ConstWordLatticeRef ExactPhonemeAccuracyGenerator::getWordLattice(
	ConstWordLatticeRef lattice)
    {
#if 1
	Fsa::ConstAutomatonRef result;
#endif

	// for Fsa::minimize see comment in Lattice/Compose.hh
	result = staticCopy(determinize(removeEpsilons(staticCopy(result))));
	return composeMatching(
	    result,
	    multiply(
		lattice,
		Fsa::Weight(f32(0))));
    }

    ConstWordLatticeRef getExactPhonemeAccuracy(
	ConstWordLatticeRef lattice,
	const std::string &orth,
	Bliss::OrthographicParser *orthToLemma,
	Fsa::ConstAutomatonRef lemmaPronToPhoneme,
	Fsa::ConstAutomatonRef lemmaToPhoneme)
    {
	// accuracy: 0 1 0 -1
	// levenshtein: 1 1 1 0
	ExactPhonemeAccuracyGenerator generator(
	    orth, orthToLemma, lemmaPronToPhoneme,
	    lemmaToPhoneme,
	    0, 1, 0, -1);
	ConstWordLatticeRef latticeWithAccuracy =
	    generator.getWordLattice(lattice);
	WordLattice *result = new WordLattice;
	result->setWordBoundaries(latticeWithAccuracy->wordBoundaries());
	result->setFsa(Fsa::multiply(latticeWithAccuracy->part(0),
				     Fsa::Weight(-1)),
		       WordLattice::accuracyFsa);
	return ConstWordLatticeRef(result);
    }

    /**
     * ApproximateAccuracyAutomaton
     */
    class ApproximateAccuracyAutomaton : public ModifyWordLattice
    {
    protected:
	struct TimeInterval
	{
	    Speech::TimeframeIndex startTime, endTime;
	    TimeInterval(Speech::TimeframeIndex _startTime, Speech::TimeframeIndex _endTime) :
		startTime(_startTime), endTime(_endTime) {}
	};
	struct TimeIntervalHash
	{
	    size_t operator()(const TimeInterval &i) const {
		return ((i.startTime & 0x0000ffff) | (i.startTime << 16));
	    }
	};
	struct TimeIntervalEquality
	{
	    bool operator()(const TimeInterval &i1, const TimeInterval &i2) const {
		return (i1.startTime == i2.startTime) && (i1.endTime == i2.endTime);
	    }
	};
	struct Hypothesis : public TimeInterval
	{
	    Fsa::LabelId label;
	    Hypothesis(Fsa::LabelId _label, Speech::TimeframeIndex _startTime, Speech::TimeframeIndex _endTime) :
		TimeInterval(_startTime, _endTime), label(_label) {}
	};
	typedef Core::hash_set<TimeInterval, TimeIntervalHash, TimeIntervalEquality> TimeIntervals;
	typedef Core::hash_map<Fsa::LabelId, TimeIntervals> States;
	typedef Core::Vector<States> ActiveStates;
    protected:
	ShortPauses shortPauses_;
	ActiveStates stateIds_;
	Core::Ref<const Bliss::LemmaPronunciationAlphabet> alphabet_;
    protected:
	virtual Fsa::LabelId label(Fsa::LabelId l) const { return l; }
	virtual f32 accuracy(const States &refs, const Hypothesis &h) const;
    public:
	ApproximateAccuracyAutomaton(
	    ConstWordLatticeRef, const ShortPauses &);
    };

    ApproximateAccuracyAutomaton::ApproximateAccuracyAutomaton(
	ConstWordLatticeRef lattice,
	const ShortPauses &shortPauses)
	:
	ModifyWordLattice(lattice),
	shortPauses_(shortPauses),
	alphabet_(required_cast(const Bliss::LemmaPronunciationAlphabet*,
				lattice->part(0)->getInputAlphabet().get()))
    {
	shortPauses_.insert(Fsa::Epsilon);
    }

    f32 ApproximateAccuracyAutomaton::accuracy(
	const States &refs, const Hypothesis &h) const
    {
	if (!refs.empty()) {
	    f32 acc = Core::Type<f32>::min;
	    for (States::const_iterator rIt = refs.begin(); rIt != refs.end(); ++ rIt) {
		for (TimeIntervals::const_iterator r = rIt->second.begin(); r != rIt->second.end(); ++ r) {
		    f32 overlap = std::min(h.endTime, r->endTime) - std::max(h.startTime, r->startTime);
		    verify(overlap >= 0);
		    require(r->startTime < r->endTime);
		    overlap /= (r->endTime - r->startTime);
		    f32 _acc = (label(rIt->first) == label(h.label)) ? -1 + 2 * overlap : -1 + overlap;
		    acc = std::max(_acc, acc);
		}
	    }
	    return acc;
	} else {
	    return 0;
	}
    }

    /**
     * ApproximateWordAccuracyAutomaton:
     *     Word lattices may contain epsilon arcs (input labels)
     *     but they must have vanishing duration.
     */
    class ApproximateWordAccuracyAutomaton : public ApproximateAccuracyAutomaton
    {
    private:
	bool useLemmata_;
    private:
	class LatticeToActiveStates : public DfsState
	{
	private:
	    ActiveStates &stateIds_;
	public:
	    LatticeToActiveStates(ConstWordLatticeRef, ActiveStates &);
	    virtual void discoverState(Fsa::ConstStateRef sp);
	};
    protected:
	virtual Fsa::LabelId label(Fsa::LabelId pronId) const {
	    if (useLemmata_) {
		const Bliss::LemmaPronunciation *lp = alphabet_->lemmaPronunciation(pronId);
		if (lp) return lp->lemma()->id();
		return Fsa::Epsilon;
	    } else {
		return pronId;
	    }
	}
    public:
	ApproximateWordAccuracyAutomaton(
	    ConstWordLatticeRef, ConstWordLatticeRef, const ShortPauses &, bool);
	virtual ~ApproximateWordAccuracyAutomaton() {}

	virtual std::string describe() const {
	    return Core::form("approximate-word-accuracy(%s)", fsa_->describe().c_str());
	}
	virtual void modifyState(Fsa::State *sp) const;
    };

    ApproximateWordAccuracyAutomaton::LatticeToActiveStates::LatticeToActiveStates(
	ConstWordLatticeRef correct, ActiveStates &stateIds) :
	DfsState(correct),
	stateIds_(stateIds) {}

    void ApproximateWordAccuracyAutomaton::LatticeToActiveStates::discoverState(Fsa::ConstStateRef sp)
    {
	const Speech::TimeframeIndex startTime = wordBoundaries_->time(sp->id());
	for (Fsa::State::const_iterator a = sp->begin(); a != sp->end(); ++ a) {
	    const Speech::TimeframeIndex endTime =
		wordBoundaries_->time(fsa_->getState(a->target())->id());
	    stateIds_.grow(endTime, States());
	    const TimeInterval times(startTime, endTime);
	    for (Speech::TimeframeIndex time = startTime; time < endTime; ++ time) {
		stateIds_[time][a->input()].insert(times);
	    }
	}
    }

    ApproximateWordAccuracyAutomaton::ApproximateWordAccuracyAutomaton(
	ConstWordLatticeRef lattice,
	ConstWordLatticeRef correct,
	const ShortPauses &shortPauses,
	bool useLemmata)
	:
	ApproximateAccuracyAutomaton(lattice, shortPauses),
	useLemmata_(useLemmata)
    {
	LatticeToActiveStates s(correct, stateIds_);
	s.dfs();
    }

    void ApproximateWordAccuracyAutomaton::modifyState(Fsa::State *sp) const
    {
	const Speech::TimeframeIndex startTime = wordBoundaries_->time(sp->id());
	for (Fsa::State::iterator a = sp->begin(); a != sp->end(); ++ a) {
	    if (shortPauses_.find(label(a->input())) == shortPauses_.end()) {
		const Speech::TimeframeIndex endTime =
		    wordBoundaries_->time(fsa_->getState(a->target())->id());
		States refs;
		for (Speech::TimeframeIndex time = startTime; time < endTime; ++ time) {
		    for (States::const_iterator rIt = stateIds_[time].begin(); rIt != stateIds_[time].end(); ++ rIt) {
			refs[rIt->first].insert(rIt->second.begin(), rIt->second.end());
		    }
		}
		a->weight_ = Fsa::Weight(
		    accuracy(refs, Hypothesis(a->input(), startTime, endTime)));
	    } else {
		a->weight_ = Fsa::Weight(f32(0));
	    }
	}
    }

    /**
     * ApproximatePhoneAccuracyAutomaton
     */
    class ApproximatePhoneAccuracyAutomaton : public ApproximateAccuracyAutomaton
    {
    protected:
	Speech::AlignmentGeneratorRef alignmentGenerator_;
	Core::Ref<const Am::AllophoneStateAlphabet> allophoneStateAlphabet_;
    private:
	class AlignmentToActiveStates
	{
	public:
	    AlignmentToActiveStates(const Speech::Alignment &, ApproximatePhoneAccuracyAutomaton &parent);
	};

	class LatticeToActiveStates : public DfsState
	{
	private:
	    ApproximatePhoneAccuracyAutomaton &parent_;
	public:
	    LatticeToActiveStates(ConstWordLatticeRef, ApproximatePhoneAccuracyAutomaton &parent);
	    virtual void discoverState(Fsa::ConstStateRef sp);
	};
	friend class AlignmentToSoftAccuracies;
	friend class LatticeToActiveStates;
    protected:
	virtual Fsa::LabelId label(Fsa::LabelId e) const {
	    return allophoneStateAlphabet_->allophoneState(e).allophone()->central();
	}
    public:
	ApproximatePhoneAccuracyAutomaton(
	    ConstWordLatticeRef, ConstWordLatticeRef, const ShortPauses &, Speech::AlignmentGeneratorRef);
	ApproximatePhoneAccuracyAutomaton(
	    ConstWordLatticeRef, const Speech::Alignment &, const ShortPauses &, Speech::AlignmentGeneratorRef);

	virtual std::string describe() const {
	    return Core::form("approximate-phone-accuracy(%s)", fsa_->describe().c_str());
	}
	virtual void modifyState(Fsa::State *sp) const;
    };

    ApproximatePhoneAccuracyAutomaton::LatticeToActiveStates::LatticeToActiveStates(
	ConstWordLatticeRef correct,
	ApproximatePhoneAccuracyAutomaton &parent)
	:
	DfsState(correct),
	parent_(parent)
    {}

    void ApproximatePhoneAccuracyAutomaton::LatticeToActiveStates::discoverState(Fsa::ConstStateRef sp)
    {
	const Speech::TimeframeIndex startTime = wordBoundaries_->time(sp->id());
	for (Fsa::State::const_iterator a = sp->begin(); a != sp->end(); ++ a) {
	    const Bliss::LemmaPronunciation *pronunciation = parent_.alphabet_->lemmaPronunciation(a->input());
	    if (!pronunciation) {
		continue;
	    }
	    const Speech::TimeframeIndex endTime =
		wordBoundaries_->time(fsa_->getState(a->target())->id());
	    Bliss::Coarticulated<Bliss::LemmaPronunciation> coarticulatedPronunciation(
		*pronunciation, wordBoundaries_->transit(sp->id()).final,
		wordBoundaries_->transit(fsa_->getState(a->target())->id()).initial);
	    const Speech::Alignment *alignment =
		parent_.alignmentGenerator_->getAlignment(coarticulatedPronunciation, startTime, endTime);
	    parent_.stateIds_.grow(endTime, States());
	    for (std::vector<Speech::AlignmentItem>::const_iterator al = alignment->begin(); al != alignment->end();) {
		Fsa::LabelId _label = parent_.label(al->emission);
		std::vector<Speech::AlignmentItem>::const_iterator bl = al;
		for (; bl != alignment->end(); ++ bl) {
		    if (parent_.label(bl->emission) != _label) break;
		}
		Speech::TimeframeIndex blTime = bl != alignment->end() ? bl->time : endTime;
		TimeInterval times(al->time, blTime);
		for (Speech::TimeframeIndex time = al->time; time < blTime; ++ time) {
		    parent_.stateIds_[time][al->emission].insert(times);
		}
		al = bl;
	    }
	}
    }

    ApproximatePhoneAccuracyAutomaton::AlignmentToActiveStates::AlignmentToActiveStates(
	const Speech::Alignment &alignment,
	ApproximatePhoneAccuracyAutomaton &parent)
    {
	ApproximatePhoneAccuracyAutomaton::ActiveStates &stateIds = parent.stateIds_;
	stateIds.grow(alignment.back().time, States());
	for (Speech::Alignment::const_iterator al = alignment.begin(); al != alignment.end();) {
	    const Fsa::LabelId _label = parent.label(al->emission);
	    Speech::Alignment::const_iterator bl = al;
	    for (; bl != (alignment.end() - 1);) {
		Speech::Alignment::const_iterator cl = bl + 1;
		if (parent.label(cl->emission) != _label) break;
		bl = cl;
	    }
	    verify(stateIds.size() > bl->time);
	    TimeInterval times(al->time, bl->time);
	    for (Speech::TimeframeIndex time = al->time; time < bl->time; ++ time) {
		stateIds[time][al->emission].insert(times);
	    }
	    al = ++ bl;
	}
    }

    ApproximatePhoneAccuracyAutomaton::ApproximatePhoneAccuracyAutomaton(
	ConstWordLatticeRef lattice,
	ConstWordLatticeRef correct,
	const ShortPauses &shortPauses,
	Speech::AlignmentGeneratorRef alignmentGenerator)
	:
	ApproximateAccuracyAutomaton(lattice, shortPauses),
	alignmentGenerator_(alignmentGenerator),
	allophoneStateAlphabet_(alignmentGenerator->acousticModel()->allophoneStateAlphabet())
    {
	LatticeToActiveStates s(correct, *this);
	s.dfs();
    }

    ApproximatePhoneAccuracyAutomaton::ApproximatePhoneAccuracyAutomaton(
	ConstWordLatticeRef lattice,
	const Speech::Alignment &alignment,
	const ShortPauses &shortPauses,
	Speech::AlignmentGeneratorRef alignmentGenerator)
	:
	ApproximateAccuracyAutomaton(lattice, shortPauses),
	alignmentGenerator_(alignmentGenerator),
	allophoneStateAlphabet_(alignmentGenerator->acousticModel()->allophoneStateAlphabet())
    {
	AlignmentToActiveStates s(alignment, *this);
    }

    void ApproximatePhoneAccuracyAutomaton::modifyState(Fsa::State *sp) const
    {
	const Speech::TimeframeIndex startTime = wordBoundaries_->time(sp->id());
	for (Fsa::State::iterator a = sp->begin(); a != sp->end(); ++ a) {
	    const Bliss::LemmaPronunciation *pronunciation = alphabet_->lemmaPronunciation(a->input());
	    f32 weight = 0;
	    if (pronunciation) {
		const Speech::TimeframeIndex endTime =
		    wordBoundaries_->time(fsa_->getState(a->target())->id());
		Bliss::Coarticulated<Bliss::LemmaPronunciation> coarticulatedPronunciation(
		    *pronunciation, wordBoundaries_->transit(sp->id()).final,
		    wordBoundaries_->transit(fsa_->getState(a->target())->id()).initial);
		const Speech::Alignment *alignment =
		    alignmentGenerator_->getAlignment(coarticulatedPronunciation, startTime, endTime);
		for (std::vector<Speech::AlignmentItem>::const_iterator al = alignment->begin(); al != alignment->end();) {
		    Fsa::LabelId _label = label(al->emission);
		    std::vector<Speech::AlignmentItem>::const_iterator bl = al;
		    for (; bl != alignment->end(); ++ bl) {
			if (label(bl->emission) != _label) break;
		    }
		    if (shortPauses_.find(_label) == shortPauses_.end()) {
			States refs;
			Speech::TimeframeIndex blTime = bl != alignment->end() ? bl->time : endTime;
			for (Speech::TimeframeIndex time = al->time; time < blTime; ++ time) {
			    if (stateIds_.size() > time) {
				for (States::const_iterator rIt = stateIds_[time].begin(); rIt != stateIds_[time].end(); ++ rIt) {
				    refs[rIt->first].insert(rIt->second.begin(), rIt->second.end());
				}
			    } else {
// 				std::cerr << "corrupted lattice?" << std::endl;
			    }
			}
			weight += accuracy(refs, Hypothesis(al->emission, al->time, blTime));
		    }
		    al = bl;
		}
	    }
	    a->weight_ = Fsa::Weight(weight);
	}
    }

    ConstWordLatticeRef getApproximateAccuracy(Core::Ref<const ApproximateAccuracyAutomaton> a)
    {
	Core::Ref<WordLattice> result(new WordLattice);
	result->setWordBoundaries(a->wordBoundaries());
	result->setFsa(Fsa::cache(a), WordLattice::accuracyFsa);
	return result;
    }

    ConstWordLatticeRef getApproximateWordAccuracy(
	ConstWordLatticeRef lattice,
	ConstWordLatticeRef correct,
	const ShortPauses &shortPauses,
	bool useLemmata)
    {
	Core::Ref<ApproximateAccuracyAutomaton> a(
	    new ApproximateWordAccuracyAutomaton(lattice, correct, shortPauses, useLemmata));
	return getApproximateAccuracy(a);
    }

    ConstWordLatticeRef getApproximatePhoneAccuracy(
	ConstWordLatticeRef lattice,
	ConstWordLatticeRef correct,
	const ShortPauses &shortPauses,
	Speech::AlignmentGeneratorRef alignmentGenerator)
    {
	Core::Ref<ApproximateAccuracyAutomaton> a(
	    new ApproximatePhoneAccuracyAutomaton(lattice, correct, shortPauses, alignmentGenerator));
	return getApproximateAccuracy(a);
    }

    /**
     * ApproximatePhoneAccuracyMaskAutomaton
     */
    class ApproximatePhoneAccuracyMaskAutomaton : public ApproximatePhoneAccuracyAutomaton
    {
    protected:
	virtual f32 accuracy(const States &refs, const Hypothesis &h) const;
	const Speech::Confidences &mask_;
    public:
	ApproximatePhoneAccuracyMaskAutomaton(
	    ConstWordLatticeRef, ConstWordLatticeRef, const Speech::Confidences &, const ShortPauses &, Speech::AlignmentGeneratorRef);

	virtual std::string describe() const {
	    return Core::form("approximate-phone-accuracy-mask(%s)", fsa_->describe().c_str());
	}
    };

    ApproximatePhoneAccuracyMaskAutomaton::ApproximatePhoneAccuracyMaskAutomaton(
	ConstWordLatticeRef lattice,
	ConstWordLatticeRef correct,
	const Speech::Confidences &mask,
	const ShortPauses &shortPauses,
	Speech::AlignmentGeneratorRef alignmentGenerator)
	:
	ApproximatePhoneAccuracyAutomaton(lattice, correct, shortPauses, alignmentGenerator),
	mask_(mask)
    {
    }

    f32 ApproximatePhoneAccuracyMaskAutomaton::accuracy(
	const States &refs, const Hypothesis &h) const
    {
	f32 weight = 0.0;
	for (Speech::TimeframeIndex time = h.startTime; time < h.endTime; ++ time) {
	    weight += f32(mask_[time]);
	}
	weight = weight/(h.endTime-h.startTime);
	return weight * ApproximatePhoneAccuracyAutomaton::accuracy(refs, h);
    }

    ConstWordLatticeRef getApproximatePhoneAccuracyMask(
	ConstWordLatticeRef lattice,
	ConstWordLatticeRef correct,
	const Speech::Confidences &mask,
	const ShortPauses &shortPauses,
	Speech::AlignmentGeneratorRef alignmentGenerator)
    {
	Core::Ref<ApproximateAccuracyAutomaton> a(
	    new ApproximatePhoneAccuracyMaskAutomaton(lattice, correct, mask, shortPauses, alignmentGenerator));
	return getApproximateAccuracy(a);
    }

    /**
     * FramePhoneAccuracyAutomaton
     */
    class FramePhoneAccuracyAutomaton : public ApproximatePhoneAccuracyAutomaton
    {
    private:
	f32 normalization_;
    protected:
	virtual f32 accuracy(const States &refs, const Hypothesis &h) const;
    public:
	FramePhoneAccuracyAutomaton(
	    ConstWordLatticeRef, ConstWordLatticeRef, const ShortPauses &,
	    Speech::AlignmentGeneratorRef, f32 normalization);

	virtual std::string describe() const {
	    return Core::form("frame-phone-accuracy-%f(%s)", normalization_, fsa_->describe().c_str());
	}
    };

    FramePhoneAccuracyAutomaton::FramePhoneAccuracyAutomaton(
	ConstWordLatticeRef lattice,
	ConstWordLatticeRef correct,
	const ShortPauses &shortPauses,
	Speech::AlignmentGeneratorRef alignmentGenerator,
	f32 normalization)
	:
	ApproximatePhoneAccuracyAutomaton(lattice, correct, shortPauses, alignmentGenerator),
	normalization_(normalization)
    {}

    f32 FramePhoneAccuracyAutomaton::accuracy(
	const States &refs, const Hypothesis &h) const
    {
	const Fsa::LabelId _label = label(h.label);
	const Speech::TimeframeIndex nFrames = h.endTime - h.startTime;
	std::vector<s8> accs(nFrames, 0);
	for (States::const_iterator rIt = refs.begin(); rIt != refs.end(); ++ rIt) {
	    for (TimeIntervals::const_iterator r = rIt->second.begin(); r != rIt->second.end(); ++ r) {
		if ((label(rIt->first) == _label)) {
		    std::fill(accs.begin() + std::max((s32)r->startTime - (s32)h.startTime, s32(0)),
			      accs.begin() + std::min((s32)r->endTime - (s32)h.startTime, (s32)accs.size()),
			      1);
		}
	    }
	}
	return (f32)std::accumulate(accs.begin(), accs.end(), 0) / (1 + normalization_ * (nFrames - 1));
    }

    ConstWordLatticeRef getFramePhoneAccuracy(
	ConstWordLatticeRef lattice,
	ConstWordLatticeRef correct,
	const ShortPauses &shortPauses,
	Speech::AlignmentGeneratorRef alignmentGenerator,
	f32 normalization)
    {
	Core::Ref<ApproximateAccuracyAutomaton> a(
	    new FramePhoneAccuracyAutomaton(lattice, correct, shortPauses, alignmentGenerator, normalization));
	return getApproximateAccuracy(a);
    }

    /**
     * FrameStateAccuracyAutomaton
     */
    class FrameStateAccuracyAutomaton : public ModifyWordLattice
    {
    private:
	typedef Core::hash_map<Fsa::LabelId, bool> States;
	typedef Core::Vector<States> ActiveStates;
    protected:
	Core::Ref<const Bliss::LemmaPronunciationAlphabet> alphabet_;
	ActiveStates stateIds_;
	Speech::AlignmentGeneratorRef alignmentGenerator_;
	const ShortPauses &shortPauses_;
    private:
	class LatticeToActiveStates : public DfsState
	{
	private:
	    FrameStateAccuracyAutomaton &parent_;
	public:
	    LatticeToActiveStates(ConstWordLatticeRef, FrameStateAccuracyAutomaton &parent);
	    virtual void discoverState(Fsa::ConstStateRef sp);
	};
    protected:
	Fsa::LabelId label(Fsa::LabelId e) const {
	    return alignmentGenerator_->acousticModel()->emissionIndex(e);
	}
	bool isShortPause(Fsa::LabelId label) const {
	    return shortPauses_.find(label) != shortPauses_.end();
	}
	f32 accuracy(const States &refs, const Fsa::LabelId &h) const;
    public:
	FrameStateAccuracyAutomaton(
	    ConstWordLatticeRef, ConstWordLatticeRef, const ShortPauses &, Speech::AlignmentGeneratorRef);
	virtual std::string describe() const {
	    return Core::form("frame-state-accuracy(%s)", fsa_->describe().c_str());
	}
	virtual void modifyState(Fsa::State *sp) const;
    };

    FrameStateAccuracyAutomaton::LatticeToActiveStates::LatticeToActiveStates(
	ConstWordLatticeRef correct, FrameStateAccuracyAutomaton &parent) :
	DfsState(correct),
	parent_(parent) {}

    void FrameStateAccuracyAutomaton::LatticeToActiveStates::discoverState(Fsa::ConstStateRef sp)
    {
	const Speech::TimeframeIndex startTime = wordBoundaries_->time(sp->id());
	for (Fsa::State::const_iterator a = sp->begin(); a != sp->end(); ++ a) {
	    const Bliss::LemmaPronunciation *pronunciation = parent_.alphabet_->lemmaPronunciation(a->input());
	    if (!pronunciation) continue;
	    const Speech::TimeframeIndex endTime =
		wordBoundaries_->time(fsa_->getState(a->target())->id());
	    Bliss::Coarticulated<Bliss::LemmaPronunciation> coarticulatedPronunciation(
		*pronunciation, wordBoundaries_->transit(sp->id()).final,
		wordBoundaries_->transit(fsa_->getState(a->target())->id()).initial);
	    const Speech::Alignment *alignment =
		parent_.alignmentGenerator_->getAlignment(coarticulatedPronunciation, startTime, endTime);
	    for (std::vector<Speech::AlignmentItem>::const_iterator al = alignment->begin(); al != alignment->end(); ++ al) {
		parent_.stateIds_.grow(al->time, States());
		parent_.stateIds_[al->time][parent_.label(al->emission)] = true;
	    }
	}
    }

    FrameStateAccuracyAutomaton::FrameStateAccuracyAutomaton(
	ConstWordLatticeRef lattice,
	ConstWordLatticeRef correct,
	const ShortPauses &shortPauses,
	Speech::AlignmentGeneratorRef alignmentGenerator)
	:
	ModifyWordLattice(lattice),
	alphabet_(required_cast(const Bliss::LemmaPronunciationAlphabet*,
				lattice->part(0)->getInputAlphabet().get())),
	alignmentGenerator_(alignmentGenerator),
	shortPauses_(shortPauses)
    {
	LatticeToActiveStates s(correct, *this);
	s.dfs();
    }

    void FrameStateAccuracyAutomaton::modifyState(Fsa::State *sp) const
    {
	const Speech::TimeframeIndex startTime = wordBoundaries_->time(sp->id());
	for (Fsa::State::iterator a = sp->begin(); a != sp->end(); ++ a) {
	    const Bliss::LemmaPronunciation *pronunciation = alphabet_->lemmaPronunciation(a->input());
	    f32 weight = 0;
	    if (pronunciation) {
		const Speech::TimeframeIndex endTime =
		    wordBoundaries_->time(fsa_->getState(a->target())->id());
		Bliss::Coarticulated<Bliss::LemmaPronunciation> coarticulatedPronunciation(
		    *pronunciation, wordBoundaries_->transit(sp->id()).final,
		    wordBoundaries_->transit(fsa_->getState(a->target())->id()).initial);
		const Speech::Alignment *alignment =
		    alignmentGenerator_->getAlignment(coarticulatedPronunciation, startTime, endTime);
		for (std::vector<Speech::AlignmentItem>::const_iterator al = alignment->begin(); al != alignment->end(); ++ al) {
		    weight += accuracy(stateIds_[al->time], label(al->emission));
		}
	    }
	    a->weight_ = Fsa::Weight(weight);
	}
    }

    f32 FrameStateAccuracyAutomaton::accuracy(const States &refs, const Fsa::LabelId &h) const
    {
	return isShortPause(h) ? 0 : (refs.find(h) != refs.end() ? 1 : 0);
    }

    ConstWordLatticeRef getFrameStateAccuracy(
	ConstWordLatticeRef lattice,
	ConstWordLatticeRef correct,
	const ShortPauses &shortPauses,
	Speech::AlignmentGeneratorRef alignmentGenerator)
    {
	Core::Ref<FrameStateAccuracyAutomaton> a(
	    new FrameStateAccuracyAutomaton(lattice, correct, shortPauses, alignmentGenerator));
	Core::Ref<WordLattice> result(new WordLattice);
	result->setWordBoundaries(a->wordBoundaries());
	result->setFsa(Fsa::cache(a), WordLattice::accuracyFsa);
	return result;
    }


    /**
     * SoftFramePhoneAccuracyAutomaton
     */
    class SoftFramePhoneAccuracyAutomaton : public ApproximatePhoneAccuracyAutomaton
    {
    private:
	class AlignmentToSoftAccuracies
	{
	protected:
	    SoftFramePhoneAccuracyAutomaton &parent_;
	protected:
	    bool isShortPause(Fsa::LabelId label) const {
		return parent_.shortPauses_.find(label) != parent_.shortPauses_.end();
	    }
	public:
	    AlignmentToSoftAccuracies(SoftFramePhoneAccuracyAutomaton &parent) : parent_(parent) {}
	    AlignmentToSoftAccuracies(const Speech::Alignment &, SoftFramePhoneAccuracyAutomaton &);
	};
	class LatticeToSoftAccuracies : public AlignmentToSoftAccuracies, public DfsState
	{
	public:
	    LatticeToSoftAccuracies(ConstWordLatticeRef, SoftFramePhoneAccuracyAutomaton &);
	    virtual void discoverState(Fsa::ConstStateRef sp);
	};
	friend class LatticeToSoftAccuracies;
    private:
	typedef Core::hash_map<Fsa::LabelId, f32> SoftAccuracy;
	typedef Core::Vector<SoftAccuracy> SoftAccuracies;
	SoftAccuracies softAccuracies_;
    protected:
	virtual f32 accuracy(const States &refs, const Hypothesis &h) const;
    public:
	SoftFramePhoneAccuracyAutomaton(
	    ConstWordLatticeRef, ConstWordLatticeRef, const ShortPauses &,
	    Speech::AlignmentGeneratorRef);
	SoftFramePhoneAccuracyAutomaton(
	    ConstWordLatticeRef, const Speech::Alignment &, const ShortPauses &,
	    Speech::AlignmentGeneratorRef);

	virtual std::string describe() const {
	    return Core::form("soft-frame-phone-accuracy(%s)", fsa_->describe().c_str());
	}
    };

    SoftFramePhoneAccuracyAutomaton::LatticeToSoftAccuracies::LatticeToSoftAccuracies(
	ConstWordLatticeRef lattice,
	SoftFramePhoneAccuracyAutomaton &parent)
	:
	SoftFramePhoneAccuracyAutomaton::AlignmentToSoftAccuracies(parent),
	DfsState(lattice)
    {}

    void SoftFramePhoneAccuracyAutomaton::LatticeToSoftAccuracies::discoverState(Fsa::ConstStateRef sp)
    {
	const Speech::TimeframeIndex startTime = wordBoundaries_->time(sp->id());
	SoftAccuracies &accs = parent_.softAccuracies_;
	Core::Ref<const Bliss::LemmaPronunciationAlphabet> alphabet = parent_.alphabet_;
	for (Fsa::State::const_iterator a = sp->begin(); a != sp->end(); ++ a) {
	    const Bliss::LemmaPronunciation *pronunciation = alphabet->lemmaPronunciation(a->input());
	    if (!pronunciation) continue;
	    const Speech::TimeframeIndex endTime =
		wordBoundaries_->time(fsa_->getState(a->target())->id());
	    Bliss::Coarticulated<Bliss::LemmaPronunciation> coarticulatedPronunciation(
		*pronunciation, wordBoundaries_->transit(sp->id()).final,
		wordBoundaries_->transit(fsa_->getState(a->target())->id()).initial);
	    const Speech::Alignment *alignment =
		parent_.alignmentGenerator_->getAlignment(coarticulatedPronunciation, startTime, endTime);
	    accs.grow(endTime);
	    for (std::vector<Speech::AlignmentItem>::const_iterator al = alignment->begin(); al != alignment->end(); ++ al) {
		const Fsa::LabelId _label = parent_.label(al->emission);
		SoftAccuracy::iterator accIt =
		    accs[al->time].insert(std::pair<Fsa::LabelId, f32>(_label, 0)).first;
		if (!isShortPause(_label)) {
		    accIt->second += f32(a->weight());
		}
	    }
	}
    }

    SoftFramePhoneAccuracyAutomaton::AlignmentToSoftAccuracies::AlignmentToSoftAccuracies(
	const Speech::Alignment &alignment,
	SoftFramePhoneAccuracyAutomaton &parent) :
	parent_(parent)
    {
	SoftAccuracies &accs = parent_.softAccuracies_;
	accs.grow(alignment.back().time);
	for (Speech::Alignment::const_iterator al = alignment.begin(); al != alignment.end(); ++ al) {
	    verify(accs.size() > al->time);
	    const Fsa::LabelId _label = parent_.label(al->emission);
	    SoftAccuracy::iterator accIt =
		accs[al->time].insert(std::pair<Fsa::LabelId, f32>(_label, 0)).first;
	    if (!isShortPause(_label)) {
		accIt->second += f32(al->weight);
	    }
	}
    }

    SoftFramePhoneAccuracyAutomaton::SoftFramePhoneAccuracyAutomaton(
	ConstWordLatticeRef lattice,
	ConstWordLatticeRef correct,
	const ShortPauses &shortPauses,
	Speech::AlignmentGeneratorRef alignmentGenerator)
	:
	ApproximatePhoneAccuracyAutomaton(lattice, correct, shortPauses, alignmentGenerator)
    {
	LatticeToSoftAccuracies s(correct, *this);
	s.dfs();
    }

    SoftFramePhoneAccuracyAutomaton::SoftFramePhoneAccuracyAutomaton(
	ConstWordLatticeRef lattice,
	const Speech::Alignment &forcedAlignment,
	const ShortPauses &shortPauses,
	Speech::AlignmentGeneratorRef alignmentGenerator)
	:
	ApproximatePhoneAccuracyAutomaton(lattice, forcedAlignment, shortPauses, alignmentGenerator)
    {
	AlignmentToSoftAccuracies s(forcedAlignment, *this);
    }

    f32 SoftFramePhoneAccuracyAutomaton::accuracy(
	const States &refs, const Hypothesis &h) const
    {
	const Fsa::LabelId _label = label(h.label);
	const Speech::TimeframeIndex nFrames = h.endTime - h.startTime;
	std::vector<f32> accs(nFrames, f32(0));
	for (States::const_iterator rIt = refs.begin(); rIt != refs.end(); ++ rIt) {
	    for (TimeIntervals::const_iterator r = rIt->second.begin(); r != rIt->second.end(); ++ r) {
		if ((label(rIt->first) == _label)) {
		    Speech::TimeframeIndex t = std::max((s32)r->startTime - (s32)h.startTime, s32(0));
		    Speech::TimeframeIndex at = std::max(r->startTime, h.startTime);
		    for (; t < (u32)std::min((s32)r->endTime - (s32)h.startTime, (s32)accs.size()); ++ t, ++ at) {
			accs[t] = std::max(accs[t], softAccuracies_[at].find(_label)->second);
		    }
		}
	    }
	}
	return std::accumulate(accs.begin(), accs.end(), f32(0));
    }

    ConstWordLatticeRef getSoftFramePhoneAccuracy(
	ConstWordLatticeRef lattice,
	ConstWordLatticeRef correct,
	const ShortPauses &shortPauses,
	Speech::AlignmentGeneratorRef alignmentGenerator)
    {
	Core::Ref<ApproximateAccuracyAutomaton> a(
	    new SoftFramePhoneAccuracyAutomaton(lattice, correct, shortPauses, alignmentGenerator));
	return getApproximateAccuracy(a);
    }

    ConstWordLatticeRef getSoftFramePhoneAccuracy(
	ConstWordLatticeRef lattice,
	const Speech::Alignment &forcedAlignment,
	const ShortPauses &shortPauses,
	Speech::AlignmentGeneratorRef alignmentGenerator)
    {
	Core::Ref<ApproximateAccuracyAutomaton> a(
	    new SoftFramePhoneAccuracyAutomaton(lattice, forcedAlignment, shortPauses, alignmentGenerator));
	return getApproximateAccuracy(a);
    }

    /**
     * WeightedFramePhoneAccuracyAutomaton
     */
    class WeightedFramePhoneAccuracyAutomaton : public ApproximatePhoneAccuracyAutomaton
    {
    private:
	class SetDerivativesDfsState : public DfsState
	{
	private:
	    // without constant factor @param beta
	    struct diffSigmoid : public std::unary_function<f64, f64> {
		const f64 beta, marginFactor;
		static const s64 tol = 45035996274LL; // = Core::differenceUlp(1, 1.00001)
		static constexpr const f64 inf = 1e9;
		f32 operator()(f64 x) const {
		    require(!Core::isSignificantlyLessUlp(x, 0, tol) && !Core::isSignificantlyLessUlp(1, x, tol));
		    if (beta != 1) {
			if (Core::isAlmostEqualUlp(x, 0, tol) || Core::isAlmostEqualUlp(x, 1, tol)) {
			    return beta > 1 ? 0 : inf;
			} else {
			    const f64 s = pow(x, beta) / (marginFactor * pow(1 - x, beta) + pow(x, beta));
			    const f64 ds = (s * (1 - s)) / (x * (1 - x));
			    return ds < inf ? (f32)(beta * ds) : inf;
			}
		    } else {
			return 1;
		    }
		}
		diffSigmoid(f64 _beta, f64 _margin) : beta(_beta), marginFactor(exp(beta * _margin)) { require(beta > 0); }
	    };
	private:
	    WeightedFramePhoneAccuracyAutomaton &parent_;
	private:
	    bool isShortPause(Fsa::LabelId label) const {
		return parent_.shortPauses_.find(label) != parent_.shortPauses_.end();
	    }
	    bool isActive(Fsa::LabelId label, const States &states) const {
		return states.find(label) != states.end();
	    }
	    bool isCorrect(Fsa::LabelId label, Speech::TimeframeIndex time) const {
		return (isShortPause(label) || isActive(label, parent_.stateIds_[time]));
	    }
	public:
	    SetDerivativesDfsState(ConstWordLatticeRef, WeightedFramePhoneAccuracyAutomaton &);
	    virtual void discoverState(Fsa::ConstStateRef sp);
	    virtual void finish();
	};
	friend class SetDerivativesDfsState;
    private:
	f32 beta_;
	f32 margin_;
	Core::Vector<f32> derivatives_;
    protected:
	virtual f32 accuracy(const States &refs, const Hypothesis &h) const;
    public:
	WeightedFramePhoneAccuracyAutomaton(
	    ConstWordLatticeRef, ConstWordLatticeRef, const ShortPauses &,
	    Speech::AlignmentGeneratorRef, f32 beta, f32 margin);

	virtual std::string describe() const {
	    return Core::form("weighted-frame-phone-accuracy-%f(%s)", beta_, fsa_->describe().c_str());
	}
    };

    WeightedFramePhoneAccuracyAutomaton::SetDerivativesDfsState::SetDerivativesDfsState(
	ConstWordLatticeRef lattice,
	WeightedFramePhoneAccuracyAutomaton &parent)
	:
	DfsState(lattice),
	parent_(parent)
    {}

    void WeightedFramePhoneAccuracyAutomaton::SetDerivativesDfsState::discoverState(Fsa::ConstStateRef sp)
    {
	const Speech::TimeframeIndex startTime = wordBoundaries_->time(sp->id());
	Core::Vector<f32> &drvs = parent_.derivatives_;
	Core::Ref<const Bliss::LemmaPronunciationAlphabet> alphabet = parent_.alphabet_;
	for (Fsa::State::const_iterator a = sp->begin(); a != sp->end(); ++ a) {
	    const Bliss::LemmaPronunciation *pronunciation = alphabet->lemmaPronunciation(a->input());
	    if (!pronunciation) continue;
	    const Speech::TimeframeIndex endTime =
		wordBoundaries_->time(fsa_->getState(a->target())->id());
	    Bliss::Coarticulated<Bliss::LemmaPronunciation> coarticulatedPronunciation(
		*pronunciation, wordBoundaries_->transit(sp->id()).final,
		wordBoundaries_->transit(fsa_->getState(a->target())->id()).initial);
	    const Speech::Alignment *alignment =
		parent_.alignmentGenerator_->getAlignment(coarticulatedPronunciation, startTime, endTime);
	    drvs.grow(endTime, 0);
	    for (std::vector<Speech::AlignmentItem>::const_iterator al = alignment->begin(); al != alignment->end(); ++ al) {
		if (isCorrect(parent_.label(al->emission), al->time)) {
		    drvs[al->time] += f32(a->weight());
		}
	    }
	}
    }

    void WeightedFramePhoneAccuracyAutomaton::SetDerivativesDfsState::finish()
    {
	Core::Vector<f32> &drvs = parent_.derivatives_;
	std::transform(drvs.begin(), drvs.end(), drvs.begin(), diffSigmoid(parent_.beta_, parent_.margin_));
    }

    WeightedFramePhoneAccuracyAutomaton::WeightedFramePhoneAccuracyAutomaton(
	ConstWordLatticeRef lattice,
	ConstWordLatticeRef correct,
	const ShortPauses &shortPauses,
	Speech::AlignmentGeneratorRef alignmentGenerator,
	f32 beta,
	f32 margin)
	:
	ApproximatePhoneAccuracyAutomaton(lattice, correct, shortPauses, alignmentGenerator),
	beta_(beta),
	margin_(margin)
    {
	require(beta_ > 0);
	SetDerivativesDfsState s(lattice, *this);
	s.dfs();
    }

    f32 WeightedFramePhoneAccuracyAutomaton::accuracy(
	const States &refs, const Hypothesis &h) const
    {
	const Fsa::LabelId _label = label(h.label);
	const Speech::TimeframeIndex nFrames = h.endTime - h.startTime;
	std::vector<f32> accs(nFrames, f32(0));
	for (States::const_iterator rIt = refs.begin(); rIt != refs.end(); ++ rIt) {
	    if ((label(rIt->first) == _label)) {
		for (TimeIntervals::const_iterator r = rIt->second.begin(); r != rIt->second.end(); ++ r) {
		    std::copy(
			derivatives_.begin() + std::max((s32)r->startTime - (s32)h.startTime, s32(0)),
			derivatives_.begin() + std::min((s32)r->endTime - (s32)h.startTime, (s32)accs.size()),
			accs.begin());
		}
	    }
	}
	return std::accumulate(accs.begin(), accs.end(), f32(0));
    }

    ConstWordLatticeRef getWeightedFramePhoneAccuracy(
	ConstWordLatticeRef lattice,
	ConstWordLatticeRef correct,
	const ShortPauses &shortPauses,
	Speech::AlignmentGeneratorRef alignmentGenerator,
	f32 beta, f32 margin)
    {
	Core::Ref<ApproximateAccuracyAutomaton> a(
	    new WeightedFramePhoneAccuracyAutomaton(lattice, correct, shortPauses, alignmentGenerator, beta, margin));
	return getApproximateAccuracy(a);
    }

} // namespace Lattice
