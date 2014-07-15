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
#ifndef _T_FSA_AUTOMATON_HH
#define _T_FSA_AUTOMATON_HH

#include <vector>
#include <Core/ReferenceCounting.hh>
#include <Core/Types.hh>
#include <Core/XmlStream.hh>
#include "tSemiring.hh"
#include "Alphabet.hh"
#include "Types.hh"

namespace Ftl
{
    /*
      template<class _Weight>
      concept Arc {
      typedef Weight_ Weight;
      Arc();
      Arc(Fsa::StateId target, Weight weight, Fsa::LabelId input, Fsa::LabelId output);

      Fsa::StateId target() const;
      Weight       weight() const;
      Fsa::LabelId input()  const;
      Fsa::LabelId output() const;
      };
    */

    /**
     * The basic arc structure. This is a struct only as you are allowed to modify
     * member variables directly if you own an instance of it. Otherwise use the
     * read-only attribute methods to retrieve the appropriate values.
     * @warning in case of acceptors, input and output labels MUST be the same!
     *    If not, some algorithms might produce unpredictable results.
     *
     * Implements concept Ftl::Arc
     *
     **/
    template<class _Weight> struct Arc
    {
	typedef _Weight Weight;

	Fsa::StateId target_;
	Weight weight_;
	Fsa::LabelId input_;
	Fsa::LabelId output_;
	Arc() {}

	Arc(Fsa::StateId target, Weight weight, Fsa::LabelId input,
	    Fsa::LabelId output) :
	    target_(target), weight_(weight), input_(input), output_(output)
	    {}

	~Arc() {}

	Fsa::StateId target() const
	    {
		return target_;
	    }
	Weight weight() const
	    {
		return weight_;
	    }
	Fsa::LabelId input() const
	    {
		return input_;
	    }
	Fsa::LabelId output() const
	    {
		return output_;
	    }
	void setTarget(Fsa::StateId target)
	    {
		target_ = target;
	    }
	void setWeight(const Weight &weight)
	    {
		weight_ = weight;
	    }
	void setInput(Fsa::LabelId input)
	    {
		input_ = input;
	    }
	void setOutput(Fsa::LabelId output)
	    {
		output_ = output;
	    }
    };

    /**
     * stores arcs in a STL vector.
     * advantage: fast insertion
     * disadvantage: overhead in memory usage
     */
    template<class _Arc>
    class VectorArcContainer : public std::vector<_Arc>
    {
	typedef _Arc Arc;
	typedef std::vector<_Arc> Predecessor;
    public:
	Arc* addArc() {
	    Predecessor::push_back(_Arc());
	    return &Predecessor::back();
	}
	Arc* addArc(Fsa::StateId target, typename _Arc::Weight weight, Fsa::LabelId input) {
	    Predecessor::push_back(_Arc(target, weight, input, input));
	    return &Predecessor::back();
	}
	Arc* addArc(Fsa::StateId target, typename _Arc::Weight weight, Fsa::LabelId input,
		    Fsa::LabelId output) {
	    Predecessor::push_back(_Arc(target, weight, input, output));
	    return &Predecessor::back();
	}

	void truncate(typename Predecessor::iterator i) { Predecessor::erase(i, Predecessor::end()); }
	void minimize() {}
    };

    /**
     * stores arcs in self managed memory.
     * advantage: no overhead in memory usage
     * disadvantage: slower insertion
     */
    template<class _Arc>
    class ArrayArcContainer
    {
	typedef _Arc Arc;
	typedef ArrayArcContainer<_Arc> Self;
    public:
	typedef _Arc* iterator;
	typedef const _Arc* const_iterator;
	typedef std::reverse_iterator<iterator> reverse_iterator;
	typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    private:
	Arc *arcs_;
	u32 nArcs_;
	void reallocate(size_t s);
	void reallocateOneMore();
	void destroy(iterator first, iterator last);

    public:
	ArrayArcContainer() : arcs_(0), nArcs_(0) {}
	ArrayArcContainer(const ArrayArcContainer<_Arc>&);
	~ArrayArcContainer() { clear(); }
	Self& operator=(const Self&);

	iterator begin() { return arcs_; }
	iterator end() { return arcs_ + nArcs_; }
	const_iterator begin() const { return arcs_; }
	const_iterator end() const { return arcs_ + nArcs_; }
	reverse_iterator rbegin() { return reverse_iterator(arcs_ + nArcs_ - 1); }
	reverse_iterator rend() { return reverse_iterator(arcs_ - 1); }
	const_reverse_iterator rbegin() const { return const_reverse_iterator(arcs_ + nArcs_ - 1); }
	const_reverse_iterator rend() const { return const_reverse_iterator(arcs_ - 1); }

	const Arc& operator[](unsigned i) const { return arcs_[i]; }
	Arc& operator[](unsigned i) { return arcs_[i]; }
	size_t size() const { return nArcs_; }
	bool empty() const { return (nArcs_ == 0); }

	Arc* addArc();
	Arc* addArc(Fsa::StateId target, typename _Arc::Weight weight, Fsa::LabelId input);
	Arc* addArc(Fsa::StateId target, typename _Arc::Weight weight, Fsa::LabelId input,
		    Fsa::LabelId output);
	iterator insert(iterator pos, const Arc &a);

	void truncate(iterator i);
	void clear();
	void resize(size_t s);
	void minimize();
    };

    /**
     * The basic state class. States are reference counted as you can't know when
     * to delete them.
     **/
    template<class _Arc, template<class T> class _ArcContainer = VectorArcContainer>
    class State : public Core::ReferenceCounted
    {
	typedef State< _Arc, _ArcContainer> Self;
	typedef _ArcContainer<_Arc> ArcContainer;
public:
	typedef Core::Ref<Self> Ref;
	typedef Core::Ref<const Self> ConstRef;
	typedef _Arc Arc;
	typedef typename _Arc::Weight Weight;
private:
	Fsa::StateId idAndTags_;
	ArcContainer arcs_;
public:
	typedef typename ArcContainer::iterator iterator;
	typedef typename ArcContainer::const_iterator const_iterator;
	typedef typename ArcContainer::reverse_iterator reverse_iterator;
	typedef typename ArcContainer::const_reverse_iterator
			const_reverse_iterator;
	Weight weight_;
public:
	State(Fsa::StateId id = 0, Fsa::StateTag tags = Fsa::StateTagNone) :
		idAndTags_((id & Fsa::StateIdMask) | (tags & Fsa::StateTagMask))
	{}

	State(Fsa::StateId id, Fsa::StateTag tags, const Weight &weight) :
		idAndTags_((id & Fsa::StateIdMask) | (tags & Fsa::StateTagMask)),
				weight_(weight)
	{}

	~State() {}

	State(const Self &o) :
		idAndTags_(o.idAndTags_), arcs_(o.arcs_), weight_(o.weight_)
	{}

	template<template<typename> class _OtherArcContainer>
	State(const State<_Arc, _OtherArcContainer> &o) :
		idAndTags_((o.id() & Fsa::StateIdMask) | (o.tags() & Fsa::StateTagMask)),
		weight_(o.weight_)
	{
		arcs_.resize(o.nArcs());
		std::copy(o.begin(), o.end(), begin());
	}

	template<template<typename> class _OtherArcContainer>
	operator State<_Arc, _OtherArcContainer>()
	{
		State<_Arc, _OtherArcContainer> s(*this);
		return s;
	}

	void clear()
	{
		idAndTags_ = 0;
		arcs_.clear();
	}
	Weight weight() const
	{
		return weight_;
	}
	void setWeight(const Weight &weight)
	{
		weight_ = weight;
	}
	void setId(Fsa::StateId id)
	{
		idAndTags_ = (id & Fsa::StateIdMask) | (idAndTags_ & Fsa::StateTagMask);
	}
	Fsa::StateId id() const
	{
		return idAndTags_ & Fsa::StateIdMask;
	}
	void addTags(Fsa::StateTag tags)
	{
		require_((tags & Fsa::StateIdMask) == 0);
		idAndTags_ = idAndTags_ | tags;
	}
	void setTags(Fsa::StateTag tags)
	{
		require_((tags & Fsa::StateIdMask) == 0);
		idAndTags_ = (idAndTags_ & Fsa::StateIdMask) | tags;
	}
	bool hasTags(Fsa::StateTag tags) const
	{
		return idAndTags_ & tags;
	}
	Fsa::StateTag tags() const
	{
		return idAndTags_ & Fsa::StateTagMask;
	}
	bool isFinal() const
	{
		return hasTags(Fsa::StateTagFinal);
	}
	void setFinal(const Weight &weight)
	{
		addTags(Fsa::StateTagFinal);
		weight_ = weight;
	}
	void unsetFinal()
	{
		idAndTags_ &= ~Fsa::StateTagFinal;
	}

	const Arc* operator[](unsigned i) const
	{
		return &(arcs_[i]);
	}
	const Arc* getArc(unsigned i) const
	{
		return &(arcs_[i]);
	}

	/// Warning: the returned pointer can become invalid after a new call of newArc()
	Arc* newArc();
	Arc* newArc(Fsa::StateId target, Weight weight, Fsa::LabelId input);
	Arc* newArc(Fsa::StateId target, Weight weight, Fsa::LabelId input,
			Fsa::LabelId output);
	bool hasArcs() const
	{
		return !arcs_.empty();
	}
	u32 nArcs() const
	{
		return arcs_.size();
	}
	iterator begin()
	{
		return arcs_.begin();
	}
	iterator end()
	{
		return arcs_.end();
	}
	const_iterator begin() const
	{
		return arcs_.begin();
	}
	const_iterator end() const
	{
		return arcs_.end();
	}
	reverse_iterator rbegin()
	{
		return arcs_.rbegin();
	}
	reverse_iterator rend()
	{
		return arcs_.rend();
	}
	const_reverse_iterator rbegin() const
	{
		return arcs_.rbegin();
	}
	const_reverse_iterator rend() const
	{
		return arcs_.rend();
	}
	void truncate(iterator i)
	{
		arcs_.truncate(i);
	}

	void resize(size_t s)
	{
		arcs_.resize(s);
	}
	void minimize()
	{
		arcs_.minimize();
	}
	iterator insert(iterator pos, const Arc &a)
	{
		return arcs_.insert(pos, a);
	}
	template<class Predicate> void remove(Predicate pred)
	{
		arcs_.erase(std::remove_if(arcs_.begin(), arcs_.end(), pred),
				arcs_.end());
	}
	template<class T, class StrictWeakOrdering> iterator lower_bound(
			const T &value, StrictWeakOrdering comp)
	{
		return std::lower_bound(arcs_.begin(), arcs_.end(), value, comp);
	}
	template<class T, class StrictWeakOrdering> const_iterator lower_bound(
			const T &value, StrictWeakOrdering comp) const
	{
		return std::lower_bound(arcs_.begin(), arcs_.end(), value, comp);
	}
	template<class StrictWeakOrdering> void sort(StrictWeakOrdering comp)
	{
		std::stable_sort(arcs_.begin(), arcs_.end(), comp);
	}

	size_t getMemoryUsed() const
	{
		return sizeof(State) + sizeof(Arc) * arcs_.size();
	}
};

/**
 * The basic automaton data structure. Automaton is a virtual base class to ALL
 * other automaton classes. It defines the abstract, lazy interface to access an
 * automaton. All algorithms are based only on this abstract definition. Lazy
 * automaton classes therefore derive from this class and overload the appropriate
 * methods.
 **/
template<class _Semiring, class _State =State<Arc<typename _Semiring::Weight> > > class Automaton :
	public Core::ReferenceCounted
{
	typedef Automaton<_Semiring, _State> Self;
public:
	/*
	 Attention:
	 If you use a class inheriting from tAutomaton in conjunction with Ftl algorithms,
	 you must redefine Ref and ConstRef.

	 - todo: get rid of ::ConstRef and ::Ref by replacing them by explicit Core::Ref<...> constructs
	 (has to be done in all t-files)
	 */
	typedef Core::Ref<Self> Ref;
	typedef Core::Ref<const Self> ConstRef;

	typedef typename _Semiring::Weight Weight;
	typedef typename _State::Arc Arc;
	typedef _State State;
	typedef typename State::Ref StateRef;
	typedef typename State::ConstRef ConstStateRef;
	typedef _Semiring Semiring;
	typedef typename Semiring::ConstRef ConstSemiringRef;

protected:
	mutable Fsa::Property knownProperties_, properties_;

protected:
	Automaton() :
		knownProperties_(0), properties_(0)
	{
	}

	virtual void copyProperties(ConstRef f,
			Fsa::Property mask = Fsa::PropertyAll)
	{
	    verify(f);
	    unsetProperties(Fsa::PropertyAll);
	    setProperties(mask & f->knownProperties(), mask & f->properties());
	    verify((mask & knownProperties_) == (mask & f->knownProperties()));
	    verify((mask & properties_) == (mask & f->properties()));
	}

public:
	/**
	 * The destructor of the automaton. Add cleanup code to it if you allocate
	 * temporary objects.
	 **/
	virtual ~Automaton()
	{
	}

	/**
	 * also see Fsa/Types.hh
	 * @return returns the type of the automaton
	 **/
	virtual Fsa::Type type() const
	{
		return Fsa::TypeUnknown;
	}

	/** Declare properties as known and set. */
	virtual void addProperties(Fsa::Property properties) const
	{
		knownProperties_ |= properties;
		properties_ |= properties;
	}
	/** deprecated, use addProperties */
	virtual void setProperties(Fsa::Property properties) const
	{
		addProperties(properties);
	}

	/** Declare properties as known and their (known) values, i.e. set or unset these properties. */
	virtual void setProperties(Fsa::Property knownProperties,
			Fsa::Property properties) const
	{
		knownProperties_ |= knownProperties;
		properties_ &= ~knownProperties;
		properties_ |= knownProperties & properties;
	}

	/** Declare properties as unknown (and unset them). */
	virtual void unsetProperties(Fsa::Property unknownProperties) const
	{
		knownProperties_ &= ~unknownProperties;
		properties_ &= ~unknownProperties;
	}

	/**
	 * Get properties that can currently be retrieved without
	 * further computation.
	 **/
	Fsa::Property knownProperties() const
	{
		return knownProperties_;
	}

	/**
	 * Test if a property can be tested without further
	 * computation.
	 */
	bool knowsProperty(Fsa::Property property) const
	{
		return knownProperties() & property;
	}

	/**
	 * Get properties. Do not use the information returned by this method
	 * without looking at the knownProperties!
	 **/
	Fsa::Property properties() const
	{
		return properties_;
	}

	/**
	 * Test a property
	 * - returns false, if unknown
	 * - does NOT compute the requested property, if unknown
	 */
	bool hasProperty(Fsa::Property property) const
	{
		return property & properties() & knownProperties();
	}

	/**
	 * @return returns the reference to a semiring structure that should be used
	 *    to interpret arc and final state weights
	 **/
	virtual ConstSemiringRef semiring() const = 0;
	//{ return ConstSemiringRef(); }

	/**
	 * An automaton has only a single initial state.
	 * @return returns the state id of the initial state or InvalidStateId if
	 *    the automaton has no initial state
	 **/
	virtual Fsa::StateId initialStateId() const
	{
		return Fsa::InvalidStateId;
	}

	/**
	 * Returns a reference to the input alphabet.
	 * @return returns a reference to the lazy input alphabet or an empty reference
	 *    if the automaton provides no input alphabet
	 **/
	virtual Fsa::ConstAlphabetRef getInputAlphabet() const = 0;
	Fsa::ConstAlphabetRef inputAlphabet() const
	{
		return getInputAlphabet();
	}

	/**
	 * Returns a reference to the output alphabet.
	 * In case of an acceptor this method should return the same reference than
	 * getInputAlphabet().
	 * @return returns a reference to the lazy output alphabet or an empty reference
	 *    if the automaton provides no output alphabet
	 **/
	virtual Fsa::ConstAlphabetRef getOutputAlphabet() const
	{
		return Fsa::ConstAlphabetRef();
	}
	Fsa::ConstAlphabetRef outputAlphabet() const
	{
		return getOutputAlphabet();
	}

	/**
	 * Returns a reference to a state. All algorithms assume that this does not
	 * fail (return an empty reference) if the state id has been reported by the
	 * abstract automaton itself before (either through initialStateId() or target
	 * state ids on arcs).
	 * @warning Note that in case of lazy algorithms calling this method may take some
	 *    time as states, i.e. mainly the list of outgoing arcs, are calculated on-demand.
	 * @param s id of the requested state
	 * @return return a reference to the state with id s or an empty reference
	 *    if the state does not exist
	 **/
	virtual ConstStateRef getState(Fsa::StateId s) const = 0;

	/**
	 * @param s id of state to dump
	 * @param o output stream to dump information to
	 **/
	virtual void dumpState(Fsa::StateId s, std::ostream &o) const
	{
		o << s;
	}
	virtual size_t getMemoryUsed() const
	{
		return 0;
	}
	virtual void dumpMemoryUsage(Core::XmlWriter &o) const
	{
	}
	virtual std::string describe() const = 0;
};

/**
 **/
template<class _AutomatonFrom, class _AutomatonTo> class WrapperAutomaton :
	public _AutomatonTo
{
	typedef _AutomatonTo Precursor;
public:
	// from
	typedef typename _AutomatonFrom::Weight _FromWeight;
	typedef typename _AutomatonFrom::Arc _FromArc;
	typedef typename _AutomatonFrom::State _FromState;
	typedef typename _AutomatonFrom::Semiring _FromSemiring;
	typedef Core::Ref<const _AutomatonFrom> _ConstFromAutomatonRef;
	typedef Core::Ref<const _FromState> _ConstFromStateRef;
	typedef Core::Ref<const _FromSemiring> _ConstFromSemiringRef;
	// to
	typedef typename _AutomatonTo::Weight _Weight;
	typedef typename _AutomatonTo::Arc _Arc;
	typedef typename _AutomatonTo::State _State;
	typedef typename _AutomatonTo::Semiring _Semiring;
	typedef Core::Ref<const _AutomatonTo> _ConstAutomatonRef;
	typedef Core::Ref<const _State> _ConstStateRef;
	typedef Core::Ref<const _Semiring> _ConstSemiringRef;
protected:
	_ConstFromAutomatonRef fsa_;
protected:
	void setMaster(_ConstFromAutomatonRef f);
public:
	WrapperAutomaton(_ConstFromAutomatonRef fsa,
			Fsa::Property mask = Fsa::PropertyAll);
	virtual ~WrapperAutomaton()
	{
	}
	virtual Fsa::Type type() const
	{
		return fsa_->type();
	}
	virtual Fsa::StateId initialStateId() const
	{
		return fsa_->initialStateId();
	}
	virtual Fsa::ConstAlphabetRef getInputAlphabet() const
	{
		return fsa_->getInputAlphabet();
	}
	virtual Fsa::ConstAlphabetRef getOutputAlphabet() const
	{
		return fsa_->getOutputAlphabet();
	}
	virtual size_t getMemoryUsed() const
	{
		return fsa_->getMemoryUsed();
	}
	virtual void dumpMemoryUsage(Core::XmlWriter &o) const
	{
		fsa_->dumpMemoryUsage(o);
	}
	virtual void dumpState(Fsa::StateId s, std::ostream &o) const
	{
		fsa_->dumpState(s, o);
	}
	;
	/*
	 virtual _ConstStateRef getState(Fsa::StateId s) const = 0;
	 virtual _ConstSemiringRef semiring() const = 0;
	 */
};
template<class _AutomatonFrom, class _AutomatonTo> WrapperAutomaton<
		_AutomatonFrom, _AutomatonTo>::WrapperAutomaton(
		Core::Ref<const _AutomatonFrom> f, Fsa::Property mask) :
	fsa_(f)
{
	this->unsetProperties(Fsa::PropertyAll);
	this->setProperties(mask & f->knownProperties(), mask & f->properties());
	setMaster(f);
}
template<class _AutomatonFrom, class _AutomatonTo> void WrapperAutomaton<
		_AutomatonFrom, _AutomatonTo>::setMaster(
		Core::Ref<const _AutomatonFrom> f)
{
}

/**
 **/
template<class _Automaton> class SlaveAutomaton : public _Automaton
{
	typedef _Automaton Precursor;
public:
	typedef typename _Automaton::State _State;
	typedef typename _Automaton::Semiring _Semiring;
	typedef Core::Ref<const _Automaton> _ConstAutomatonRef;
	typedef Core::Ref<const _State> _ConstStateRef;
	typedef Core::Ref<const _Semiring> _ConstSemiringRef;
protected:
	_ConstAutomatonRef fsa_;
protected:
	void setMaster(_ConstAutomatonRef f);

    void setFsa(_ConstAutomatonRef fsa, Fsa::Property mask = Fsa::PropertyAll) {
	    fsa_ = fsa;
	    Precursor::copyProperties(fsa, mask);
     }
    SlaveAutomaton() {}

    SlaveAutomaton(_ConstAutomatonRef fsa,
		   Fsa::Property mask = Fsa::PropertyAll);

public:
	virtual ~SlaveAutomaton()
	{}
	virtual Fsa::Type type() const
	{
		return fsa_->type();
	}
	virtual _ConstSemiringRef semiring() const
	{
		return fsa_->semiring();
	}
	virtual Fsa::StateId initialStateId() const
	{
		return fsa_->initialStateId();
	}
	virtual Fsa::ConstAlphabetRef getInputAlphabet() const
	{
		return fsa_->getInputAlphabet();
	}
	virtual Fsa::ConstAlphabetRef getOutputAlphabet() const
	{
		return fsa_->getOutputAlphabet();
	}
	virtual _ConstStateRef getState(Fsa::StateId s) const
	{
		return fsa_->getState(s);
	}
	virtual void dumpState(Fsa::StateId s, std::ostream &o) const
	{
		fsa_->dumpState(s, o);
	}
	;
	virtual size_t getMemoryUsed() const
	{
		return fsa_->getMemoryUsed();
	}
	virtual void dumpMemoryUsage(Core::XmlWriter &o) const
	{
		fsa_->dumpMemoryUsage(o);
	}
};
template<class _Automaton> SlaveAutomaton<_Automaton>::SlaveAutomaton(
		Core::Ref<const _Automaton> f, Fsa::Property mask)
{
	setFsa(f, mask);
	setMaster(f);
}
template<class _Automaton> void SlaveAutomaton<_Automaton>::setMaster(
		Core::Ref<const _Automaton> f)
{
}

/**
 **/
template<class _Automaton> class ModifyAutomaton :
	public SlaveAutomaton<_Automaton>
{
	typedef SlaveAutomaton<_Automaton> Precursor;
public:
	typedef typename _Automaton::State _State;
	typedef Core::Ref<const _Automaton> _ConstAutomatonRef;
	typedef Core::Ref<const _State> _ConstStateRef;
protected:
	mutable _ConstStateRef lastState_;
public:
	ModifyAutomaton(_ConstAutomatonRef f) :
		Precursor(f), lastState_(new _State())
	{
		this->setProperties(Fsa::PropertyStorage | Fsa::PropertyCached, 0);
	}
	virtual void modifyState(_State *sp) const = 0;
	virtual _ConstStateRef getState(Fsa::StateId s) const
	{
		_State *sp;
		if (lastState_->refCount() == 1) {
			sp = const_cast<_State*>(lastState_.get());
			*sp = *Precursor::fsa_->getState(s);
		} else {
			sp = new _State(*Precursor::fsa_->getState(s));
			lastState_ = _ConstStateRef(sp);
		}
		modifyState(sp);
		return lastState_;
	}
};
} // namespace Ftl

#include "tAutomaton.cc"

#endif // _T_FSA_AUTOMATON_HH
