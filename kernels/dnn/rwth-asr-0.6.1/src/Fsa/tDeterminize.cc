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
#include "Hash.hh"
#include "Stack.hh"
#include "Utility.hh"
#include <Core/Vector.hh>
#include "tAutomaton.hh"
#include "tCache.hh"
#include "tDeterminize.hh"
#include "tSort.hh"
#include <Core/Assertions.hh>

namespace Ftl {

    /*
     * possible optimizations:
     * - we always split/merge sets of substates => special representation for split/merge? [memory]
     * - use specialized hash table for output strings; we waste some memory by using a separate hash table [memory]
     * - modify state hash function to also incorporate state output label [speed]
     *
     * done:
     * - find memory leakage [memory]
     * - optimize storage for intermediate arcs [speed and slightly memory]
     * - use InvalidLabelId as end marker for strings => discard length field [speed]
     * - store string start within substate and state [speed]
     * - compress substate sequences: gzip gives more than a factor of 4 on these huge vectors [memory]
     */
    template<class _Automaton>
    class DeterminizeAutomaton : public _Automaton {
    private:
	typedef DeterminizeAutomaton<_Automaton> Self;
	typedef _Automaton Precursor;

    public:
	typedef typename _Automaton::Weight Weight;
	typedef typename _Automaton::Arc _Arc;
	typedef typename _Automaton::State _State;
	typedef typename _Automaton::ConstStateRef _ConstStateRef;
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
	typedef typename _Automaton::ConstSemiringRef _ConstSemiringRef;

    private:
	_ConstAutomatonRef fsa_;
	_ConstSemiringRef semiring_;
	bool disambiguate_;

	// substate:
	// - sequences of substates do not occur twice due to hashing of states
	// - hashed sequences of substates are sorted by state and output
	struct Substate {
	    typedef u32 Cursor;
	    static const Cursor NoPredecessor = 0x07fffffff;

	    Cursor predecessor_;
	    Fsa::StateId state_;
	    Fsa::LabelIdStrings::Id output_; // hash index
	    Weight weight_;
	    bool disconnect_;
	    Substate() {}
	    Substate(Fsa::StateId state, Fsa::LabelIdStrings::Id output, Weight weight) :
		predecessor_(NoPredecessor), state_(state), output_(output), weight_(weight), disconnect_(false) {}
	    Substate(Cursor predecessor, Fsa::StateId state, Fsa::LabelIdStrings::Id output, Weight weight) :
		predecessor_(predecessor), state_(state), output_(output), weight_(weight), disconnect_(false) {}
	    bool operator< (const Substate &s) const {
		return (state_ < s.state_) || ((state_ == s.state_) && (output_ < s.output_));
	    }
	};

	class Substates {
	public:
	    typedef typename Substate::Cursor Cursor;
	    static const Cursor Overflow = 0x80000000;
	    static const Cursor NoPredecessor = 0x07fffffff;
	    typedef const Cursor* const_predecessor_iterator;

	private:
	    // compressed substates format:
	    // cursor (hash-link to next set of substates)
	    // partition byte
	    // substate data
	    // partition byte
	    // substate data
	    // ...
	    //
	    // partition byte format (ABBCCCDD):
	    // A     wether its predecessor is set or not (due to garbage collection)
	    // BB    # of bits for state: 00 = 8, 01 = 16, 10 = 24, 11 = 32
	    // CCC   # of bits for output string: 000 = 0, 001 = 8, 010 = 16, 011 = 24, 100 = 32
	    // DD    weight: 00 = default (one()), 01 = individual, 10 = weight equal to previous substate,
	    //       11 = UNUSED => ensures that 0xff marks the end of a substate sequence
	    // 0xff  end of substate sequence
	    u32 nSubstates_;
	    Core::Vector<Cursor> bins_;
	    Core::Vector<u8> substates_;
	    _ConstSemiringRef semiring_;
	    Core::Vector<Core::Vector<Cursor> > predecessors_;

	public:
	    class const_iterator {
	    private:
		friend class Substates;
		typedef const_iterator Self;
		const Substates &container_;
		Cursor current_, skip_;
		Substate substate_;
		const_iterator(const Substates &container, Cursor i) : container_(container), current_(i) {
		    substate_.weight_ = container_.semiring_->one();
		    uncompress();
		}
		void uncompress() {
		    Core::Vector<u8>::const_iterator tmp = container_.substates_.begin() + current_;
		    u8 partition = *tmp;
		    if (partition != 0xff) {
			++tmp;
			if (partition & 0x80) substate_.predecessor_ = Fsa::getBytesAndIncrement(tmp, sizeof(Cursor));
			else substate_.predecessor_ = NoPredecessor;
			Fsa::StateId state = Fsa::getBytesAndIncrement(tmp, ((partition >> 5) & 0x03) + 1);
			substate_.state_ = state >> 1;
			substate_.disconnect_ = (state & 0x1);
			substate_.output_ = Fsa::getBytesAndIncrement(tmp, (partition >> 2) & 0x07);
			switch (partition & 0x03) {
			case 0x00:
			    substate_.weight_ = container_.semiring_->one();
			    break;
			case 0x01:
			    substate_.weight_ = container_.semiring_->uncompress(tmp);
			    break;
			case 0x02:
			    break;
			}
		    }
		    skip_ = tmp - container_.substates_.begin() - current_;
		}
	    public:
		bool valid() const { return (container_.substates_[current_] != 0xff); }
		typename Substate::Cursor cursor() const { return current_; }
		Self& operator++() {
		    current_ += skip_;
		    uncompress();
		    return *this;
		}
		const Substate* operator->() const { return &substate_; }
		const Substate& operator*() const { return substate_; }
		bool operator != (const const_iterator i) const { return current_ != i.current_; }
	    };

	public:
	    Substates(_ConstSemiringRef semiring) :
		nSubstates_(0),
		bins_(13, Core::Type<Cursor>::max),
		semiring_(semiring)
	    {}

	    size_t nExternalPredecessors() const { return predecessors_.size(); }
	    void resizeExternalPrecessors(size_t size) { predecessors_.resize(size); }
	    const_predecessor_iterator predecessorBegin(Cursor p) const {
		return &predecessors_[p][0];
	    }
	    const_predecessor_iterator predecessorEnd(Cursor p) const {
		return &predecessors_[p][predecessors_[p].size() - 1];
	    }
	    Cursor addPredecessor(Cursor p1, Cursor p2) {
		//if (p2 == None) return p1; // does not occur
		if (p1 == NoPredecessor) return p2;
		if (!(p1 & Overflow)) {
		    predecessors_.push_back(Core::Vector<Cursor>(1, p1));
		    p1 = predecessors_.size() - 1;
		}
		if (!(p2 & Overflow)) predecessors_[p1].push_back(p2);
		else predecessors_[p1].insert
		    (predecessors_[p1].end(), predecessors_[p2 & ~Overflow].begin(),
		     predecessors_[p2 & ~Overflow].end());
		return (p1 | Overflow);
	    }

	    Cursor size(u8 partition) const {
		if (partition == 0xff) return 0;
		return (partition & 0x80 ? sizeof(Cursor) : 0) + ((partition >> 5) & 0x03) + 1 +
		    ((partition >> 2) & 0x07) + (partition & 0x01 ? semiring_->compressedSize() : 0);
	    }
	    Fsa::StateId state(Cursor pos) const {
		Core::Vector<u8>::const_iterator tmp = substates_.begin() + pos;
		u8 partition = *(tmp++);
		if (partition & 0x80) tmp += sizeof(Cursor);
		return Fsa::getBytesAndIncrement(tmp, ((partition >> 5) & 0x03) + 1) >> 1;
	    }
	    Cursor predecessor(Cursor pos) const {
		Core::Vector<u8>::const_iterator tmp = substates_.begin() + pos;
		u8 partition = *(tmp++);
		Cursor predecessor = NoPredecessor;
		if (partition & 0x80) predecessor = Fsa::getBytesAndIncrement(tmp, sizeof(Cursor));
		return predecessor;
	    }
	    u32 hash(Cursor pos) const {
		u32 value = 0;
		for (Core::Vector<u8>::const_iterator i = substates_.begin() + pos + sizeof(Cursor); *i != 0xff;) {
		    Core::Vector<u8>::const_iterator end = i + 1 + size(*i);
		    for (; i != end; ++i) value = 337 * value + *i;
		}
		return value;
	    }
	    bool equal(Cursor pos1, Cursor pos2) const {
		/*! \todo compare without the user state tags = cycle indicators */
		Core::Vector<u8>::const_iterator i1 = substates_.begin() + pos1 + sizeof(Cursor);
		Core::Vector<u8>::const_iterator i2 = substates_.begin() + pos2 + sizeof(Cursor);
		for (; *i1 != 0xff;) {
		    u8 partition = *(i1++);
		    if (partition != *(i2)) return false;
		    require(*i2 != 0xff);
		    ++i2;
		    Cursor len = size(partition);
		    if (memcmp(&*i1, &*i2, len) != 0) return false;
		    i1 += len;
		    i2 += len;
		}
		if (*i2 != 0xff) return false;
		return true;
	    }
	    std::pair<Cursor, bool> insert(const Core::Vector<Substate> &v) {
		Cursor start = substates_.size();
		Fsa::appendBytes(substates_, 0, sizeof(Cursor));
		Weight previous = semiring_->one();
		for (typename Core::Vector<Substate>::const_iterator s = v.begin(); s != v.end(); ++s) {
		    u8 partition = (s->predecessor_ != NoPredecessor ? 0x80 : 0x00);
		    Fsa::StateId state = (s->state_ << 1) | (s->disconnect_ ? 1 : 0);
		    partition |= (Fsa::estimateBytes(state) - 1) << 5;
		    if (s->output_ > 0) partition |= Fsa::estimateBytes(s->output_) << 2;
		    if (previous == s->weight_) partition |= 0x02;
		    else if (!semiring_->isDefault(s->weight_)) partition |= 0x01;
		    substates_.push_back(partition);
		    if (partition & 0x80) Fsa::appendBytes(substates_, s->predecessor_, sizeof(Cursor));
		    Fsa::appendBytes(substates_, state, ((partition >> 5) & 0x03) + 1);
		    Fsa::appendBytes(substates_, s->output_, (partition >> 2) & 0x07);
		    if (partition & 0x01) semiring_->compress(substates_, s->weight_);
		    previous = s->weight_;
		}
		substates_.push_back(0xff);

		// get hash value
		u32 key = hash(start);
		Cursor i = bins_[key % bins_.size()];
		for (; (i != Core::Type<Cursor>::max) && (!equal(start, i));
		     i = Cursor(Fsa::getBytes(substates_.begin() + i, sizeof(Cursor))));

		// if existing: resize vector, else: add to hash table
		if (i != Core::Type<Cursor>::max) {
		    substates_.resize(start);
		    return std::make_pair(i, true);
		}

		// resize hash table on demand
		if (nSubstates_ > 2 * bins_.size()) {
		    std::fill(bins_.begin(), bins_.end(), Core::Type<Cursor>::max);
		    bins_.grow(2 * bins_.size() - 1, Core::Type<Cursor>::max);
		    for (Cursor i = 0; i != start; ++i) {
			u32 key = hash(i) % bins_.size();
			Fsa::setBytes(substates_.begin() + i, bins_[key], sizeof(Cursor));
			bins_[key] = i;
			for (i += sizeof(Cursor); substates_[i] != 0xff; i += 1 + size(substates_[i]));
		    }
		}
		Fsa::setBytes(substates_.begin() + start, bins_[key % bins_.size()], sizeof(Cursor));
		bins_[key % bins_.size()] = start;
		++nSubstates_;
		return std::make_pair(start, false);
	    }
	    const u8* begin() const { return &substates_[0]; }
	    size_t size() const { return substates_.size(); }
	    const_iterator begin(Cursor start) const { return const_iterator(*this, start + sizeof(Cursor)); }
	    size_t getMemoryUsed() const {
		return bins_.getMemoryUsed() + substates_.getMemoryUsed() + sizeof(typename Precursor::ConstSemiringRef);
	    }
	    size_t getPredecessorMemoryUsed() const {
		size_t predecessors = 0;
		for (typename Core::Vector<Core::Vector<Cursor> >::const_iterator i = predecessors_.begin();
		     i != predecessors_.end(); ++i)
		    predecessors += i->getMemoryUsed();
		return predecessors;
	    }

	};
	mutable Substates substates_;
	mutable Core::Vector<Substate> tmpSubstates_;

	// state
	struct State_ {
	    typename Substates::Cursor subset_; // state label: subset of state and weights
	    Fsa::LabelIdStrings::Id output_; // hash index
	    State_(typename Substates::Cursor subset, Fsa::LabelIdStrings::Id output) : subset_(subset), output_(output) {}
	};
	struct StateHashKey_ {
	    const Self &d_;
	    StateHashKey_(Self &d) : d_(d) {}
	    u32 operator() (const State_ &s) {
		u32 value = 0;
		for (typename Substates::const_iterator sub = d_.substates_.begin(s.subset_); sub.valid(); ++sub)
		    value = 337 * value + 2239 * sub->output_ + sub->state_;
		return value;
	    }
	};
	struct StateHashEqual_ {
	    const Self &d_;
	    StateHashEqual_(Self &d) : d_(d) {}
	    bool operator() (const State_ &s1, const State_ &s2) const {
		if (s1.output_ != s2.output_) return false;
		else if (s1.subset_ != s2.subset_) {
		    typename Substates::const_iterator sub1 = d_.substates_.begin(s1.subset_);
		    typename Substates::const_iterator sub2 = d_.substates_.begin(s2.subset_);
		    for (; sub1.valid() && sub2.valid(); ++sub1, ++sub2) {
			if (sub1->state_ != sub2->state_) return false;
			if (d_.semiring_->compare(sub1->weight_, sub2->weight_) != 0) return false;
			if (sub1->output_ != sub2->output_) return false;
		    }
		    if (sub1.valid() != sub2.valid()) return false;
		}
		return true;
	    }
	};
	typedef Fsa::Hash<State_, StateHashKey_, StateHashEqual_> States;
	mutable States states_;

	// output string
	mutable Fsa::LabelIdStrings outputs_;

	// temporary arc
	struct Arc_ {
	    typename Substates::Cursor predecessor_;
	    Fsa::LabelId input_;
	    Fsa::StateId target_;
	    Weight weight_;
	    Fsa::LabelId start_;
	    Arc_() {}
	    Arc_(typename Substates::Cursor predecessor, Fsa::LabelId input, Fsa::StateId target, Weight weight, Fsa::LabelId start) :
		predecessor_(predecessor), input_(input), target_(target), weight_(weight), start_(start) {}
	    bool operator< (const Arc_ &a) const { return input_ < a.input_; }
	};
	mutable Core::Vector<Arc_> tmpArcs_;
	mutable Core::Vector<Fsa::LabelId> tmpOutputs_;

    public:
	DeterminizeAutomaton(_ConstAutomatonRef f, bool disambiguate) :
	    disambiguate_(disambiguate), substates_(f->semiring()), states_(StateHashKey_(*this), StateHashEqual_(*this))
	{
	    fsa_ = sort<_Automaton>(f, Fsa::SortTypeByInput);
	    // copyProperties(fsa_, Fsa::PropertyAll);
	    this->unsetProperties(Fsa::PropertyAll);
	    this->setProperties(f->knownProperties(), f->properties());
	    this->setProperties(Fsa::PropertyStorage | Fsa::PropertyCached, 0);
	    this->unsetProperties(Fsa::PropertySortedByOutput);
	    //if (!hasProperties(fsa_, PropertyStorage)) fsa_ = cache<_Automaton>(fsa_, 10000);
	    semiring_ = fsa_->semiring();
	}

	virtual Fsa::Type type() const { return fsa_->type(); }
	virtual _ConstSemiringRef semiring() const { return semiring_; }
	virtual Fsa::StateId initialStateId() const {
	    if (fsa_->initialStateId() != Fsa::InvalidStateId) {
		Fsa::LabelIdStrings::Id output = outputs_.stop(outputs_.start());
		subsetStart();
		subsetInsert(Substate(fsa_->initialStateId(), output, semiring_->one()));
		return insertState(subsetStop(), output);
	    }
	    return Fsa::InvalidStateId;
	}
	virtual Fsa::ConstAlphabetRef getInputAlphabet() const { return fsa_->getInputAlphabet(); }
	virtual Fsa::ConstAlphabetRef getOutputAlphabet() const { return fsa_->getOutputAlphabet(); }

	// subsets
	void subsetStart() const { tmpSubstates_.resize(0); }
	void subsetInsert(const Substate &s) const { tmpSubstates_.push_back(s); }
	void subsetMarkSiblingsWithAmbiguousCycles() const {
	    // we follow the predecessor links until we find the same original state (= cycle)
	    for (typename Core::Vector<Substate>::iterator sub = tmpSubstates_.begin(); sub != tmpSubstates_.end(); ++sub)
		if ((*outputs_.begin(sub->output_) != Fsa::InvalidLabelId) || (!semiring_->isDefault(sub->weight_))) {
		    Fsa::Stack<typename Substate::Cursor> S;
		    if (sub->predecessor_ & Substates::Overflow) {
			for (typename Substates::const_predecessor_iterator p = substates_.predecessorBegin(sub->predecessor_);
			     p != substates_.predecessorEnd(sub->predecessor_); ++p) S.push(*p);
		    } else if (sub->predecessor_ != Substates::NoPredecessor) S.push(sub->predecessor_);
		    while (!S.empty()) {
			typename Substate::Cursor substate = S.pop();
			if (substates_.state(substate) == sub->state_) {
			    sub->disconnect_ = true;
			    break;
			} else {
			    substate = substates_.predecessor(substate);
			    if (substate & Substates::Overflow)
				for (typename Substates::const_predecessor_iterator p = substates_.predecessorBegin(substate);
				     p != substates_.predecessorEnd(substate); ++p) S.push(*p);
			    else if (substate != Substates::NoPredecessor) S.push(substate);
			}
		    }
		}
	}
	typename Substates::Cursor subsetStop() const {
	    size_t nExternalPredecessors = 0;
	    if (disambiguate_) nExternalPredecessors = substates_.nExternalPredecessors();
	    std::sort(tmpSubstates_.begin(), tmpSubstates_.end());
	    typename Core::Vector<Substate>::iterator j = tmpSubstates_.begin();
	    for (typename Core::Vector<Substate>::const_iterator i = tmpSubstates_.begin() + 1; i != tmpSubstates_.end(); ++i) {
		if ((j->state_ == i->state_) && (j->output_ == i->output_)) {
		    j->weight_ = semiring()->collect(j->weight_, i->weight_);
		    if (disambiguate_) j->predecessor_ = substates_.addPredecessor(j->predecessor_, i->predecessor_);
		} else if (++j < i) *j = *i;
	    }
	    tmpSubstates_.resize(j - tmpSubstates_.begin() + 1);
	    if (disambiguate_) subsetMarkSiblingsWithAmbiguousCycles();
	    std::pair<typename Substates::Cursor, bool> id = substates_.insert(tmpSubstates_);
	    if (disambiguate_) {
		if (id.second) {
		    for (typename Core::Vector<Substate>::const_iterator i = tmpSubstates_.begin() + 1; i != tmpSubstates_.end(); ++i)
			j->predecessor_ = substates_.addPredecessor(j->predecessor_, i->predecessor_);
		    substates_.resizeExternalPrecessors(nExternalPredecessors);
		}
	    }
	    return id.first;
	}

	// states
	Fsa::StateId insertState(typename Substates::Cursor subset, Fsa::LabelIdStrings::Id output) const {
	    Fsa::StateId id = states_.insert(State_(subset, output));
	    if (id > Fsa::StateIdMask) std::cerr << "determinize: out of state ids" << std::endl;
	    return id;
	}

	// string semirings
	void lcp(Core::Vector<Fsa::LabelId> &output, Core::Vector<Fsa::LabelId>::const_iterator start) const {
	    Core::Vector<Fsa::LabelId>::const_iterator o = output.begin();
	    for (; (*o != Fsa::InvalidLabelId) && (*o == *start); ++o, ++start);
	    output.resize(o - output.begin());
	    output.push_back(Fsa::InvalidLabelId);
	}
	bool isPrefix(Core::Vector<Fsa::LabelId> &output, Core::Vector<Fsa::LabelId>::const_iterator start) const {
	    Core::Vector<Fsa::LabelId>::const_iterator o = output.begin();
	    for (; (*o != Fsa::InvalidLabelId) && (*o == *start); ++o, ++start);
	    return (*o == Fsa::InvalidLabelId);
	}

	virtual _ConstStateRef getState(Fsa::StateId s) const {
	    if (s < states_.size()) {
		const State_ *state = &states_[s];
		const Fsa::LabelIdStrings::Id output = state->output_;
		_State *sp = new _State(s);

		if (*outputs_.begin(output) == Fsa::InvalidLabelId) { // zero length string
		    Fsa::StateTag tags = Fsa::StateTagNone;
		    Weight fw = semiring_->zero();

		    // extend weights and gather arcs
		    tmpArcs_.resize(0);
		    tmpOutputs_.resize(0);
		    typename Substates::Cursor predecessor = Substates::NoPredecessor;
		    for (typename Substates::const_iterator sub = substates_.begin(state->subset_); sub.valid(); ++sub)
			if (!sub->disconnect_) {
			    //std::cout << " " << sub.cursor() << " " << sub->state_ << std::endl;
			    _ConstStateRef _sp = fsa_->getState(sub->state_);
			    const Fsa::LabelIdStrings::Id output = sub->output_;
			    for (typename _State::const_iterator a = _sp->begin(); a != _sp->end(); ++a) {
				if (disambiguate_) predecessor = sub.cursor();
				tmpArcs_.push_back(Arc_(predecessor, a->input(), a->target(),
							semiring_->extend(sub->weight_, a->weight()), tmpOutputs_.size()));
				for (Core::Vector<Fsa::LabelId>::const_iterator i = outputs_.begin(output);
				     *i != Fsa::InvalidLabelId; ++i)
				    tmpOutputs_.push_back(*i);
				if (a->output() != Fsa::Epsilon) tmpOutputs_.push_back(a->output());
				tmpOutputs_.push_back(Fsa::InvalidLabelId);
			    }
			    // subsequential final strings at final states (yet not determinized on output side)
			    if (_sp->isFinal() && (*outputs_.begin(output) != Fsa::InvalidLabelId)) {
				_Arc *a = sp->newArc();
				a->output_ = *outputs_.begin(output);
				if (fsa_->type() == Fsa::TypeTransducer) a->input_ = Fsa::Epsilon;
				else a->input_ = a->output();
				a->weight_ = semiring_->one();
				subsetStart();
				subsetInsert(Substate(sub->state_, outputs_.insert(output + 1), sub->weight_));
				a->target_ = insertState(subsetStop(), state->output_);
				state = &states_[s];
				tags |= (_sp->tags() & ~Fsa::StateTagFinal);
			    } else tags |= _sp->tags();
			    if (_sp->isFinal()) {
				fw = semiring_->collect(fw, semiring_->extend(sub->weight_, _sp->weight_));
			    }
			} else { // disambiguate automaton => disconnect detected, ambiguous cycles
			    subsetStart();
			    subsetInsert(Substate(sub->state_, 0, semiring_->one()));
			    Fsa::LabelIdStrings::Id id = outputs_.insert(sub->output_ + 1);
			    Fsa::StateId target = insertState
				(subsetStop(), *outputs_.begin(sub->output_) == Fsa::InvalidLabelId ? sub->output_ : id);
			    Fsa::LabelId label = *outputs_.begin(sub->output_) == Fsa::InvalidLabelId ? Fsa::Epsilon :
				*outputs_.begin(sub->output_);
			    sp->newArc(target, sub->weight_,
				       (fsa_->type() == Fsa::TypeTransducer) ? Fsa::Epsilon : label, label);
			}
		    sp->setTags(tags);
		    if (sp->isFinal()) {
			sp->weight_ = fw;
		    }
		    std::sort(tmpArcs_.begin(), tmpArcs_.end());

		    // create arcs and collect weights
		    // one iteration of the following loop creates one arc with a unique input label
		    Core::Vector<Fsa::LabelId> outputArc, outputTarget;
		    for (typename Core::Vector<Arc_>::const_iterator a = tmpArcs_.begin(); a != tmpArcs_.end();) {
			typename Core::Vector<Arc_>::const_iterator first = a, last;
			_Arc *arc = sp->newArc();

			// determinize arcs and collect arc weights
			arc->input_ = first->input_;
			arc->weight_ = a->weight_;
			outputArc.resize(0);
			for (Core::Vector<Fsa::LabelId>::const_iterator i = tmpOutputs_.begin() + a->start_;
			     *i != Fsa::InvalidLabelId; ++i)
			    outputArc.push_back(*i);
			outputArc.push_back(Fsa::InvalidLabelId);
			for (++a; (a != tmpArcs_.end()) && (a->input_ == first->input_); ++a) {
			    arc->weight_ = semiring_->collect(arc->weight_, a->weight_);
			    lcp(outputArc, tmpOutputs_.begin() + a->start_);
			}
			last = a;

			// partial arc label
			Fsa::LabelIdStrings::Id output = 0;
			if (outputArc.size() > 1) {
			    arc->output_ = outputArc.front();
			    if (outputArc.size() > 2) output = outputs_.append(outputArc.begin() + 1);
			} else arc->output_ = Fsa::Epsilon;

			// gather target weights and outputs and create target state
			Weight inv = semiring_->invert(arc->weight_);
			subsetStart();
			for (a = first; a < last; ++a) {
			    Fsa::LabelIdStrings::Id output = 0;
			    Core::Vector<Fsa::LabelId>::const_iterator start = tmpOutputs_.begin() + a->start_;
			    if (isPrefix(outputArc, start)) output = outputs_.append(start + outputArc.size() - 1);
			    subsetInsert(Substate(a->predecessor_, a->target_, output,
						  semiring_->extend(inv, a->weight_)));
			}
			arc->target_ = insertState(subsetStop(), output);
			state = &states_[s]; // update pointer to state (might change due to resize of hash vector)
		    }
		} else {
		    // create new intermediate state with a single arc (also used for subsequential final strings)
		    sp->newArc(insertState(state->subset_, outputs_.insert(output + 1)), semiring_->one(),
			       (fsa_->type() == Fsa::TypeTransducer) ? Fsa::Epsilon : *outputs_.begin(output),
			       *outputs_.begin(output));
		}
		return _ConstStateRef(sp);
	    }
	    return _ConstStateRef();
	}
	void dumpOutput(Fsa::LabelIdStrings::Id output, std::ostream &o) const {
	    const Fsa::LabelIdStrings::Id s = output;
	    if (*outputs_.begin(s) == Fsa::InvalidLabelId) o << "eps";
	    else {
		Fsa::ConstAlphabetRef a = fsa_->getOutputAlphabet();
		for (Core::Vector<Fsa::LabelId>::const_iterator i = outputs_.begin(s); *i != Fsa::InvalidLabelId; ++i) {
		    if (i > outputs_.begin(s)) o << " ";
		    if (a) o << a->symbol(*i);
		    else o << *i;
		}
	    }
	}
	virtual void dumpState(Fsa::StateId s, std::ostream &o) const {
	    if (s < states_.size()) {
		const State_ *state = &states_[s];
		typename Substates::const_iterator begin = substates_.begin(state->subset_);
		for (typename Substates::const_iterator sub = begin; sub.valid(); ++sub) {
		    if (sub != begin) o << ",";
		    o << "(";
		    if (sub->disconnect_) o << "+";
		    fsa_->dumpState(sub->state_ & Fsa::StateIdMask, o);
		    if (!semiring_->isDefault(sub->weight_))
			o << "," << semiring_->asString(sub->weight_);
		    if (*outputs_.begin(sub->output_) != Fsa::InvalidLabelId) {
			o << ",";
			dumpOutput(sub->output_, o);
		    }
		    o << ")";
		}
		if (*outputs_.begin(state->output_) != Fsa::InvalidLabelId) {
		    o << " [";
		    dumpOutput(state->output_, o);
		    o << "] ";
		}
	    } else o << "unknown";
	}
	virtual size_t getMemoryUsed() const {

	    return fsa_->getMemoryUsed() + sizeof(_ConstAutomatonRef) + 2 * sizeof(Fsa::LabelId) +
		substates_.getMemoryUsed() + states_.getMemoryUsed() + outputs_.getMemoryUsed() +
		tmpSubstates_.getMemoryUsed() + tmpArcs_.getMemoryUsed() + tmpOutputs_.getMemoryUsed();
	}
	virtual void dumpMemoryUsage(Core::XmlWriter &o) const {
	    o << Core::XmlOpen("determinize");
	    fsa_->dumpMemoryUsage(o);
	    o << Core::XmlFull("states", states_.getMemoryUsed())
	      << Core::XmlFull("substates", substates_.getMemoryUsed())
	      << Core::XmlFull("predecessors", substates_.getPredecessorMemoryUsed())
	      << Core::XmlFull("outputs", outputs_.getMemoryUsed())
	      << Core::XmlFull("temporary", tmpArcs_.getMemoryUsed() + tmpOutputs_.getMemoryUsed())
	      << Core::XmlClose("determinize");
	}
	virtual std::string describe() const { return "determinize(" + fsa_->describe() + ")"; }
    };



    template<class _Automaton>
    typename _Automaton::ConstRef determinize(typename _Automaton::ConstRef f, bool disambiguate) {
	require(f);
	return typename _Automaton::ConstRef(new DeterminizeAutomaton<_Automaton>(f, disambiguate));
    }


    template<class _Automaton>
    class LocalDeterminizeAutomaton : public SlaveAutomaton<_Automaton> {
    private:
	typedef SlaveAutomaton<_Automaton> Precursor;

    public:
	typedef typename _Automaton::Arc _Arc;
	typedef typename _Automaton::State _State;
	typedef typename _Automaton::ConstStateRef _ConstStateRef;
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
	typedef typename _Automaton::ConstSemiringRef _ConstSemiringRef;

    private:
	_ConstSemiringRef semiring_;

    public:
	LocalDeterminizeAutomaton(_ConstAutomatonRef f) :
	    Precursor(sort<_Automaton>(f, Fsa::SortTypeByInputAndTarget)) {
	    semiring_ = f->semiring();
	}
	virtual _ConstStateRef getState(Fsa::StateId s) const {
	    _ConstStateRef _sp = Precursor::fsa_->getState(s);
	    if (_sp->nArcs() <= 1) return _sp;
	    _State *sp = new _State(_sp->id(), _sp->tags(), _sp->weight());
	    typename _State::const_iterator _a = _sp->begin(), _pa = _a;
	    *sp->newArc() = *_a;
	    for (++_a; _a != _sp->end(); ++_a) {
		if ((_a->input() == _pa->input()) && (_a->target() == _pa->target()))
		    (sp->begin() + sp->nArcs() - 1)->weight_ =
			semiring_->collect((sp->begin() + sp->nArcs() - 1)->weight(), _a->weight());
		else *sp->newArc() = *_a;
		_pa = _a;
	    }
	    return _ConstStateRef(sp);
	}
	virtual std::string describe() const { return "local-determinize(" + Precursor::fsa_->describe() + ")"; }
    };

    template<class _Automaton>
    typename _Automaton::ConstRef localDeterminize(typename _Automaton::ConstRef f) {
	require(f);
	return typename _Automaton::ConstRef(new LocalDeterminizeAutomaton<_Automaton>(f));
    }

    template<class _Automaton>
    class ReplaceDisambiguationSymbolsAutomaton : public ModifyAutomaton<_Automaton> {
	typedef ModifyAutomaton<_Automaton> Precursor;
    public:
	typedef typename _Automaton::State _State;
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
    private:
	bool shouldReplaceInput_, shouldReplaceOutput_;
	Fsa::LabelId replacement_;
    public:
	ReplaceDisambiguationSymbolsAutomaton(
	    _ConstAutomatonRef f, Fsa::LabelId ll,
	    bool shouldReplaceInput, bool shouldReplaceOutput)
	    :
	    Precursor(f),
	    shouldReplaceInput_(shouldReplaceInput),
	    shouldReplaceOutput_(shouldReplaceOutput),
	    replacement_(ll)
	{
	    this->unsetProperties(Fsa::PropertySorted);
	}

	virtual void modifyState(_State *sp) const {
	    for (typename _State::iterator a = sp->begin(); a != sp->end(); ++a) {
		if (shouldReplaceInput_)
		    if (Precursor::fsa_->getInputAlphabet()->isDisambiguator(a->input()))
			a->input_ = replacement_;
		if (shouldReplaceOutput_)
		    if (Precursor::fsa_->getOutputAlphabet()->isDisambiguator(a->output()))
			a->output_ = replacement_;
	    }
	}

	virtual std::string describe() const {
	    return "replaceDisambiguationSymbols("
		+ Precursor::fsa_->describe() + ", "
		+ Precursor::fsa_->getInputAlphabet()->symbol(replacement_) +")";
	}
    };

    template<class _Automaton>
    typename _Automaton::ConstRef removeDisambiguationSymbols(typename _Automaton::ConstRef f) {
	return typename _Automaton::ConstRef(
	    new ReplaceDisambiguationSymbolsAutomaton<_Automaton>(
		f, Fsa::Epsilon, true, true));
    }

    template<class _Automaton>
    typename _Automaton::ConstRef replaceDisambiguationSymbols(typename _Automaton::ConstRef f, Fsa::LabelId ll) {
	return typename _Automaton::ConstRef(
	    new ReplaceDisambiguationSymbolsAutomaton<_Automaton>(
		f, ll, true, true));
    }

    template<class _Automaton>
    typename _Automaton::ConstRef removeInputDisambiguationSymbols(typename _Automaton::ConstRef f) {
	return typename _Automaton::ConstRef(
	    new ReplaceDisambiguationSymbolsAutomaton<_Automaton>(
		f, Fsa::Epsilon, true, false));
    }

    template<class _Automaton>
    typename _Automaton::ConstRef replaceInputDisambiguationSymbols(typename _Automaton::ConstRef f, Fsa::LabelId ll) {
	return typename _Automaton::ConstRef(
	    new ReplaceDisambiguationSymbolsAutomaton<_Automaton>(
		f, ll, true, false));
    }

} // namespace Ftl
