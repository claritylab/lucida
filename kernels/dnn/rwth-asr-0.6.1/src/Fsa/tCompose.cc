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
#include <Core/Assertions.hh>
#include "tAlphabet.hh"
#include "tBasic.hh"
#include "tCache.hh"
#include "tCompose.hh"
#include "tProject.hh"
#include "tRational.hh"
#include "tSort.hh"
#include "Alphabet.hh"
#include "Hash.hh"
#include "Stack.hh"
#include "Utility.hh"
#include <Core/Vector.hh>


namespace Ftl {
    /**
     * Composition result of two transducers.
     */

    template<class _Automaton>
    class ComposeAutomaton : public _Automaton {
    public:
	virtual ~ComposeAutomaton() {}

	/** State in the left original automaton.
	 * Given a state of the composition result, return the
	 * corresponding original state of the left automaton. */
	virtual Fsa::StateId leftStateId(Fsa::StateId) const = 0;

	/** State in the right original automaton.
	 * Given a state of the composition result, return the
	 * corresponding original state of the right automaton. */
	virtual Fsa::StateId rightStateId(Fsa::StateId) const = 0;
    };

    template<class _Automaton>
    class GeneralComposeAutomaton : public ComposeAutomaton<_Automaton> {
    private:
	typedef ComposeAutomaton<_Automaton> Precursor;

    public:
	typedef typename _Automaton::State _State;
	typedef typename _Automaton::ConstStateRef _ConstStateRef;
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
	typedef typename _Automaton::ConstSemiringRef _ConstSemiringRef;

    protected:
	_ConstAutomatonRef fl_, fr_;

	// unused state id bits of left state hold bits of filter state
	struct StateHashKey_;
	class State_ {
	private:
	    friend struct StateHashKey_;
	    Fsa::StateId l_;
	    Fsa::StateId r_;
	public:
	    State_(Fsa::StateId l, Fsa::StateId f, Fsa::StateId r) : l_(l | (f << Fsa::StateIdBits)), r_(r) {}
	    Fsa::StateId l() const { return l_ & Fsa::StateIdMask; }
	    Fsa::StateId f() const { return l_ >> Fsa::StateIdBits; }
	    Fsa::StateId r() const { return r_; }
	    bool operator== (const State_ &s) const { return (l_ == s.l_) && (r_ == s.r_); }
	};
	struct StateHashKey_ {
	    u32 operator() (const State_ &s) { return 2239 * (size_t)s.l_ + (size_t)s.r_; }
	};
	typedef Fsa::Hash<State_, StateHashKey_> States;
	mutable States states_;


#ifdef STRING_POTENTIALS
	mutable Fsa::LabelIdStrings stringPotentials_;
	Fsa::LabelIdStrings::Id emptyStringPotential_;
	mutable Core::Vector<Fsa::LabelIdStrings::Id> stringPotentialsLeft_, stringPotentialsRight_;

	void stringPotential
	(_ConstAutomatonRef f, Fsa::StateId s, Core::Vector<Fsa::LabelIdStrings::Id> &potentials, bool input) const {
	    // cache potentials
	    //std::cout << "state: " << s << std::endl;
	    if (s >= potentials.size()) potentials.grow(s, Fsa::LabelIdStrings::Invalid);
	    if (potentials[s] != Fsa::LabelIdStrings::Invalid) return;

	    // start with s being the first state to explore
	    Fsa::Stack<Fsa::StateId> states_;
	    states_.push(s);
	    typedef Core::Vector<std::pair<Fsa::StateId, Fsa::LabelIdStrings::Id> > StatePotentials;
	    StatePotentials potentials_;
	    Fsa::LabelIdStrings::Id start = stringPotentials_.start();

	    // we may encounter a trap (no final state) => don't loop infinitely
	    while (!states_.empty()) {
		Fsa::Stack<Fsa::StateId> newStates_;
		// as long as we have only a single state to inspect we'll also store its potential
		if (states_.size() == 1)
		    potentials_.push_back(std::make_pair(states_.top(), stringPotentials_.start() - start));
		Fsa::LabelId label = Fsa::InvalidLabelId;

		// check wether all arcs leaving states have the same label (stack of states is empty if finished)
		while (!states_.empty()) {
		    //std::cout << "inspect: " << states_.top() << std::endl;
		    _ConstStateRef sp = f->getState(states_.pop());
		    if (sp->isFinal()) {
		      FinalOrNoMatchingLabels:
			start = stringPotentials_.stop(start);
			Fsa::StateId maxStateId = 0;
			for (StatePotentials::const_iterator p = potentials_.begin(); p != potentials_.end(); ++p)
			    maxStateId = std::max(maxStateId, p->first);
			if (maxStateId >= potentials.size()) potentials.grow(maxStateId, Fsa::LabelIdStrings::Invalid);
			for (StatePotentials::const_iterator p = potentials_.begin(); p != potentials_.end(); ++p) {
			    //std::cout << "store: " << p->first << "(" << start + p->second << ")" << std::endl;
			    potentials[p->first] = stringPotentials_.insert(start + p->second);
			}
			return;
		    }
		    for (typename _State::const_iterator a = sp->begin(); a != sp->end(); ++a) {
			Fsa::LabelId newLabel = input ? a->input() : a->output();
			if (newLabel != Fsa::Epsilon) {
			    newStates_.push(a->target()); // does non-sense in goto-case
			    if (label == newLabel) newStates_.push(a->target());
			    else if (label == Fsa::InvalidLabelId) label = newLabel;
			    else goto FinalOrNoMatchingLabels;
			} else states_.push(a->target());
		    }
		}
		stringPotentials_.append(label);
		std::swap(states_, newStates_);
	    }

	    // in case of a trap we set potentials to empty strings
	    stringPotentials_.discard(start);
	    Fsa::StateId maxStateId = 0;
	    for (StatePotentials::const_iterator p = potentials_.begin(); p != potentials_.end(); ++p)
		maxStateId = std::max(maxStateId, p->first);
	    if (maxStateId >= potentials.size()) potentials.grow(maxStateId, Fsa::LabelIdStrings::Invalid);
	    for (StatePotentials::const_iterator p = potentials_.begin(); p != potentials_.end(); ++p) {
		//std::cout << "store: " << p->first << std::endl;
		potentials[p->first] = emptyStringPotential_;
	    }
	}
	Fsa::LabelIdStrings::Id stringPotentialLeft(Fsa::StateId s) const {
	    //std::cout << "left" << std::endl;
	    stringPotential(fl_, s, stringPotentialsLeft_, false);
	    return stringPotentialsLeft_[s];
	}
	Fsa::LabelIdStrings::Id stringPotentialRight(Fsa::StateId s) const {
	    //std::cout << "right" << std::endl;
	    stringPotential(fr_, s, stringPotentialsRight_, true);
	    return stringPotentialsRight_[s];
	}
	bool areStringPotentialsPrefixes(Fsa::StateId sl, Fsa::StateId sr) const {
	    return true;
	    Fsa::LabelIdStrings::Id ll = stringPotentialLeft(sl), lr = stringPotentialRight(sr);
	    if (ll == lr) return true;
	    Fsa::LabelIdStrings::const_iterator il = stringPotentials_.begin(ll), ir = stringPotentials_.begin(lr);
	    for (; *il == *ir; ++il, ++ir)
	    return ((*il == Fsa::InvalidLabelId) || (*ir == Fsa::InvalidLabelId));
	}
#endif

    public:
	GeneralComposeAutomaton(_ConstAutomatonRef fl, _ConstAutomatonRef fr, bool reportUnknowns) {
	    if (fl->semiring() != fr->semiring()) std::cerr << "inconsistent semirings for compose" << std::endl;
	    fl_ = cache<_Automaton>(sort<_Automaton>(mapOutput<_Automaton>(fl, fr->getInputAlphabet(), reportUnknowns ? 10 : 0), Fsa::SortTypeByOutput), 10000);
	    fr_ = cache<_Automaton>(sort<_Automaton>(fr, Fsa::SortTypeByInput), 10000);
	    this->setProperties(Fsa::PropertyStorage | Fsa::PropertyCached, Fsa::PropertyNone);
	    if ((fl_->hasProperty(Fsa::PropertyAcyclic)) || (fr_->hasProperty(Fsa::PropertyAcyclic)))
		this->addProperties(Fsa::PropertyAcyclic);
	    this->unsetProperties(Fsa::PropertySorted);
#ifdef STRING_POTENTIALS
	    emptyStringPotential_ = stringPotentials_.stop(stringPotentials_.start());
#endif
	}
	virtual ~GeneralComposeAutomaton() {}

	virtual Fsa::Type type() const {
	    if ((fl_->type() == Fsa::TypeAcceptor) && (fr_->type() == Fsa::TypeAcceptor))
		return Fsa::TypeAcceptor; // intersection
	    return Fsa::TypeTransducer;
	}
	virtual _ConstSemiringRef semiring() const { return fl_->semiring(); }
	virtual Fsa::StateId initialStateId() const {
	    Fsa::StateId leftInitial = fl_->initialStateId();
	    if (leftInitial == Fsa::InvalidStateId) return Fsa::InvalidStateId;
	    Fsa::StateId rightInitial = fr_->initialStateId();
	    if (rightInitial == Fsa::InvalidStateId) return Fsa::InvalidStateId;
	    return insertState(leftInitial, 0, rightInitial);
	}
	virtual Fsa::ConstAlphabetRef getInputAlphabet() const {
	    if (type() == Fsa::TypeAcceptor) return fr_->getInputAlphabet();
	    return fl_->getInputAlphabet();
	}
	virtual Fsa::ConstAlphabetRef getOutputAlphabet() const {
	    if (type() == Fsa::TypeAcceptor) return fl_->getInputAlphabet();
	    return fr_->getOutputAlphabet();
	}

	virtual Fsa::StateId leftStateId(Fsa::StateId s) const {
	    return states_[s].l();
	}

	virtual Fsa::StateId rightStateId(Fsa::StateId s) const {
	    return states_[s].r();
	}

	Fsa::StateId insertState(Fsa::StateId l, Fsa::StateId f, Fsa::StateId r) const {
	    Fsa::StateId id = states_.insert(State_(l, f, r));
	    if (id > Fsa::StateIdMask) std::cerr << "compose: out of state ids" << std::endl;
	    return id;
	}

	void composeArcs(_State *sp, _ConstStateRef sl, _ConstStateRef sr,
			 typename _State::const_iterator al, typename _State::const_iterator ar) const {
	    // the following factor of 2 comparison is heuristic, but tested on a larger set of automata
	    if (((sl->nArcs() << 2) < sr->nArcs()) || (sl->nArcs() > (sr->nArcs() << 2))) {
		for (; (al < sl->end()) && (ar < sr->end());) {
		    if ((al->output() > Fsa::LastLabelId) || (ar->input() > Fsa::LastLabelId)) break;
		    if (al->output() == ar->input()) {
			for (typename _State::const_iterator a = ar; ((a < sr->end()) && (al->output() == a->input())); ++a)
#ifdef STRING_POTENTIALS
			    if (areStringPotentialsPrefixes(al->target(), a->target()))
#endif
				sp->newArc(insertState(al->target(), 0, a->target()),
					   semiring()->extend(al->weight(), a->weight()), al->input(), a->output());
			++al;
		    } else if (al->output() > ar->input()) {
			u32 left = 0, right = sr->end() - ar;
			while (right - left > 1) {
			    u32 i = (left + right) >> 1;
			    if (al->output() > (ar + i)->input()) left = i;
			    else right = i;
			}
			ar += right;
		    } else {
			u32 left = 0, right = sl->end() - al;
			while (right - left > 1) {
			    u32 i = (left + right) >> 1;
			    if ((al + i)->output() < ar->input()) left = i;
			    else right = i;
			}
			al += right;
		    }
		}
	    } else {
		for (; (al < sl->end()) && (ar < sr->end()); ++al) {
		    if ((al->output() > Fsa::LastLabelId) || (ar->input() > Fsa::LastLabelId)) break;
		    if (al->output() >= ar->input()) {
			for (; ((ar < sr->end()) && (al->output() > ar->input())); ++ar);
			for (typename _State::const_iterator a = ar; ((a < sr->end()) && (al->output() == a->input())); ++a)
#ifdef STRING_POTENTIALS
			    if (areStringPotentialsPrefixes(al->target(), a->target()))
#endif
				sp->newArc(insertState(al->target(), 0, a->target()),
					   semiring()->extend(al->weight(), a->weight()), al->input(), a->output());
		    }
		}
	    }
	    Fsa::StateTag tags = sl->tags() & sr->tags();
	    if (tags & Fsa::StateTagFinal) sp->weight_ = semiring()->extend(sl->weight_, sr->weight_);
	    sp->setTags(tags);
	}

	void composeSpecialArcs(_State *sp, _ConstStateRef sl, _ConstStateRef sr) const {
	    Fsa::LabelId label;
	    typename _State::const_iterator al, ar;


	    for (typename _State::const_reverse_iterator sar = sr->rbegin();
		 (sar !=  sr->rend()) && (sar->input() > Fsa::LastLabelId); ++sar) {
		label = sar->output();
		switch (sar->input()) {
		case Fsa::Any:
		    for (al = sl->begin(); al < sl->end(); ++al) {
			//if (type() == Fsa::TypeAcceptor) label = al->input();
			if (fr_->type() == Fsa::TypeAcceptor) label = al->output();
			sp->newArc(insertState(al->target(), 0, sar->target()),
				   semiring()->extend(al->weight(), sar->weight()), al->input(), label);
		    }
		    break;
		case Fsa::Failure:
		    for (al = sl->begin(), ar = sr->begin(); (al < sl->end()) && (ar < sr->end()); ++ar) {
			if (al->output() > Fsa::LastLabelId) break;
			if (ar->input() >= al->output())
			    for (; ((al < sl->end()) && (ar->input() == al->output())); ++al);
			if (ar->input() > al->output()) {
			    if (type() == Fsa::TypeAcceptor) label = Fsa::Failure;
			    sp->newArc(insertState(sl->id(), 0 ,sar->target()),
				       semiring()->extend(al->weight(), sar->weight()), Fsa::Failure, label);
			}
		    }
		    break;
		case Fsa::Else:
		    for (al = sl->begin(), ar = sr->begin(); (al < sl->end()) && (ar < sr->end()); ++ar) {
			if (al->output() == ar->input())
			    for (; ((al < sl->end()) && (al->output() == ar->input())); ++al);
			if ((al < sl->end()) && (al->output() < ar->input())) {
			    if (type() == Fsa::TypeAcceptor) label = Fsa::Else;
			    sp->newArc(insertState(sl->id(), 0, sar->target()),
				       semiring()->extend(al->weight(), sar->weight()), Fsa::Failure, label);
			    break;
			}
			if ((al->output() > Fsa::LastLabelId) || (ar->input() > Fsa::LastLabelId)) break;
		    }
		    break;
		default:
		    break;
		}
	    }
	}

	virtual void dumpState(Fsa::StateId s, std::ostream &o) const {
	    if (s < states_.size()) {
		o << "(";
		fl_->dumpState(states_[s].l(), o);
		o << ", " << states_[s].f() << ", ";
		//if (states_[s].f() != 0)
		//else o << ",";
		fr_->dumpState(states_[s].r(), o);
		o << ")";
	    } else o << "unknown";
	}
	virtual size_t getMemoryUsed() const {
	    return fl_->getMemoryUsed() + fr_->getMemoryUsed() +
		2 * sizeof(_ConstAutomatonRef) + 4 * sizeof(Fsa::LabelId) + states_.getMemoryUsed()
#ifdef STRING_POTENTIALS
		+ stringPotentials_.getMemoryUsed() + stringPotentialsLeft_.getMemoryUsed()
		+ stringPotentialsRight_.getMemoryUsed()
#endif
		;
	}
	virtual void dumpMemoryUsage(Core::XmlWriter &o) const {
	    o << Core::XmlOpen("compose");
	    fl_->dumpMemoryUsage(o);
	    fr_->dumpMemoryUsage(o);
	    o << Core::XmlFull("states", states_.getMemoryUsed()) << Core::XmlClose("compose");
	}
    };

    template<class _Automaton>
    class ComposeMatchingAutomaton : public GeneralComposeAutomaton<_Automaton> {
    private:
	typedef GeneralComposeAutomaton<_Automaton> Precursor;

    public:
	typedef typename _Automaton::State _State;
	typedef typename _Automaton::ConstStateRef _ConstStateRef;
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;

    public:
	ComposeMatchingAutomaton(_ConstAutomatonRef fl, _ConstAutomatonRef fr, bool reportUnknowns = true) :
	    Precursor(fl, fr, reportUnknowns) {}
	virtual ~ComposeMatchingAutomaton() {}

	virtual _ConstStateRef getState(Fsa::StateId s) const {
	    if (s < Precursor::states_.size()) {
		Fsa::StateId l = Precursor::states_[s].l(), r = Precursor::states_[s].r();
		_ConstStateRef sl = Precursor::fl_->getState(l), sr = Precursor::fr_->getState(r);
		typename _State::const_iterator al = sl->begin(), ar = sr->begin();

		_State *sp = new _State(s);
		switch (Precursor::states_[s].f()) {
		case 0:
		    if ((al != sl->end()) && (al->output() == Fsa::Epsilon)) {
			for (; (ar != sr->end()) && (ar->input() == Fsa::Epsilon); ++ar);
			if ((ar != sr->end()) || (sr->isFinal()))
			    for (; (al != sl->end()) && (al->output() == Fsa::Epsilon); ++al)
				sp->newArc(Precursor::insertState(al->target(), 1, r),
					   this->semiring()->extend(al->weight(), Precursor::fr_->semiring()->one()),
					   al->input(), Fsa::Epsilon);
			al = sl->begin();
			ar = sr->begin();
		    }
		    if ((ar != sr->end()) && (ar->input() == Fsa::Epsilon)) {
			for (; (al != sl->end()) && (al->output() == Fsa::Epsilon); ++al);
			if ((al != sl->end()) || (sl->isFinal()))
			    for (; (ar != sr->end()) && (ar->input() == Fsa::Epsilon); ++ar)
				sp->newArc(Precursor::insertState(l, 2, ar->target()),
					   this->semiring()->extend(Precursor::fl_->semiring()->one(), ar->weight()),
					   Fsa::Epsilon, ar->output());
			al = sl->begin();
			ar = sr->begin();
		    }
		    break;
		case 1:
		    for (; (al != sl->end()) && (al->output() == Fsa::Epsilon); ++al)
			sp->newArc(Precursor::insertState(al->target(), 1, r),
				   this->semiring()->extend(al->weight(), Precursor::fr_->semiring()->one()),
				   al->input(), Fsa::Epsilon);
		    break;
		case 2:
		    for (; (ar != sr->end()) && (ar->input() == Fsa::Epsilon); ++ar)
			sp->newArc(Precursor::insertState(l, 2, ar->target()),
				   this->semiring()->extend(Precursor::fl_->semiring()->one(), ar->weight()),
				   Fsa::Epsilon, ar->output());
		    break;
		default:
		    break;
		}
		Precursor::composeArcs(sp, sl, sr, al, ar);
		Precursor::composeSpecialArcs(sp, sl, sr);
		return _ConstStateRef(sp);
	    }
	    return _ConstStateRef();
	}
	virtual std::string describe() const {
	    return Core::form("composeMatching(%s,%s)", Precursor::fl_->describe().c_str(), Precursor::fr_->describe().c_str());
	}
    };

    template<class _Automaton>
    typename _Automaton::ConstRef composeMatching
    (typename _Automaton::ConstRef fl, typename _Automaton::ConstRef fr, bool reportUnknowns) {
	return typename _Automaton::ConstRef(new ComposeMatchingAutomaton<_Automaton>(fl, fr, reportUnknowns));
    }

    template<class _Automaton>
    typename _Automaton::ConstRef difference
    (typename _Automaton::ConstRef fl, typename _Automaton::ConstRef fr) {
	return trim<_Automaton>(composeMatching<_Automaton>(fl, complement<_Automaton>(fr)));
    }


    template<class _Automaton>
    class ComposeSequencingAutomaton : public GeneralComposeAutomaton<_Automaton> {
    private:
	typedef GeneralComposeAutomaton<_Automaton> Precursor;

    public:
	typedef typename _Automaton::State _State;
	typedef typename _Automaton::ConstStateRef _ConstStateRef;
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;

    public:
	ComposeSequencingAutomaton(_ConstAutomatonRef fl, _ConstAutomatonRef fr, bool reportUnknowns = true) :
	    Precursor(fl, fr, reportUnknowns) {}
	virtual ~ComposeSequencingAutomaton() {}

	virtual _ConstStateRef getState(Fsa::StateId s) const {
	    if (s < Precursor::states_.size()) {
		Fsa::StateId l = Precursor::states_[s].l(), r = Precursor::states_[s].r();
		_ConstStateRef sl = Precursor::fl_->getState(l), sr = Precursor::fr_->getState(r);
		typename _State::const_iterator al = sl->begin(), ar = sr->begin();

		_State *sp = new _State(s);
		switch (Precursor::states_[s].f()) {
		case 0:
		    for (; (al != sl->end()) && (al->output() == Fsa::Epsilon); ++al)
#ifdef STRING_POTENTIALS
			if (areStringPotentialsPrefixes(al->target(), r))
#endif
			    sp->newArc(Precursor::insertState(al->target(), 0, r),
				       this->semiring()->extend(al->weight(), Precursor::fr_->semiring()->one()),
				       al->input(), Fsa::Epsilon);
		    // fall-through
		case 1:
		    for (; (ar != sr->end()) && (ar->input() == Fsa::Epsilon); ++ar)
#ifdef STRING_POTENTIALS
			if (areStringPotentialsPrefixes(l, ar->target()))
#endif
			    sp->newArc(Precursor::insertState(l, 1, ar->target()),
				       this->semiring()->extend(Precursor::fl_->semiring()->one(), ar->weight()),
				       Fsa::Epsilon, ar->output());
		    break;
		default:
		    break;
		}
		Precursor::composeArcs(sp, sl, sr, al, ar);
		Precursor::composeSpecialArcs(sp, sl, sr);
		return _ConstStateRef(sp);
	    }
	    return _ConstStateRef();
	}
	virtual std::string describe() const {
	    return Core::form("composeSequencing(%s,%s)", Precursor::fl_->describe().c_str(), Precursor::fr_->describe().c_str());
	}
    };

    template<class _Automaton>
    typename _Automaton::ConstRef composeSequencing
    (typename _Automaton::ConstRef fl, typename _Automaton::ConstRef fr, bool reportUnknowns) {
	return typename _Automaton::ConstRef(new ComposeSequencingAutomaton<_Automaton>(fl, fr, reportUnknowns));
    }



    template<class _Automaton>
    class ComposeMapping : public Fsa::Mapping {
    public:
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
	typedef ComposeAutomaton<_Automaton> _ComposeAutomaton;
    protected:
	// weak reference!
	//	_ConstAutomatonRef fsa_;
	const _ComposeAutomaton *cFsa_;
    public:
	ComposeMapping(_ConstAutomatonRef f) : cFsa_(dynamic_cast<const _ComposeAutomaton*>(f.get())) {
	    require(cFsa_);
	}
	virtual ~ComposeMapping() {}
    };

    template<class _Automaton>
    class LeftComposeMapping : public ComposeMapping<_Automaton> {
	typedef ComposeMapping<_Automaton> Precursor;
    public:
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
    public:
	LeftComposeMapping(_ConstAutomatonRef f) : Precursor(f) {}
	virtual ~LeftComposeMapping() {}
	virtual Fsa::StateId map(Fsa::StateId target) const { return Precursor::cFsa_->leftStateId(target); }
    };

    template<class _Automaton>
    Fsa::ConstMappingRef mapToLeft(typename _Automaton::ConstRef f) {
	return Fsa::ConstMappingRef(new LeftComposeMapping<_Automaton>(f));
    }


    template<class _Automaton>
    class RightComposeMapping : public ComposeMapping<_Automaton> {
	typedef ComposeMapping<_Automaton> Precursor;
    public:
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
    public:
	RightComposeMapping(_ConstAutomatonRef f) : Precursor(f) {}
	virtual ~RightComposeMapping() {}
	virtual Fsa::StateId map(Fsa::StateId target) const { return Precursor::cFsa_->rightStateId(target); }
    };

    template<class _Automaton>
    Fsa::ConstMappingRef mapToRight(typename _Automaton::ConstRef f) {
	return Fsa::ConstMappingRef(new RightComposeMapping<_Automaton>(f));
    }
} // namespace Ftl
