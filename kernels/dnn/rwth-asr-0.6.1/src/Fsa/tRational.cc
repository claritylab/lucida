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
#include <Core/ProgressIndicator.hh>

#include "tAutomaton.hh"
#include "tBasic.hh"
#include "tRational.hh"
#include "tDfs.hh"
#include "tStatic.hh"
#include "AlphabetUtility.hh"
#include "Types.hh"

namespace Ftl {
    template<class _Automaton> typename _Automaton::ConstRef identity(
	Fsa::ConstAlphabetRef ab, typename _Automaton::ConstSemiringRef sr) {
	StaticAutomaton<_Automaton> *result = new StaticAutomaton<_Automaton>;
	result->setType(Fsa::TypeTransducer);
	result->setProperties(Fsa::PropertyLinear | Fsa::PropertyAcyclic,
			      Fsa::PropertyNone);
	result->addProperties(Fsa::PropertySortedByInput
			      | Fsa::PropertySortedByOutput);
	result->setSemiring(sr);
	result->setInputAlphabet(ab);
	result->setOutputAlphabet(ab);

	typename _Automaton::State *st = result->newState();
	typename _Automaton::Weight one = sr->one();
	for (Fsa::Alphabet::const_iterator la = ab->begin(); la != ab->end(); ++la)
	    st->newArc(st->id(), one, la, la);
	result->setInitialStateId(st->id());
	result->setStateFinal(st);

	return typename _Automaton::ConstRef(result);
    }

    template<class _Automaton> class ClosureAutomaton :
	public ModifyAutomaton<_Automaton> {
	typedef ModifyAutomaton<_Automaton> Precursor;
    public:
	typedef typename _Automaton::State _State;
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
    private:
	Fsa::StateId initial_;
    public:
	ClosureAutomaton(_ConstAutomatonRef f) :
	    Precursor(f), initial_(f->initialStateId()) {
	}
	/*! \todo existing loop weight should be collected with final weight. */
	virtual void modifyState(_State *sp) const {
	    if (sp->isFinal() && sp->id() != initial_) {
		bool hasLoop = false;
		for (typename _State::iterator a = sp->begin(); a != sp->end(); ++a)
		    if (a->target() == initial_) {
			hasLoop = true;
			break;
		    }
		if (!hasLoop)
		    sp->newArc(initial_, sp->weight_, Fsa::Epsilon);
	    }
	}
	virtual std::string describe() const {
	    return "closure(" + Precursor::fsa_->describe() + ")";
	}
    };

    template<class _Automaton> typename _Automaton::ConstRef closure(
	typename _Automaton::ConstRef f) {
	return typename _Automaton::ConstRef(new ClosureAutomaton<_Automaton>(f));
    }

    template<class _Automaton> class KleeneClosureAutomaton :
	public SlaveAutomaton<_Automaton> {
	typedef SlaveAutomaton<_Automaton> Precursor;
    public:
	typedef typename _Automaton::State _State;
	typedef typename _Automaton::ConstStateRef _ConstStateRef;
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
    private:
	mutable bool initialIsFinal_;
    public:
	KleeneClosureAutomaton(_ConstAutomatonRef f) :
	    Precursor(closure(f)), initialIsFinal_(false) {
	}
	virtual Fsa::StateId initialStateId() const {
	    if (Precursor::fsa_->initialStateId() != Fsa::InvalidStateId) {
		_ConstStateRef
		    sp =
		    Precursor::fsa_->getState(Precursor::fsa_->initialStateId());
		if (sp->isFinal())
		    initialIsFinal_ = true;
	    }
	    if (initialIsFinal_)
		return Precursor::fsa_->initialStateId();
	    return 0;
	}
	virtual _ConstStateRef getState(Fsa::StateId s) const {
	    if (initialIsFinal_)
		return Precursor::fsa_->getState(s);
	    _State *sp;
	    if (s > 0) {
		sp = new _State(*Precursor::fsa_->getState(s - 1));
		sp->setId(s);
		for (typename _State::iterator a = sp->begin(); a != sp->end(); ++a)
		    ++a->target_;
	    } else {
		sp = new _State(0, Fsa::StateTagFinal, this->semiring()->one());
		sp->newArc(Precursor::fsa_->initialStateId() + 1,
			   this->semiring()->one(), Fsa::Epsilon);
	    }
	    return _ConstStateRef(sp);
	}
	virtual std::string describe() const {
	    return "kleeneClosure(" + Precursor::fsa_->describe() + ")";
	}
    };

    template<class _Automaton> typename _Automaton::ConstRef kleeneClosure(
	typename _Automaton::ConstRef f) {
	return typename _Automaton::ConstRef(new KleeneClosureAutomaton<_Automaton>(f));
    }

    template<class _Automaton> class ComplementAutomaton :
	public SlaveAutomaton<_Automaton> {
	typedef SlaveAutomaton<_Automaton> Precursor;
    public:
	typedef typename _Automaton::State _State;
	typedef typename _Automaton::ConstStateRef _ConstStateRef;
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
    private:
	_ConstStateRef superFinalState_;
    public:
	ComplementAutomaton(_ConstAutomatonRef f) :
	    Precursor(f) {
	    if (f->type() != Fsa::TypeAcceptor)
		std::cerr << "complement only defined for acceptors."
			  << std::endl;
	    _State *superFinalState = new _State(0, Fsa::StateTagFinal, Precursor::fsa_->semiring()->one());
	    superFinalState->newArc(superFinalState->id(),
				    Precursor::fsa_->semiring()->one(), Fsa::Any);
	    superFinalState_ = _ConstStateRef(superFinalState);
	}
	virtual Fsa::StateId initialStateId() const {
	    return Precursor::fsa_->initialStateId() + 1;
	}
	virtual _ConstStateRef getState(Fsa::StateId s) const {
	    if (s == 0)
		return superFinalState_;
	    _State *sp = new _State(*Precursor::fsa_->getState(s - 1));
	    sp->setId(s);
	    sp->setTags(sp->tags() ^ Fsa::StateTagFinal); // toggle final state flag
	    if (sp->isFinal())
		sp->weight_ = Precursor::fsa_->semiring()->one();
	    for (typename _State::iterator a = sp->begin(); a != sp->end(); ++a)
		a->target_ = a->target() + 1;
	    if (sp->nArcs() > 0)
		sp->newArc(0, Precursor::fsa_->semiring()->one(), Fsa::Failure);
	    else
		sp->newArc(0, Precursor::fsa_->semiring()->one(), Fsa::Any);
	    return _ConstStateRef(sp);
	}
	virtual std::string describe() const {
	    return "complement(" + Precursor::fsa_->describe() + ")";
	}
    };

    template<class _Automaton> typename _Automaton::ConstRef complement(
	typename _Automaton::ConstRef f) {
	return typename _Automaton::ConstRef(new ComplementAutomaton<_Automaton>(f));
    }

    template<class _Automaton> class ConcatUnionAutomaton : public _Automaton {
	typedef _Automaton Precursor;
    public:
	typedef typename _Automaton::ConstSemiringRef _ConstSemiringRef;
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
    protected:
	Fsa::Type type_;
	_ConstSemiringRef semiring_;
	Core::Vector<_ConstAutomatonRef> fsa_;
	Fsa::ConstAlphabetRef input_, output_;
	Core::Vector<Fsa::AlphabetMapping> inputMappings_, outputMappings_;

	/** Id of state in super-automaton. */
	Fsa::StateId superStateId(u32 automaton, Fsa::StateId subState) const {
	    return fsa_.size() * subState + automaton + 1;
	}

	/**
	 * Compute union of alphabets.
	 * If the first alphabet already contains all symbols, it is
	 * returned.  Otherwise a StaticAlphabet is created and all
	 * symbols from all alphabets are added to it.  In both cases
	 * a list of mapping is created, which map to the alphabet
	 * that is returned.
	 * @param alphabets input: a list of alphabets
	 * @param mappings output: a list of mappings that map each
	 * alphabet to the returned union alphabet.
	 * @return an alphabet containing the union of all symbols of
	 * all alphabets
	 **/
	static Fsa::ConstAlphabetRef uniteAlphabets(
	    const Core::Vector<Fsa::ConstAlphabetRef> &alphabets,
	    Core::Vector<Fsa::AlphabetMapping> &mappings) {
	    mappings.resize(alphabets.size());
	    Fsa::ConstAlphabetRef major = alphabets.front();
	    Core::Ref<Fsa::StaticAlphabet> merged;
	    for (u32 k = 0; k < alphabets.size(); ++k) {
		Fsa::ConstAlphabetRef alphabet(alphabets[k]);
		Fsa::AlphabetMapping &mapping(mappings[k]);
		mapAlphabet(alphabet, major, mapping, false);
		if (mapping.type() & Fsa::AlphabetMapping::typeComplete)
		    continue;
		if (!merged)
		    major = merged = staticCopy(major);
		// we should unset typeIdentity in all previous mappings
		for (Fsa::Alphabet::const_iterator j = alphabet->begin(); j
			 != alphabet->end(); ++j) {
		    if (mapping[j] == Fsa::InvalidLabelId)
			mapping.map(j, merged->addSymbol(*j));
		}
	    }
	    return major;
	}

    public:
	virtual Fsa::StateId initialStateId() const {
	    return 0;
	}

	ConcatUnionAutomaton(const Core::Vector<_ConstAutomatonRef> &fsa) :
	    fsa_(fsa) {
	    type_ = Fsa::TypeAcceptor;
	    for (typename Core::Vector<_ConstAutomatonRef>::const_iterator i =
		     fsa.begin(); i != fsa.end(); ++i) {
		if ((*i)->type() == Fsa::TypeTransducer)
		    type_ = Fsa::TypeTransducer;
	    }

	    this->setProperties(Fsa::PropertyStorage | Fsa::PropertyCached,
				Fsa::PropertyNone);

	    semiring_ = fsa_[0]->semiring();
	    for (typename Core::Vector<_ConstAutomatonRef>::const_iterator i =
		     fsa.begin(); i != fsa.end(); ++i)
		if (semiring_ != (*i)->semiring()) {
		    std::cerr
			<< "mixed semirings during union. continuing, but behaviour will be unpredictable."
			<< std::endl;
		    break;
		}

	    Core::Vector<Fsa::ConstAlphabetRef> alphabets;
	    for (typename Core::Vector<_ConstAutomatonRef>::const_iterator i =
		     fsa.begin(); i != fsa.end(); ++i)
		alphabets.push_back((*i)->getInputAlphabet());
	    input_ = uniteAlphabets(alphabets, inputMappings_);

	    alphabets.clear();
	    for (typename Core::Vector<_ConstAutomatonRef>::const_iterator i =
		     fsa.begin(); i != fsa.end(); ++i)
		alphabets.push_back((*i)->getOutputAlphabet());
	    output_ = uniteAlphabets(alphabets, outputMappings_);
	}

	virtual std::string name() const = 0;
	virtual Fsa::Type type() const {
	    return type_;
	}
	virtual _ConstSemiringRef semiring() const {
	    return semiring_;
	}
	virtual Fsa::ConstAlphabetRef getInputAlphabet() const {
	    return input_;
	}
	virtual Fsa::ConstAlphabetRef getOutputAlphabet() const {
	    return output_;
	}
	virtual void dumpState(Fsa::StateId s, std::ostream &o) const {
	    u32 k = subAutomaton(s);
	    o << k << ",";
	    fsa_[k]->dumpState(subStateId(s), o);
	}
	;
	virtual size_t getMemoryUsed() const {
	    size_t memoryUsed = 0;
	    for (typename Core::Vector<_ConstAutomatonRef>::const_iterator i =
		     fsa_.begin(); i != fsa_.end(); ++i)
		memoryUsed += (*i)->getMemoryUsed();
	    return memoryUsed;
	}
	virtual void dumpMemoryUsage(Core::XmlWriter &o) const {
	    o << Core::XmlOpen(name());
	    for (typename Core::Vector<_ConstAutomatonRef>::const_iterator i =
		     fsa_.begin(); i != fsa_.end(); ++i)
		(*i)->dumpMemoryUsage(o);
	    o << Core::XmlClose(name());
	}
	virtual std::string describe() const {
	    std::string result = name() + "(";
	    for (size_t i = 0; i < fsa_.size(); ++i) {
		if (i)
		    result += ",";
		result += fsa_[i]->describe();
	    }
	    result += ")";
	    return result;
	}
	// State ids are interleaved.  We reserve 0 for the initial state.
	/** Index of sub-automaton. */
	u32 subAutomaton(Fsa::StateId s) const {
	    return (s-1) % fsa_.size();
	}
	/** Id of state in sub-automaton. */
	Fsa::StateId subStateId(Fsa::StateId s) const {
	    return (s-1) / fsa_.size();
	}
    };

    template<class _Automaton> class ConcatAutomaton :
	public ConcatUnionAutomaton<_Automaton> {
	typedef ConcatUnionAutomaton<_Automaton> Precursor;
    public:
	typedef typename _Automaton::State _State;
	typedef typename _Automaton::ConstStateRef _ConstStateRef;
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
    public:
	ConcatAutomaton(const Core::Vector<_ConstAutomatonRef> &fsa) :
	    Precursor(fsa) {
	}
	virtual std::string name() const {
	    return "concat";
	}

	virtual _ConstStateRef getState(Fsa::StateId s) const {
	    _State *sp;
	    if (s != 0) {
		u32 k = this->subAutomaton(s);
		sp = new _State(*(Precursor::fsa_[k]->getState(this->subStateId(s))));
		sp->setId(s);
		const Fsa::AlphabetMapping &input(Precursor::inputMappings_[k]);
		const Fsa::AlphabetMapping &output(
		    Precursor::outputMappings_[k]);
		for (typename _State::iterator a = sp->begin(); a != sp->end(); ++a) {
		    a->target_ = Precursor::superStateId(k, a->target_);
		    a->input_ = input[a->input_];
		    a->output_ = output[a->output_];
		}
		if ((k < Precursor::fsa_.size() - 1) && (sp->isFinal())) {
		    sp->newArc(Precursor::superStateId(k+1,
					    Precursor::fsa_[k + 1]->initialStateId()),
			       sp->weight_, Fsa::Epsilon);
		    sp->unsetFinal();
		}
	    } else {
		sp = new _State(s);
		sp->newArc(
		    Precursor::superStateId(0, Precursor::fsa_[0]->initialStateId()),
		    Precursor::semiring_->one(), Fsa::Epsilon);
	    }
	    return _ConstStateRef(sp);
	}
    };

    template<class _Automaton> typename _Automaton::ConstRef concat(
	const Core::Vector<typename _Automaton::ConstRef> &f) {
	if (f.size() > 1)
	    return typename _Automaton::ConstRef(new ConcatAutomaton<_Automaton>(f));
	if (f.size() == 1)
	    return f[0];
	return typename _Automaton::ConstRef(
	    );
    }

    template<class _Automaton> typename _Automaton::ConstRef concat(
	typename _Automaton::ConstRef f1, typename _Automaton::ConstRef f2) {
	Core::Vector<typename _Automaton::ConstRef> f;
	f.push_back(f1);
	f.push_back(f2);
	return ref(new ConcatAutomaton<_Automaton>(f));
    }

    template<class _Automaton>
    class UnionAutomaton : public ConcatUnionAutomaton<_Automaton> {
	typedef ConcatUnionAutomaton<_Automaton> Precursor;
    public:
	typedef typename _Automaton::State _State;
	typedef typename _Automaton::ConstStateRef _ConstStateRef;
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
    private:
	Core::Vector<typename _Automaton::Weight> initialWeights_;
    public:
	UnionAutomaton(const Core::Vector<_ConstAutomatonRef> &fsa,
		       const Core::Vector<typename _Automaton::Weight> &initialWeights) :
	    Precursor(fsa), initialWeights_(initialWeights) {
	    if (initialWeights_.empty())
		initialWeights_.resize(fsa.size(), Precursor::semiring_->one());
	    else
		verify(initialWeights_.size() == fsa.size());
	}
	virtual std::string name() const { return "unite"; }

	virtual _ConstStateRef getState(Fsa::StateId s) const {
	    _State *sp;
	    if (s != 0) {
		u32 k = this->subAutomaton(s);
		sp = new _State(*(Precursor::fsa_[k]->getState(this->subStateId(s))));
		sp->setId(s);
		const Fsa::AlphabetMapping &input(Precursor::inputMappings_[k]);
		const Fsa::AlphabetMapping &output(Precursor::outputMappings_[k]);
		for (typename _State::iterator a = sp->begin(); a != sp->end(); ++a) {
		    a->target_ = Precursor::superStateId(k, a->target_);
		    a->input_  = input[a->input_];
		    a->output_ = output[a->output_];
		}
	    } else {
		sp = new _State(s);
		for (u32 k = 0; k < Precursor::fsa_.size(); ++k)
		    sp->newArc(
			Precursor::superStateId(k, Precursor::fsa_[k]->initialStateId()),
			initialWeights_[k], Fsa::Epsilon);
	    }
	    return _ConstStateRef(sp);
	}
    };

    template<class _Automaton>
    typename _Automaton::ConstRef unite(const Core::Vector<typename _Automaton::ConstRef> &f,
					const Core::Vector<typename _Automaton::Weight> &initialWeights) {
	if (f.size()  >= 1)
	    return typename _Automaton::ConstRef(new UnionAutomaton<_Automaton>(f, initialWeights));
	// if (f.size() == 1) return f[0];
	return typename _Automaton::ConstRef();
    }

    template<class _Automaton> typename _Automaton::ConstRef unite(
	typename _Automaton::ConstRef f1, typename _Automaton::ConstRef f2) {
	Core::Vector<typename _Automaton::ConstRef> f;
	f.push_back(f1);
	f.push_back(f2);
	return unite(f);
    }

    template<class _Automaton>
    class UniteMapping : public Fsa::Mapping {
    public:
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
	typedef ConcatUnionAutomaton<_Automaton> _UniteAutomaton;
    protected:
	// weak reference!
	//	_ConstAutomatonRef fsa_;
	const _UniteAutomaton *uFsa_;
	u32 subAutomaton_;
    public:
	UniteMapping(_ConstAutomatonRef f, u32 subAutomaton) : uFsa_(dynamic_cast<const _UniteAutomaton*>(f.get())), subAutomaton_(subAutomaton) {
	    require(uFsa_);
	}
	virtual ~UniteMapping() {}
	virtual Fsa::StateId map(Fsa::StateId target) const {
	    return uFsa_->subAutomaton(target) == subAutomaton_ ? uFsa_->subStateId(target) : Fsa::InvalidStateId;
	}
    };

    template<class _Automaton>
    Fsa::ConstMappingRef mapToSubAutomaton(typename _Automaton::ConstRef f, u32 subAutomaton) {
	return Fsa::ConstMappingRef(new UniteMapping<_Automaton>(f, subAutomaton));
    }

    template<class _Automaton> class FusionAutomaton :
	public ConcatUnionAutomaton<_Automaton> {
	typedef ConcatUnionAutomaton<_Automaton> Precursor;
    public:
	typedef typename _Automaton::State State;
	typedef typename _Automaton::ConstStateRef ConstStateRef;
	typedef typename _Automaton::ConstRef ConstAutomatonRef;

    public:
	FusionAutomaton(const Core::Vector<ConstAutomatonRef> &fsa) :
	    Precursor(fsa) {
	}
	virtual std::string name() const {
	    return "fuse";
	}

	virtual ConstStateRef getState(Fsa::StateId s) const {
	    State *sp;
	    if (s != 0) {
		u32 k = Precursor::subAutomaton(s);
		sp = new State(*(Precursor::fsa_[k]->getState(Precursor::subStateId(s))));
		sp->setId(s);
		const Fsa::AlphabetMapping &input(Precursor::inputMappings_[k]);
		const Fsa::AlphabetMapping &output(
		    Precursor::outputMappings_[k]);
		for (typename State::iterator ka = sp->begin(); ka != sp->end(); ++ka) {
		    if (ka->target_ == Precursor::fsa_[k]->initialStateId())
			ka->target_ = Precursor::initialStateId();
		    else
			ka->target_ = Precursor::superStateId(k, ka->target_);
		    ka->input_ = input[ka->input_];
		    ka->output_ = output[ka->output_];
		}
		hope(!sp->isFinal()); //Patrick Lehnen: "Why do we hope this? In UnionAutomaton this hope is not set."
	    } else {
		sp = new State(s);
		for (u32 k = 0; k < Precursor::fsa_.size(); ++k) {
		    const Fsa::AlphabetMapping &input(
			Precursor::inputMappings_[k]);
		    const Fsa::AlphabetMapping &output(
			Precursor::outputMappings_[k]);
		    ConstStateRef
			ks =
			Precursor::fsa_[k]->getState(Precursor::fsa_[k]->initialStateId());
		    for (typename State::const_iterator ka = ks->begin(); ka
			     != ks->end(); ++ka) {
			sp->newArc(Precursor::superStateId(k, ka->target()), ka->weight(),
				   input[ka->input_], output[ka->output_]);
		    }
		    if (ks->isFinal()) {
			if (sp->isFinal())
			    sp->weight_ = Precursor::semiring_->collect(
				sp->weight(), ks->weight());
			else
			    sp->setFinal(ks->weight());
		    }
		}
	    }
	    return ConstStateRef(sp);
	}
    };

    template<class _Automaton> typename _Automaton::ConstRef fuse(
	const Core::Vector<typename _Automaton::ConstRef> &f) {
	if (f.size() > 1)
	    return typename _Automaton::ConstRef(new FusionAutomaton<_Automaton>(f));
	if (f.size() == 1)
	    return f[0];
	return typename _Automaton::ConstRef();
    }

    template<class _Automaton> typename _Automaton::ConstRef fuse(
	typename _Automaton::ConstRef f1, typename _Automaton::ConstRef f2) {
	Core::Vector<typename _Automaton::ConstRef> f;
	f.push_back(f1);
	f.push_back(f2);
	return ref(new FusionAutomaton<_Automaton>(f));
    }

    /*
     * transpose
     *
     * the input fsa accepts string a1 ... aN with a cost w iff the output fsa
     * accepts aN ... a1 with the same cost. the output fsa is trimmed automatically
     * input state ids are retained.
     */
    template<class _Automaton> class TransposeDfsState :
	public DfsState<_Automaton> {
	typedef DfsState<_Automaton> Precursor;
    public:
	typedef typename _Automaton::Arc _Arc;
	typedef typename _Automaton::State _State;
	typedef typename _Automaton::StateRef _StateRef;
	typedef typename _Automaton::ConstStateRef _ConstStateRef;
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
    private:
	StaticAutomaton<_Automaton> *fsa_;
	std::vector<_ConstStateRef> finals_;

    public:
	TransposeDfsState(_ConstAutomatonRef f) :
	    Precursor(f) {
	    fsa_ = new StaticAutomaton<_Automaton>;
	    fsa_->setType(f->type());
	    Fsa::Property prop = f->knownProperties();
	    fsa_->setProperties(prop, f->properties());
	    fsa_->setSemiring(f->semiring());
	    fsa_->setInputAlphabet(f->getInputAlphabet());
	    if (f->type() == Fsa::TypeTransducer)
		fsa_->setOutputAlphabet(f->getOutputAlphabet());
	}
	StaticAutomaton<_Automaton>* result() {
	    return fsa_;
	}
	const std::vector<_ConstStateRef>& finals() const {
	    return finals_;
	}

	_StateRef addState(Fsa::StateId s) {
	    _StateRef sp = fsa_->state(s);
	    if (!sp) {
		_State *sp = new _State(s);
		fsa_->setState(sp);
		return _StateRef(sp);
	    }
	    return sp;
	}
	virtual void discoverState(_ConstStateRef sp) {
	    _StateRef nsp = addState(sp->id());
	    Fsa::StateTag tags = sp->tags();
	    if (tags & Fsa::StateTagFinal) {
		tags &= ~Fsa::StateTagFinal;
		finals_.push_back(sp);
	    }
	    if (sp->id() == Precursor::fsa_->initialStateId()) {
		tags |= Fsa::StateTagFinal;
		nsp->weight_ = fsa_->semiring()->one();
	    }
	    nsp->setTags(tags);
	    for (typename _State::const_iterator a = sp->begin(); a
		     != sp->end(); ++a) {
		_Arc *arc = addState(a->target())->newArc();
		*arc = *a;
		arc->target_ = sp->id();
	    }
	}
	virtual std::string describe() const {
	    return "transpose(" + fsa_->describe() + ")";
	}
    };

    template<class _Automaton> typename _Automaton::ConstRef transpose(
	typename _Automaton::ConstRef f, bool progress) {
	typedef typename _Automaton::State _State;
	typedef typename _Automaton::ConstStateRef _ConstStateRef;
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
	require(f);
	Core::ProgressIndicator *p = 0;
	if (progress)
	    p = new Core::ProgressIndicator("transposing", "states");
	TransposeDfsState<_Automaton> s(f);
	s.dfs(p);
	if (p)
	    delete p;
	StaticAutomaton<_Automaton> *nf = s.result();

	if (s.finals().size() == 0) {
	    nf->setInitialStateId(Fsa::InvalidStateId);
	    nf->clear();
	} else {
	    _State *initial = nf->newState();
	    nf->setInitialStateId(initial->id());
	    for (typename std::vector<_ConstStateRef>::const_iterator sp =
		     s.finals().begin(); sp != s.finals().end(); ++sp)
		initial->newArc((*sp)->id(), (*sp)->weight_, Fsa::Epsilon);
	}
	return _ConstAutomatonRef(nf);
    }
} // namespace Ftl
