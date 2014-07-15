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
#include "Lattice.hh"
#include <Fsa/Dfs.hh>
#include <Fsa/Hash.hh>
#include <Fsa/Sort.hh>


using namespace Lattice;

//======================================================================================
bool WordBoundary::Transit::readBinary(Core::BinaryInputStream &bi) {
    u16 tmp;
    bi >> tmp; final = tmp;
    bi >> tmp; initial = tmp;
    return bi;
}

bool WordBoundary::Transit::writeBinary(Core::BinaryOutputStream &bo) const {
    bo << (u16) final << (u16) initial;
    return bo;
}

const WordBoundary WordBoundaries::getFinalWordBoundary() const
{
    Core::Vector<WordBoundary>::const_iterator it = internal_.begin();
    for (; it != internal_.end(); ++ it) {
	if (it->valid()) {
	    break;
	}
    }
    if (it == internal_.end()) {
	return WordBoundary();
    }
    WordBoundary result = *it;
    ++ it;
    for (; it != internal_.end(); ++ it) {
	if (it->valid() and (it->time() > result.time())) {
	    result = *it;
	}
    }
    // final word boundary must be non-coarticulated
    require(result.transit() == WordBoundary::Transit());
    return result;
}

struct WordBoundaries::Header {
    static const char *magic;
    static const size_t magicSize = 8;
    static const u32 version = 3;
};

const char *WordBoundaries::Header::magic = "LATWRDBN";

bool WordBoundaries::writeBinary(Core::BinaryOutputStream &bo) const {
    bo.write(Header::magic, Header::magicSize);
    bo << (u32) Header::version;
    bo << (u32) size();
    for (size_t i = 0; i < size(); ++ i) {
	bo << (u32) time(i);
	transit(i).writeBinary(bo);
    }
    return bo;
}

bool WordBoundaries::readBinary(Core::BinaryInputStream &bi) {
    clear();

    char magic[Header::magicSize];
    u32 version;
    bi.read(magic, Header::magicSize);
    if (!bi) return false;
    if (strncmp(magic, Header::magic, Header::magicSize) != 0) {
	// backward compatibility: old format without any header
#if defined(BISANI)
	version = 1;
#else
	version = 2;
#endif
	bi.seek( - Header::magicSize, std::ios::cur);
    } else {
	if (!(bi >> version)) return false;
    }

    u32 _size;
    if (!(bi >> _size)) return false;
    for (size_t i = 0; i < _size; ++ i) {
	u32 time;
	if (!(bi >> time)) return false;
	WordBoundary::Transit transit;
	if (version >= 2) {
	    if (!transit.readBinary(bi)) return false;
	} else {
	    u32 transitId;
	    bi >> transitId;
	}
	push_back(WordBoundary(time, transit));
    }

    return bi;
}

//======================================================================================
const std::string WordLatticeDescription::defaultName("word-lattice-description");
const std::string WordLatticeDescription::nameModel("model");
const std::string WordLatticeDescription::nameLevel("level");
const std::string WordLatticeDescription::defaultLevel("word");

WordLatticeDescription::WordLatticeDescription(const std::string &name) :
    Precursor(name)
{}

WordLatticeDescription::WordLatticeDescription(const Core::Configurable &parent) :
    Precursor(prepareName(parent.fullName(), defaultName))
{}

WordLatticeDescription::WordLatticeDescription(
    const std::string &name, ConstWordLatticeRef lattice) :
    Precursor(name)
{
    initialize(lattice);
}

WordLatticeDescription::WordLatticeDescription(
    const Core::Configurable &parent, ConstWordLatticeRef lattice) :
    Precursor(prepareName(parent.fullName(), defaultName))
{
    initialize(lattice);
}

void WordLatticeDescription::initialize(ConstWordLatticeRef lattice)
{
    require(lattice);
    for (size_t i = 0; i < lattice->nParts(); ++ i) {
	operator[](i).setValue(Lattice::WordLatticeDescription::nameModel, lattice->name(i));
	operator[](i).setValue(Lattice::WordLatticeDescription::nameLevel, defaultLevel);
    }
}

//======================================================================================
const char* WordLattice::mainFsa = "main";
const char* WordLattice::acousticFsa = "acoustic";
const char* WordLattice::lmFsa = "lm";
const char* WordLattice::accuracyFsa = "accuracy";
const char* WordLattice::totalFsa = "total";
const char* WordLattice::posteriorFsa = "posterior";


WordLattice::WordLattice() {}

class MaximumTimeDfsState : public DfsState
{
private:
    Speech::TimeframeIndex maxTime_;
public:
    MaximumTimeDfsState(
	Fsa::ConstAutomatonRef fsa,
	Core::Ref<const WordBoundaries> wordBoundaries) : maxTime_(0) {
	fsa_ = fsa;
	wordBoundaries_ = wordBoundaries;
    }

    virtual void discoverState(Fsa::ConstStateRef sp) {
	const WordBoundary &wb = (*wordBoundaries_)[sp->id()];
	if (wb.valid()) {
	    maxTime_ = std::max(maxTime_, wb.time());
	}
    }

    Speech::TimeframeIndex getMaximumTime() {
	maxTime_ = 0;
	dfs();
	return maxTime_;
    }
};

Speech::TimeframeIndex  WordLattice::maximumTime() const {
    MaximumTimeDfsState d(part(0), wordBoundaries_);
    return d.getMaximumTime();
}

StandardWordLattice::StandardWordLattice(Core::Ref<const Bliss::Lexicon> lexicon)
{
    parts_.addChoice("acoustic", 0);
    parts_.addChoice("lm", 1);

    acoustic_ = Core::ref(new Fsa::StaticAutomaton);
    acoustic_->setType(Fsa::TypeAcceptor);
    acoustic_->setInputAlphabet(lexicon->lemmaPronunciationAlphabet());
    acoustic_->setSemiring(Fsa::TropicalSemiring);

    lm_ = Core::ref(new Fsa::StaticAutomaton);
    lm_->setType(Fsa::TypeAcceptor);
    lm_->setInputAlphabet(lexicon->lemmaPronunciationAlphabet());
    lm_->setSemiring(Fsa::TropicalSemiring);

    fsas_.push_back(acoustic_);
    fsas_.push_back(lm_);

    wordBoundaries_ = Core::ref(new WordBoundaries);

    initialState_ = acoustic_->newState();
    acoustic_->setInitialStateId(initialState_->id());
    finalState_ = acoustic_->newState();
    acoustic_->setStateFinal(finalState_);

    lm_->setInitialStateId(lm_->newState()->id());
    lm_->setStateFinal(lm_->newState());
}

Fsa::State *StandardWordLattice::newState()
{
    lm_->newState();
    return acoustic_->newState();
}

void StandardWordLattice::newArc(
    Fsa::State *source,
    Fsa::State *target,
    const Bliss::LemmaPronunciation *lemmaPronunciation,
    Speech::Score acoustic, Speech::Score lm)
{
    source->newArc(
	target->id(),
	Fsa::Weight(acoustic),
	(lemmaPronunciation) ? lemmaPronunciation->id() : Fsa::Epsilon);

    lm_->state(source->id())->newArc(
	target->id(),
	Fsa::Weight(lm),
	(lemmaPronunciation) ? lemmaPronunciation->id() : Fsa::Epsilon);
}

void StandardWordLattice::addAcyclicProperty()
{
    if (Fsa::isAcyclic(acoustic_)) {
	acoustic_->setProperties(Fsa::PropertyAcyclic, Fsa::PropertyAcyclic);
	lm_->setProperties(Fsa::PropertyAcyclic, Fsa::PropertyAcyclic);
    } else {
	acoustic_->setProperties(Fsa::PropertyAcyclic, 0);
	lm_->setProperties(Fsa::PropertyAcyclic, 0);
    }
}

namespace Lattice {

    class TimeConditionedWordLattice :
	public Fsa::SlaveAutomaton,
	public Fsa::DfsState
    {
    private:
	typedef std::vector<Fsa::StateId> MergedStates;
	class TimeConditionedState {
	private:
	    WordBoundary wordBoundary_;
	    mutable MergedStates mergedStates_;
	public:
	    TimeConditionedState(WordBoundary wordBoundary) : wordBoundary_(wordBoundary) {}
	    void addState(Fsa::StateId s) const { mergedStates_.push_back(s); }
	    const WordBoundary& wordBoundary() const { return wordBoundary_; }
	    const MergedStates& mergedStates() const { return mergedStates_; }
	};
	struct TimeConditionedStateHash {
	    u32 operator() (const TimeConditionedState &s) const {
		return (2239 * s.wordBoundary().time() + s.wordBoundary().transit().final);
	    }
	};
	struct TimeConditionedStateEqual {
	    bool operator() (const TimeConditionedState &s1, const TimeConditionedState &s2) const {
		return (s1.wordBoundary() == s2.wordBoundary());
	    }
	};
	typedef Fsa::Hash<TimeConditionedState, TimeConditionedStateHash, TimeConditionedStateEqual> States;
	mutable States states_;
	typedef Core::Vector<Speech::TimeframeIndex> StateMap;
	mutable StateMap stateMap_;
	/**
	 * Word boundaries of input word lattice.
	 */
	Core::Ref<const WordBoundaries> wordBoundaries_;
	/**
	 * Word boundaries of time conditioned word lattice.
	 */
	mutable Core::Ref<WordBoundaries> timeConditionedWordBoundaries_;

	struct ByInputAndTarget : public std::binary_function<Fsa::Arc, Fsa::Arc, bool> {
	    bool operator() (const Fsa::Arc &a, const Fsa::Arc &b) const {
		return ((a.input() < b.input()) || ((a.input() == b.input()) && (a.target() < b.target())));
	    }
	};

    public:
	TimeConditionedWordLattice(ConstWordLatticeRef in) :
	    Fsa::SlaveAutomaton(in->part(WordLattice::acousticFsa)),
	    Fsa::DfsState(in->part(WordLattice::acousticFsa)),
	    wordBoundaries_(in->wordBoundaries()),
	    timeConditionedWordBoundaries_(new WordBoundaries) {
	    dfs();
	}
	virtual Fsa::StateId initialStateId() const {
	    // this construciton is necessary to guarantee the existence of the word boundary
	    Fsa::StateId s =
		Fsa::SlaveAutomaton::fsa_->getState(
		    Fsa::SlaveAutomaton::fsa_->initialStateId())->id();
	    return states_.insert(TimeConditionedState((*wordBoundaries_)[s]));
	}
	virtual void discoverState(Fsa::ConstStateRef sp) {
	    States::Cursor s = states_.insert(TimeConditionedState((*wordBoundaries_)[sp->id()]));
	    states_[s].addState(sp->id());
	    stateMap_.grow(sp->id(), 0);
	    stateMap_[sp->id()] = s;
	}
	virtual Fsa::ConstStateRef getState(Fsa::StateId s) const {
	    Fsa::State *sp = new Fsa::State(s);
	    // merge arcs
	    const MergedStates &states = states_[s].mergedStates();
	    for (MergedStates::const_iterator it = states.begin(); it != states.end(); ++it) {
		Fsa::ConstStateRef _sp = Fsa::SlaveAutomaton::fsa_->getState(*it);
		sp->addTags(_sp->tags());
		if (_sp->isFinal()) sp->weight_ = _sp->weight();
		for (Fsa::State::const_iterator a = _sp->begin(); a != _sp->end(); ++a) {
		    Fsa::Arc *_a = sp->newArc();
		    *_a = *a;
		    _a->target_ = stateMap_[_a->target()];
		}
	    }

	    // remove duplicate entries
	    sp->sort(ByInputAndTarget());
	    if (sp->nArcs() > 1) {
		Fsa::State::iterator ai = sp->begin() + 1, ao = sp->begin();
		for (; ai != sp->end(); ++ai) {
		    if ((ai->input() != ao->input()) || (ai->target() != ao->target()))
			*(++ao) = *ai;
		}
		sp->truncate(++ao);
	    }
	    timeConditionedWordBoundaries_->set(s, states_[s].wordBoundary());
	    return Fsa::ConstStateRef(sp);
	}
	virtual std::string describe() const {
	    return Core::form("time-conditioned-word-lattice(%s)", Fsa::SlaveAutomaton::fsa_->describe().c_str());
	}
	virtual size_t getMemoryUsed() const {
	    return Fsa::SlaveAutomaton::fsa_->getMemoryUsed() + states_.getMemoryUsed() + stateMap_.getMemoryUsed();
	}
	Core::Ref<const WordBoundaries> wordBoundaries() const {
	    return timeConditionedWordBoundaries_;
	}
    };
}

ConstWordLatticeRef Lattice::timeConditionedWordLattice(ConstWordLatticeRef in)
{
    Core::Ref<const TimeConditionedWordLattice>f(new TimeConditionedWordLattice(in));
    WordLattice *result = new WordLattice;
    result->setWordBoundaries(f->wordBoundaries());
    result->setFsa(f, WordLattice::acousticFsa);
    return ConstWordLatticeRef(result);
}
