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
#include "FlfCore/Basic.hh"
#include "FlfCore/Traverse.hh"
#include "Copy.hh"

namespace Flf {

    // -------------------------------------------------------------------------
    namespace {
		void copyStub(ConstLatticeRef l, StaticLattice *staticLattice, StaticBoundaries *staticBoundaries, const std::string &desc) {
			staticLattice->clear();
			staticLattice->setDescription(desc);
			staticLattice->setType(l->type());
			staticLattice->setProperties(l->knownProperties(), l->properties());
			staticLattice->setInputAlphabet(l->getInputAlphabet());
			if (l->type() != Fsa::TypeAcceptor)
				staticLattice->setOutputAlphabet(l->getOutputAlphabet());
			staticLattice->setSemiring(l->semiring());
			if (staticBoundaries)
				staticBoundaries->clear();
		}
    } // namespace
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    // copy by reference, i.e. the states are made persistent
    struct CopyStateByReference {
		CopyStateByReference(ConstSemiringRef semiring) {}
		State * operator() (ConstStateRef sr) const {
			return const_cast<State*>(sr.get());
		}
		std::string describe(ConstLatticeRef l) const {
			return Core::form("persistent(%s)", l->describe().c_str());
		}
    };

    // (flat) copy by value, i.e. state is copied, but no weights
    struct CopyStateByValue {
		CopyStateByValue(ConstSemiringRef semiring) {}
		State * operator() (ConstStateRef sr) const {
			return new State(*sr);
		}
		std::string describe(ConstLatticeRef l) const {
			return Core::form("copy(%s)", l->describe().c_str());
		}
    };

    // deep copy by value, i.e. state and weights are copied
    struct DeepCopyStateByValue {
		ConstSemiringRef semiring;
		DeepCopyStateByValue(ConstSemiringRef semiring) : semiring(semiring) {}
		State * operator() (ConstStateRef sr) const {
			State *sp = new State(*sr);
			if (sr->isFinal())
				sp->setWeight(semiring->clone(sr->weight()));
			for (State::iterator a = sp->begin(); a != sp->end(); ++a)
				a->setWeight(semiring->clone(a->weight()));
			return sp;
		}
		std::string describe(ConstLatticeRef l) const {
			return Core::form("deepCopy(%s)", l->describe().c_str());
		}
    };

    template<class CopyState>
    class CopyBuilder : public TraverseState {
		typedef TraverseState Precursor;
    private:
		ConstLatticeRef lattice_;
		ConstBoundariesRef boundaries_;
		StaticLattice *staticLattice_;
		StaticBoundaries *staticBoundaries_;
		CopyState copyState_;
    public:
		CopyBuilder(ConstLatticeRef l, StaticLattice *staticLattice, StaticBoundaries *staticBoundaries, const CopyState &copyState) :
			Precursor(l),
			lattice_(l), boundaries_(l->getBoundaries()),
			staticLattice_(staticLattice), staticBoundaries_(staticBoundaries),
			copyState_(copyState) {
			copyStub(lattice_, staticLattice_, staticBoundaries_, copyState.describe(l));
			staticLattice_->setInitialStateId(l->initialStateId());
			staticLattice_->setTopologicalSort(l->getTopologicalSort());
			traverse();
		}
		virtual ~CopyBuilder() {}

		void exploreState(ConstStateRef sr) {
			staticLattice_->setState(copyState_(sr));
			if (staticBoundaries_)
				staticBoundaries_->set(sr->id(), boundaries_->get(sr->id()));
		}
    };

    template<class CopyState>
    std::pair<bool,bool> copy_(ConstLatticeRef l, StaticLattice *staticLattice, StaticBoundaries *staticBoundaries) {
		require(staticLattice);
		if (!l || (l->initialStateId() == Fsa::InvalidStateId))
			return std::make_pair(false, false);
		if (!l->getBoundaries()->valid())
			staticBoundaries = 0;
		CopyState copyState(l->semiring());
		CopyBuilder<CopyState> buildCopy(l, staticLattice, staticBoundaries, copyState);
		return staticBoundaries ? std::make_pair(true, true) : std::make_pair(true, false);
    }

    template<class CopyState>
    ConstLatticeRef copy_(ConstLatticeRef l) {
		if (!l || (l->initialStateId() == Fsa::InvalidStateId))
			return ConstLatticeRef();
		if (l->hasProperty(Fsa::PropertyStorage))
			return l;
		StaticLattice *staticLattice = new StaticLattice;
		StaticBoundaries *staticBoundaries = l->getBoundaries()->valid() ? new StaticBoundaries : 0;
		copy_<CopyState>(l, staticLattice, staticBoundaries);
		if (staticBoundaries)
			staticLattice->setBoundaries(ConstBoundariesRef(staticBoundaries));
		return ConstLatticeRef(staticLattice);
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    std::pair<bool,bool> persistent(ConstLatticeRef l, StaticLattice *staticLattice, StaticBoundaries *staticBoundaries) {
		return copy_<CopyStateByReference>(l, staticLattice, staticBoundaries);
    }

    std::pair<bool,bool> copy(ConstLatticeRef l, StaticLattice *staticLattice, StaticBoundaries *staticBoundaries) {
		return copy_<CopyStateByValue>(l, staticLattice, staticBoundaries);
    }

    std::pair<bool,bool> deepCopy(ConstLatticeRef l, StaticLattice *staticLattice, StaticBoundaries *staticBoundaries) {
		return copy_<DeepCopyStateByValue>(l, staticLattice, staticBoundaries);
    }

    ConstLatticeRef persistent(ConstLatticeRef l) {
		return copy_<CopyStateByReference>(l);
    }

    ConstLatticeRef copy(ConstLatticeRef l) {
		return copy_<CopyStateByValue>(l);
    }

    ConstLatticeRef deepCopy(ConstLatticeRef l) {
		return copy_<DeepCopyStateByValue>(l);
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class CachingMapBuilder : public TraverseState {
		typedef TraverseState Precursor;
    private:
		StateIdList mapping_;
		ConstStateRefList states_;

    public:
		CachingMapBuilder(ConstLatticeRef l) :
			Precursor(l) {
			traverse();
		}
		virtual ~CachingMapBuilder() {}

    public:
		void exploreState(ConstStateRef sr) {
			mapping_.grow(sr->id());
			mapping_[sr->id()] = states_.size();
			states_.push_back(sr);
		}

		StateIdList & mapping() { return mapping_; }
		ConstStateRefList & states() { return states_; }
    };

    template <class CopyState>
    std::pair<bool,bool> normalizeCopy_(ConstLatticeRef l, StaticLattice *staticLattice, StaticBoundaries *staticBoundaries) {
		require(staticLattice);
		if (!l || (l->initialStateId() == Fsa::InvalidStateId))
			return std::make_pair(false, false);
		staticLattice->clear();
		if (staticBoundaries)
			staticBoundaries->clear();
		ConstBoundariesRef boundaries = l->getBoundaries();
		if (!boundaries->valid())
			staticBoundaries = 0;
		CopyState copyState(l->semiring());
		CachingMapBuilder stateMapping(l);
		StateIdList &mapping = stateMapping.mapping();
		copyStub(l, staticLattice, staticBoundaries, "normalized-" + copyState.describe(l));
		staticLattice->setInitialStateId(mapping[l->initialStateId()]);
		for (ConstStateRefList::reverse_iterator itSr = stateMapping.states().rbegin();
			 itSr != stateMapping.states().rend(); ++itSr) {
			Fsa::StateId oldSid = (*itSr)->id();
			State *sp = copyState(*itSr);
			sp->setId(mapping[oldSid]);
			for (State::iterator a = sp->begin(); a != sp->end(); ++a)
				a->setTarget(mapping[a->target()]);
			staticLattice->setState(sp);
			if (staticBoundaries)
				staticBoundaries->set(sp->id(), boundaries->get(oldSid));
		}
		return staticBoundaries ? std::make_pair(true, true) : std::make_pair(true, false);
    }

    template <class CopyState>
    ConstLatticeRef normalizeCopy_(ConstLatticeRef l) {
		if (!l || (l->initialStateId() == Fsa::InvalidStateId))
			return ConstLatticeRef();
		StaticLattice *staticLattice = new StaticLattice;
		StaticBoundaries *staticBoundaries = l->getBoundaries()->valid() ? new StaticBoundaries : 0;
		normalizeCopy_<CopyState>(l, staticLattice, staticBoundaries);
		verify(staticLattice->initialStateId() != Fsa::InvalidStateId);
		if (staticBoundaries)
			staticLattice->setBoundaries(ConstBoundariesRef(staticBoundaries));
		return ConstLatticeRef(staticLattice);
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    std::pair<bool,bool> normalizeCopy(ConstLatticeRef l, StaticLattice *staticLattice, StaticBoundaries *staticBoundaries) {
		return normalizeCopy_<CopyStateByValue>(l, staticLattice, staticBoundaries);
    }

    std::pair<bool,bool> normalizeDeepCopy(ConstLatticeRef l, StaticLattice *staticLattice, StaticBoundaries *staticBoundaries) {
		return normalizeCopy_<DeepCopyStateByValue>(l, staticLattice, staticBoundaries);
    }

    ConstLatticeRef normalizeCopy(ConstLatticeRef l) {
		return normalizeCopy_<CopyStateByValue>(l);
    }

    ConstLatticeRef normalizeDeepCopy(ConstLatticeRef l) {
		return normalizeCopy_<DeepCopyStateByValue>(l);
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class BoundariesCopyBuilder : public TraverseState {
		typedef TraverseState Precursor;
    private:
		ConstBoundariesRef boundaries_;
		StaticBoundaries *staticBoundaries_;

    public:
		BoundariesCopyBuilder(ConstLatticeRef l, StaticBoundaries *staticBoundaries) :
			Precursor(l), boundaries_(l->getBoundaries()), staticBoundaries_(staticBoundaries) {
			staticBoundaries->clear();
			traverse();
		}
		virtual ~BoundariesCopyBuilder() {}

		void exploreState(ConstStateRef sr) {
			staticBoundaries_->set(sr->id(), boundaries_->get(sr->id()));
		}
    };

    bool copyBoundaries(ConstLatticeRef l, StaticBoundaries *staticBoundaries) {
		require(staticBoundaries);
		if (!l || !l->getBoundaries()->valid()) return false;
		BoundariesCopyBuilder copyBoundariesBuilder(l, staticBoundaries);
		return true;
    }
    ConstLatticeRef copyBoundaries(ConstLatticeRef l) {
		if (!l)
			return ConstLatticeRef();
		if (!l->getBoundaries()->valid())
			return l;
		StaticBoundaries *b = new StaticBoundaries;
		copyBoundaries(l, b);
		l->setBoundaries(ConstBoundariesRef(b));
		return l;
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class CopyNode : public FilterNode {
    public:
		static const Core::ParameterBool paramDeepCopy;
		static const Core::ParameterBool paramTrim;
		static const Core::ParameterBool paramNormalize;
    private:
		bool deepCopy_;
		bool trim_;
		bool normalize_;
    protected:
		virtual ConstLatticeRef filter(ConstLatticeRef l) {
			if (!l || (l->initialStateId() == Fsa::InvalidStateId))
				return l;
			if (trim_ && normalize_) {
				StaticLatticeRef trimmedLattice = StaticLatticeRef(new StaticLattice);
				persistent(l, trimmedLattice.get(), 0);
				trimInPlace(trimmedLattice);
				trimmedLattice->setBoundaries(l->getBoundaries());
				l = deepCopy_ ? normalizeDeepCopy(trimmedLattice) : normalizeCopy(trimmedLattice);
			} else if (trim_) {
				StaticLatticeRef trimmedLattice = StaticLatticeRef(new StaticLattice);
				if (deepCopy_) deepCopy(l, trimmedLattice.get(), 0); else persistent(l, trimmedLattice.get(), 0);
				trimInPlace(trimmedLattice);
				trimmedLattice->setBoundaries(l->getBoundaries());
				l = copyBoundaries(trimmedLattice);
			} else if (normalize_) {
				l = deepCopy_ ? normalizeDeepCopy(l) : normalizeCopy(l);
			} else {
				l = deepCopy_ ? deepCopy(l) : persistent(l);
			}
			return l;
		}

    public:
		CopyNode(const std::string &name, const Core::Configuration &config) :
			FilterNode(name, config) {
			deepCopy_ = paramDeepCopy(config);
			trim_ = paramTrim(config);
			normalize_ = paramNormalize(config);
		}
		virtual ~CopyNode() {}
    };
    const Core::ParameterBool CopyNode::paramDeepCopy(
		"deep",
		"make a deep copy, i.e. copy weights",
		false);
    const Core::ParameterBool CopyNode::paramTrim(
		"trim",
		"trim lattice",
		false);
    const Core::ParameterBool CopyNode::paramNormalize(
		"normalize",
		"normalize lattice",
		false);
    NodeRef createCopyNode(const std::string &name, const Core::Configuration &config) {
		return NodeRef(new CopyNode(name, config));
    }
    // -------------------------------------------------------------------------

} // namespace Flf
