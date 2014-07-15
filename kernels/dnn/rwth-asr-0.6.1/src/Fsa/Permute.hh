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
#ifndef _FSA_PERMUTE_HH
#define _FSA_PERMUTE_HH

#include <Core/Types.hh>
#include "Automaton.hh"
#include "Cache.hh"
#include "Dfs.hh"
#include "Hash.hh"
#include "Properties.hh"

namespace Fsa {

    /*
     * own implementation of a bitvector as we need a very fast hash key calculation
     */

    class Bitvector {
    private:
	size_t size_;
	std::vector<u32> bits_;
    public:
	Bitvector(const Bitvector &v, size_t flipPos) : size_(v.size_), bits_(v.bits_) { flip(flipPos); }
	Bitvector(size_t size, bool value = false) : size_(size), bits_((size + 31) / 32, value ? ~u32(0) : 0) {}
	size_t size() const { return size_; }
	bool operator[] (size_t pos) const { return (bits_[pos / 32] >> (pos % 32)) & 1; }
	bool operator== (const Bitvector &v) const { return (bits_ == v.bits_); }
	bool operator<(const Bitvector& v) const {return bits_<v.bits_;}

	void flip(size_t pos) { bits_[pos / 32] ^= (1 << (pos % 32)); }
	u32 hashKey() const {
	    u32 value = 0;
	    for (std::vector<u32>::const_iterator i = bits_.begin(); i != bits_.end(); ++i) value |= *i;
	    return value;
	}

	static void printBin(std::ostream& out,u32 x) {
	    // print u32 in readable form, usefull for debugging
	    for(int i=0;i<32;++i) out<<( (x>>i) & 1  ? "1" : "0");
	}

	bool isCovered(size_t A,size_t B) const {
	    // check if range [A,B) is covered

	    size_t Abi=A/32,Bbi=B/32;
	    for(size_t i=Abi+1;i<Bbi;++i) if(! (bits_[i]==~u32(0))) return 0;

	    u32 maskA= ( ~u32(0) >> (A%32)) << (A%32);
	    u32 maskB= ( ~u32(0) << (32-(B%32))) >> (32-(B%32));

	    if(Abi==Bbi)
		return (bits_[Abi] & (maskA & maskB)) == (maskA & maskB);
	    else
		return ((bits_[Abi] & maskA)==maskA) && ((bits_[Bbi] & maskB)==maskB);
	}


	friend std::ostream& operator<< (std::ostream &o, const Bitvector &b) {
	    for (u32 i = 0; i < b.size_; ++i) o << (b[i] ? "1" : "0");
	    return o;
	}
    };

    struct NoProcessing {
	void processState(const Bitvector &used, u32 lowerLimit, u32 upperLimit) {}
	void processArc(bool isNextOrdered, Arc *a) {}
	void processArc(int,int,Arc*) {}
	void setDistortionLimit(u16) {}
    };

    struct LogWeighting {
	bool isOrdered_;
	f32 baseProbability_; // probability \alpha for "normal direction" (no reordering)
	f32 logBaseProbability_;
	ConstSemiringRef semiring_;
	u32 nUncoveredPositionsInWindow_;
	u32 windowSize_;
	std::vector<f32> logProbabilitiesForReorderedPaths;
	std::vector<f32> logProbabilitiesWitoutOrderedPath;
	LogWeighting(ConstSemiringRef semiring, f32 baseProbability, u32 windowSize) :
	    baseProbability_(baseProbability), semiring_(semiring), windowSize_(windowSize),
	    logProbabilitiesForReorderedPaths(windowSize_ + 1), logProbabilitiesWitoutOrderedPath(windowSize_ + 1) {
	    logBaseProbability_ = -log(baseProbability_);
	    for (u32 i = 1; i <= windowSize_; ++i) {
		logProbabilitiesForReorderedPaths[i] = -log((1 - baseProbability_) / (i - 1));
		logProbabilitiesWitoutOrderedPath[i] = -log(1.0 / i);
	    }
	}
	void processState(const Bitvector &used, const u32 lowerLimit, const u32 upperLimit) {
	    u32 b;
	    isOrdered_ = true;
	    for (b = 0; b < used.size() && used[b]; ++b);
	    for (; b < used.size() && !used[b]; ++b);
	    if (b < used.size()) isOrdered_ = false;
	    nUncoveredPositionsInWindow_ = 0;
	    for (u32 i = lowerLimit; i < upperLimit; ++i)
		if (!used[i]) nUncoveredPositionsInWindow_++; // Here: ++ only if this position does not violate the distortion limit!
							      // (to be implemented! -> states_[s].depth_ must come in as a parameter!)
	}
	const void processArc(const bool isNextReordered, Arc *const a) {
	    Weight weight = semiring_->one();
	    if (nUncoveredPositionsInWindow_ == 1) weight = Weight(0.0);
	    else if (isOrdered_) {
		if (isNextReordered)
		    weight = Weight(logProbabilitiesForReorderedPaths[nUncoveredPositionsInWindow_]);
		else
		    weight = Weight(logBaseProbability_);
	    } else weight = Weight(logProbabilitiesWitoutOrderedPath[nUncoveredPositionsInWindow_]);
	    //    if (isNextOrdered) weight = Weight(-log(baseProbability_));
	    //          else weight =  Weight(-log((1 - baseProbability_) / (nUncoveredPositionsInWindow_ - 1)));
	    // } else weight = Weight(-log(1.0 / nUncoveredPositionsInWindow_));
	    a->weight_ = semiring_->extend(a->weight_, weight);
	    //a->weight_ = weight;
	}
       void setDistortionLimit(u16 distortionLimit) { }
    };

    template <class Processing> class PermuteAutomatonBase : public Automaton, protected DfsState {
    public:
	typedef Automaton::Weight Weight;
	typedef Automaton::Arc Arc;
	typedef Automaton::State State;
	typedef Automaton::ConstStateRef ConstStateRef;
	typedef Automaton::ConstRef ConstAutomatonRef;
	typedef Automaton::Semiring Semiring;
	typedef Automaton::ConstSemiringRef ConstSemiringRef;
    protected:
	std::vector<Arc> arcs_;
	Weight initialWeight_, finalWeight_;
	Processing *processing_;
	u32 windowSize_;
    public:
	PermuteAutomatonBase(ConstAutomatonRef f, u32 windowSize, Processing *processing) :
	    DfsState(cache(f)), processing_(processing) {
	    if (!hasProperties(fsa_, PropertyLinear))
		std::cerr << "permute is defined properly for linear automata only." << std::endl;
	    setProperties(PropertyAcyclic, PropertyAcyclic);
	    initialWeight_ = fsa_->semiring()->one();
	    finalWeight_ = fsa_->semiring()->one();
	    dfs();
	    if (arcs_.size() > 0)
		// assumes commutative semiring
		arcs_[0].weight_ = fsa_->semiring()->extend(initialWeight_, arcs_[0].weight_);
	    windowSize_ = std::min(size_t(windowSize), arcs_.size());
	}
	virtual ~PermuteAutomatonBase() { delete processing_; }
	virtual void discoverState(ConstStateRef sp) {
	    for (State::const_iterator a = sp->begin(); a != sp->end(); ++a)
		if ((a->input() == Epsilon) && (a->output() == Epsilon)) {
		    if (arcs_.size() > 0)
			arcs_[arcs_.size() - 1].weight_ =
			    fsa_->semiring()->extend(arcs_[arcs_.size() - 1].weight(), a->weight());
		    else initialWeight_ = fsa_->semiring()->extend(initialWeight_, a->weight());
		} else arcs_.push_back(*a);
	    if (sp->isFinal()) finalWeight_ = sp->weight_;
	}

	virtual Type type() const { return fsa_->type(); }
	virtual ConstSemiringRef semiring() const { return fsa_->semiring(); }
	virtual StateId initialStateId() const { return 0; }
	virtual ConstAlphabetRef getInputAlphabet() const { return fsa_->getInputAlphabet(); }
	virtual ConstAlphabetRef getOutputAlphabet() const { return fsa_->getOutputAlphabet(); }
	virtual void permuteArcs(State *sp) const = 0;
    };

    template <class Processing> class PermuteAutomaton : public PermuteAutomatonBase<Processing> {
    private:
	typedef PermuteAutomatonBase<Processing> Precursor;
    protected:

	// abstract state
	struct State_ {
	    u32 depth_;
	    Bitvector used_;
	    State_(u32 depth, const Bitvector &used) : depth_(depth),used_(used) {}
	    bool operator== (const State_ &s) const { return (used_ == s.used_); }
	};
	struct StateHashKey_ {
	    u32 operator() (const State_ &s) { return 2239 * s.depth_ + s.used_.hashKey(); }
	};
	typedef Hash<State_, StateHashKey_> States;
	mutable States states_;
    protected:
	StateId insertState(u32 depth, const Bitvector &used, size_t bit) const {
	    StateId id = states_.insert(State_(depth, Bitvector(used, bit)));
	    if (id > StateIdMask) std::cerr << "permute: out of state ids" << std::endl;
	    return id;
	}
	u32 distortionLimit_;
    public:
	PermuteAutomaton(ConstAutomatonRef f, u32 windowSize, u16 distortionLimit, Processing *processing) :
	PermuteAutomatonBase<Processing>(f,windowSize,processing) {
	    states_.insert(State_(0, Bitvector(Precursor::arcs_.size())));
	    processing->setDistortionLimit(distortionLimit);
	    distortionLimit_ = distortionLimit;
	}
	virtual ~PermuteAutomaton() { }

	virtual ConstStateRef getState(StateId s) const {
	    if (s < states_.size()) {
		State *sp = new State();
		sp->setId(s);
		if (s == 0)
		    sp->weight_ = Precursor::fsa_->semiring()->one();

		if (states_[s].depth_ < states_[s].used_.size()) {
		    this->permuteArcs(sp);
		} else {
		    sp->addTags(StateTagFinal);
		    sp->weight_ = Precursor::finalWeight_;
		}
		return ConstStateRef(sp);
	    }
	    return ConstStateRef();
	}
	virtual void dumpState(StateId s, std::ostream &o) const {
	    o << states_[s].used_; // << " " << states_[s].depth_;
	}
	virtual size_t getMemoryUsed() const {
	    return Precursor::fsa_->getMemoryUsed() + sizeof(Precursor::windowSize_) + sizeof(Arc) * Precursor::arcs_.size() +
		states_.getMemoryUsed();
	}
    };

    template <class Processing> class IBMPermuteAutomaton : public PermuteAutomaton<Processing> {
    private:
	typedef PermuteAutomaton<Processing> Precursor;
    public:
	IBMPermuteAutomaton(ConstAutomatonRef f, u32 windowSize, u16 distortionLimit, Processing *processing) :
	    PermuteAutomaton<Processing>(f, windowSize, distortionLimit, processing) { };

	virtual void permuteArcs(State *sp) const {
	    StateId s = sp->id();
	    u32 lowerLimit = 0;
	    u32 upperLimit = std::min(size_t(Precursor::states_[s].depth_ + Precursor::windowSize_), Precursor::states_[s].used_.size());
	    Precursor::processing_->processState(Precursor::states_[s].used_, lowerLimit, upperLimit);
	    for (u32 i = lowerLimit; i < upperLimit; ++i)
		if (!Precursor::states_[s].used_[i] && (size_t(abs(i - Precursor::states_[s].depth_)) < Precursor::distortionLimit_)) { // !!! LIMIT
		    u32 depth = Precursor::states_[s].depth_ + 1;
		    Arc *a = sp->newArc();
		    *a = Precursor::arcs_[i];
		    a->target_ = Precursor::insertState(depth, Precursor::states_[s].used_, i);
		    Precursor::processing_->processArc(i - Precursor::states_[s].depth_, a);  // processing_->processArc(i - states_[s].depth_, states_[s].used_[i-1],a);
		}
	}
	virtual std::string describe() const {
	    return Core::form("ibm-permute(%s,%d)", Precursor::fsa_->describe().c_str(), Precursor::windowSize_);
	}
    };

    template <class Processing> class LocalPermuteAutomaton : public PermuteAutomaton<Processing> {
    private:
	typedef PermuteAutomaton<Processing> Precursor;
    public:
	LocalPermuteAutomaton(ConstAutomatonRef f, u32 windowSize, u16 distortionLimit, Processing *processing) :
	    PermuteAutomaton<Processing>(f, windowSize, distortionLimit, processing) {};

	virtual void permuteArcs(State *sp) const {
	    StateId s = sp->id();
	    u32 firstBitNotSet;
	    for (firstBitNotSet=0; Precursor::states_[s].used_[firstBitNotSet]; firstBitNotSet++);
	    u32 upperLimit = std::min(size_t(firstBitNotSet + Precursor::windowSize_), Precursor::states_[s].used_.size());
	    Precursor::processing_->processState(Precursor::states_[s].used_, firstBitNotSet, upperLimit);
	    for (u32 i = firstBitNotSet; i < upperLimit; ++i)
		if (!Precursor::states_[s].used_[i]) {
		    Arc *a = sp->newArc();
		    *a = Precursor::arcs_[i];
		    a->target_ = Precursor::insertState(Precursor::states_[s].depth_+1, Precursor::states_[s].used_, i);
		    Precursor::processing_->processArc(i - Precursor::states_[s].depth_, a);
		}
	}
	virtual std::string describe() const {
	    return Core::form("local-permute(%s,%d)", Precursor::fsa_->describe().c_str(), Precursor::windowSize_);
	}
    };

    template <class Processing> class InverseIBMPermuteAutomaton : public PermuteAutomaton<Processing> {
    private:
	typedef PermuteAutomaton<Processing> Precursor;
    public:

	InverseIBMPermuteAutomaton(ConstAutomatonRef f, u32 windowSize, u16 distortionLimit, Processing *processing) :
	    PermuteAutomaton<Processing>(f, windowSize, distortionLimit, processing) {};

	virtual void permuteArcs(State *sp) const {
	    StateId s = sp->id();
	    u32 firstBitNotSet;
	    for (firstBitNotSet=0; Precursor::states_[s].used_[firstBitNotSet]; firstBitNotSet++);
	    Precursor::processing_->processState(Precursor::states_[s].used_, firstBitNotSet,
				      (Precursor::states_[s].depth_ - firstBitNotSet < Precursor::windowSize_) ?  Precursor::states_[s].used_.size() : firstBitNotSet+1);
	    // monotone transition
	    if(size_t(abs(firstBitNotSet - Precursor::states_[s].depth_))< Precursor::distortionLimit_) {
		Arc *a = sp->newArc();
		*a = Precursor::arcs_[firstBitNotSet];
		a->target_ = Precursor::insertState(Precursor::states_[s].depth_+1, Precursor::states_[s].used_, firstBitNotSet);
		Precursor::processing_->processArc(firstBitNotSet - Precursor::states_[s].depth_, a);
	    }
	    if (Precursor::states_[s].depth_ - firstBitNotSet < Precursor::windowSize_) { // if we can do non-monotone transitions
		for (u32 i = firstBitNotSet+1; i<Precursor::states_[s].used_.size(); i++) {
		    if (!Precursor::states_[s].used_[i] && (size_t(abs(i - Precursor::states_[s].depth_))< Precursor::distortionLimit_)) { // DISTORTION LIMIT HERE !
			Arc *a = sp->newArc();
			*a = Precursor::arcs_[i];
			a->target_ = Precursor::insertState(Precursor::states_[s].depth_+1, Precursor::states_[s].used_, i);
			Precursor::processing_->processArc(i - Precursor::states_[s].depth_, a);
		    }
		}
	    }
	}
	virtual std::string describe() const {
	    return Core::form("inverse-ibm-permute(%s,%d)", Precursor::fsa_->describe().c_str(), Precursor::windowSize_);
	}
    };

    template <class Processing> class DoubleLocalPermuteAutomaton : public PermuteAutomaton<Processing> {
    private:
	typedef PermuteAutomaton<Processing> Precursor;
    public:
	DoubleLocalPermuteAutomaton(ConstAutomatonRef f, u32 windowSize, u16 distortionLimit, Processing *processing) :
	    PermuteAutomaton<Processing>(f, windowSize, distortionLimit, processing) {};

	virtual void permuteArcs(State *sp) const {
	    // normal local window
	    StateId s = sp->id();
	    u32 lowerLimit = Precursor::states_[s].depth_;
	    u32 upperLimit = std::min(size_t(Precursor::states_[s].depth_ + Precursor::windowSize_), Precursor::states_[s].used_.size());
	    for (u32 i = lowerLimit; i < upperLimit; ++i)
		if (!Precursor::states_[s].used_[i]) {
		    u32 depth;
		    //			    depth = upperLimit;
		    // 			    for (u32 j = states_[s].depth_; j < upperLimit; ++j)
		    // 				if ((j != i) && (!states_[s].used_[j])) {
		    // 				    depth = std::min(depth, j);
		    // 				    break;
		    // 				}
		    if (i == Precursor::states_[s].depth_)
			for (depth = Precursor::states_[s].depth_ + 1;
			     depth < Precursor::states_[s].used_.size() && Precursor::states_[s].used_[depth];
			     depth++);
		    else
			depth = Precursor::states_[s].depth_;
		    Arc *a = sp->newArc();
		    *a = Precursor::arcs_[i];
		    a->target_ = Precursor::insertState(depth, Precursor::states_[s].used_, i);
		}

	    // "inverse" local window
	    // upperLimit = states_[s].inverseDepth_;
	    for (upperLimit = Precursor::states_[s].used_.size()-1; Precursor::states_[s].used_[upperLimit] && upperLimit>0; upperLimit--);
	    lowerLimit = std::max(int(upperLimit - Precursor::windowSize_), int(Precursor::states_[s].depth_ + Precursor::windowSize_ - 1));
	    for (u32 i = upperLimit; i > lowerLimit; --i)
		if (!Precursor::states_[s].used_[i]) {
		    u32 depth = Precursor::states_[s].depth_;
		    if (i == Precursor::states_[s].depth_)
			depth++;
		    Arc *a = sp->newArc();
		    *a = Precursor::arcs_[i];
		    a->target_ = Precursor::insertState(depth, Precursor::states_[s].used_, i);
		}
	}
	virtual std::string describe() const {
	    return Core::form("double-local-permute(%s,%d)", Precursor::fsa_->describe().c_str(), Precursor::windowSize_);
	}
    };

    template <class Processing> class ITGPermuteAutomaton : public PermuteAutomatonBase<Processing> {
    private:
	typedef PermuteAutomatonBase<Processing> Precursor;
	typedef std::pair<u32,u32> Span;

	struct Spans : public std::vector<Span> {
	    typedef std::vector<Span> Parent;

	    Spans() : Parent() {}
	    Spans(const Spans &x) : Parent(x) {}
	    Spans(const Spans &x, u32 newpos) : Parent(x) { push_back(Span(newpos, newpos+1)); }

	    void push_back(const Span& x) { Parent::push_back(x); reduce(); }

	    void reduce() { while(reduce_()); }
	    bool reduce_() {
		if (Parent::size() < 2) return false;
		Span &a = Parent::operator[](Parent::size() - 2);
		Span &b = Parent::back();
		if (a.first == b.second) { a.first = b.first; Parent::pop_back(); return true; }
		if (a.second == b.first) { a.second = b.second; Parent::pop_back(); return true; }
		return false;
	    }

	    bool isValid() const { return (Parent::size() == 1); }

	    u32 hashKey() const { return 59 * Parent::size(); }

	    friend std::ostream& operator<< (std::ostream& out, const Spans &x) {
		for (size_t i = 0; i < x.size(); ++i)
		    out<<"(" << x[i].first << "," << x[i].second << ") ";
		return out;
	    }

	    friend bool operator==(const Spans& a,const Spans& b) {
		return (a.size()==b.size() && memcmp(&a[0],&b[0],a.size()*sizeof(Span))==0);}
	};

	// abstract state
	struct State_ {
	    u32 depth_;
	    Bitvector used_;
	    int last_;
	    Spans spans_;

	    State_(u32 depth, const State_ &x,u32 newpos) : depth_(depth),used_(x.used_,newpos),last_((depth<used_.size() ? newpos : -2)),spans_(x.spans_,newpos) {}
	    State_(u32 depth, const Bitvector &used) : depth_(depth),used_(used),last_(-1) {}

	    bool operator==(const State_ &s) const {
		return ( used_ == s.used_
			 && spans_==s.spans_
			 && ( last_==s.last_ || ( last_<s.last_ ? used_.isCovered(last_,s.last_) : used_.isCovered(s.last_,last_))));}

	    bool isValid() const {return spans_.isValid();}

	    bool isValidSuccessor(u32 i) const {
		//  access to coverage vector could be optimized (!)
		bool isOK=1;
		if (last_ != -1) {
		    if (static_cast<int>(i) < last_) {
			for (int j = i + 1; j < last_; ++j)
			    if (used_[j] && !used_[j + 1]) { isOK = 0; break; }
		    } else {
			for (u32 j = last_ + 1; j < i; ++j)
			    if (used_[j] && !used_[j - 1]) { isOK = 0; break; }
		    }
		}
		return isOK;
	    }

	    int getLastPosition() const {return last_;}

	    friend std::ostream& operator<<(std::ostream& out,const State_& x) {
		return out<<x.depth_<<" "<<x.used_<<" "<<x.spans_<<" "<<x.last_;
	    }

	};
	struct StateHashKey_ {
	    u32 operator() (const State_ &s) { return 2239 * s.depth_ + s.used_.hashKey(); }
	};
	typedef Hash<State_, StateHashKey_> States;
	mutable States states_;
    protected:
	StateId insertState(u32 depth, const State_ &oldState, size_t bit) const {
	    StateId id = states_.insert(State_(depth, oldState, bit));
	    if (id > StateIdMask) std::cerr << "permute: out of state ids" << std::endl;
	    return id;
	}

    public:
	ITGPermuteAutomaton(ConstAutomatonRef f, u32 windowSize, u16 distortionLimit, Processing *processing) :
	    PermuteAutomatonBase<Processing>(f,windowSize, processing) {
	    states_.insert(State_(0, Bitvector(Precursor::arcs_.size())));
	};

	virtual ConstStateRef getState(StateId s) const {
	    if (s < states_.size()) {
		State *sp = new State();
		sp->setId(s);
		if (s == 0)
		    sp->weight_ = Precursor::fsa_->semiring()->one();

		if (states_[s].depth_ < states_[s].used_.size()) {
		    permuteArcs(sp);
		} else {
		    // check ITG constraints
		    if(states_[s].isValid()) {
			sp->addTags(StateTagFinal);
			sp->weight_ = Precursor::finalWeight_;
		    }
		}
		return ConstStateRef(sp);
	    }
	    return ConstStateRef();
	}

	virtual void permuteArcs(State *sp) const {
	    StateId s = sp->id();
	    u32 lowerLimit = 0;
	    u32 upperLimit = std::min(size_t(states_[s].depth_ + Precursor::windowSize_), states_[s].used_.size());

	    for (u32 i = lowerLimit; i < upperLimit; ++i)
		if (!states_[s].used_[i]) {
		    const State_ &localState=states_[s];

		    // this checks a necessary condition for a valid ITG reordering (nice speedup)
		    //  -> early discarding of (some) invalid reorderings
		    bool isOK=localState.isValidSuccessor(i);

		    if (isOK) {
			u32 depth = localState.depth_ + 1;
			Arc *a = sp->newArc();
			*a = Precursor::arcs_[i];
			Precursor::processing_->processArc(localState.getLastPosition(), i, a);
			a->target_ = insertState(depth, localState, i); // ATTENTION: insertState() invalidates localState
		    }
		}
	}

	virtual void dumpState(StateId s, std::ostream &o) const { o <<s<<"-"<<states_[s]; }


	virtual std::string describe() const {
	    return Core::form("itg-permute(%s,%d)", Precursor::fsa_->describe().c_str(), Precursor::windowSize_);
	}

    };


    ConstAutomatonRef permute(ConstAutomatonRef f, u32 windowSize = Core::Type<u32>::max, u16 distortionLimit = Core::Type<u16>::max);
    ConstAutomatonRef localPermute(ConstAutomatonRef f, u32 windowSize = Core::Type<u32>::max, u16 distortionLimit = Core::Type<u16>::max);
    ConstAutomatonRef inverseIBMPermute(ConstAutomatonRef f, u32 windowSize = Core::Type<u32>::max, u16 distortionLimit = Core::Type<u16>::max);
    ConstAutomatonRef doubleLocalPermute(ConstAutomatonRef f, u32 windowSize = Core::Type<u32>::max, u16 distortionLimit = Core::Type<u16>::max);
    ConstAutomatonRef ITGPermute(ConstAutomatonRef f, u32 windowSize = Core::Type<u32>::max, u16 distortionLimit = Core::Type<u16>::max);

} // namespace

#endif // _FSA_PERMUTE_HH
