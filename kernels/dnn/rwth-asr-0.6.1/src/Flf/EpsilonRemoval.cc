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
#include <Core/Application.hh>
#include <Fsa/Types.hh>
#include <Fsa/hSort.hh>

#include "FlfCore/Basic.hh"
#include "FlfCore/Ftl.hh"
#include "FlfCore/LatticeInternal.hh"
#include "FlfCore/TopologicalOrderQueue.hh"
#include "FlfCore/Traverse.hh"
#include "Cache.hh"
#include "EpsilonRemoval.hh"

namespace Flf {


    // -------------------------------------------------------------------------
    /**
     * Filter: true, if arc has to be removed
     *
     * WeakOrder: order arcs per state w.r.t. to matcher,
     * postcondition: for each state holds that a non-matching arc is never followed by a matching arc

    class ArcFilter {
    public:
	struct Filter {
	    Filter(ConstLatticeRef l, ConstStateRef sr);
	    bool operator()(const Arc &a) const;
	};
	struct WeakOrder {
	    WeakOrder(ConstLatticeRef l);
	    bool operator()(const Arc &a1, const Arc &a2) const;
	    static Fsa::Property properties();
	    static std::string describe();
	};
	static std::string describe();
    };

     **/


    template<class WeakOrder>
    class ArcSortLattice : public ModifyLattice {
    private:
	const WeakOrder weakOrder_;
    public:
	ArcSortLattice(ConstLatticeRef l) : ModifyLattice(l), weakOrder_(l) {
	    this->setProperties(Fsa::PropertySorted, WeakOrder::properties());
	}
	virtual ~ArcSortLattice() {}
	virtual void modifyState(State *sp) const { sp->sort(weakOrder_); }
	virtual std::string describe() const {
	    return "sort(" + fsa_->describe() + "," + WeakOrder::describe() + ")";
	}
    };


    template<class ArcFilter>
    class ArcRemovalLattice : public SlaveLattice {
    private:
	typedef typename ArcFilter::Filter Filter;
	typedef typename ArcFilter::WeakOrder WeakOrder;
	ConstSemiringRef semiring_;
	const WeakOrder weakOrder_;
	mutable TopologicalOrderQueueRef topQueue_;
	mutable Core::Vector<ScoresRef> closureScores_;

    public:
	ArcRemovalLattice(ConstLatticeRef l) :
	    SlaveLattice(cache(ConstLatticeRef(new ArcSortLattice<WeakOrder>(l)))),
	    semiring_(l->semiring()),
	    weakOrder_(l) {
	    ConstStateMapRef topologicalOrderMap = findTopologicalOrder(l);
	    verify(topologicalOrderMap && (topologicalOrderMap->maxSid != Fsa::InvalidStateId));
	    topQueue_ = createTopologicalOrderQueue(l, topologicalOrderMap);
	    closureScores_.grow(topologicalOrderMap->maxSid);
	}
	virtual ~ArcRemovalLattice() {}

	virtual ConstStateRef getState(Fsa::StateId sid) const {
	    ConstStateRef sr = fsa_->getState(sid);
	    Filter filter(fsa_, sr);

	    // Check in O(1), if matching arc(s) exist
	    if (!sr->hasArcs() || !filter(*sr->begin()))
		return sr;

	    // Initialize closure
	    verify(sid < closureScores_.size());
	    TopologicalOrderQueue &topQ = *topQueue_;
	    State *sp = new State(sr->id());
	    State::const_iterator a = sr->begin();
	    for (; (a != sr->end()) && filter(*a); ++a)
		if (!closureScores_[a->target()]) {
		    closureScores_[a->target()] = a->weight();
		    topQ.insert(a->target());
		} else
		    closureScores_[a->target()] =
			semiring_->collect(closureScores_[a->target()], a->weight());
	    for (; a != sr->end(); ++a)
		*sp->newArc() = *a;

	    // Process closure
	    ScoresRef finalWeight;
	    if (sr->isFinal()) {
		sp->addTags(Fsa::StateTagFinal);
		finalWeight = sr->weight();
	    } else
		finalWeight = semiring_->zero();
	    while (!topQ.empty()) {
		Fsa::StateId epsSid = topQ.top(); topQ.pop();
		ConstStateRef epsSr = fsa_->getState(epsSid);
		ScoresRef score = closureScores_[epsSid];
		closureScores_[epsSid].reset();
		if (epsSr->isFinal()) {
		    sp->addTags(Fsa::StateTagFinal);
		    finalWeight = semiring_->collect(
			finalWeight, semiring_->extend(score, epsSr->weight()));
		}
		State::const_iterator a = epsSr->begin(), a_end = epsSr->end();
		for (; (a != a_end) && (a->input() == Fsa::Epsilon) && (a->output() == Fsa::Epsilon); ++a)
		    if (!closureScores_[a->target()]) {
			closureScores_[a->target()] = semiring_->extend(score, a->weight());
			topQ.insert(a->target());
		    } else
			closureScores_[a->target()] =
			    semiring_->collect(
				closureScores_[a->target()], semiring_->extend(
				    score, a->weight()));
		for (; a != a_end; ++a) {
		    State::iterator pos = sp->lower_bound(*a, weakOrder_);
		    if ((pos == sp->end())
			|| (a->target() != pos->target())
			|| (a->input() != pos->input())
			|| (a->output() != pos->output())) {
			sp->insert(pos, *a)->weight_ =
			    semiring_->extend(score, a->weight());
		    } else
			pos->weight_ = semiring_->collect(
			    pos->weight(), semiring_->extend(score, a->weight()));
		}
	    }
	    if (sp->isFinal())
		sp->setWeight(finalWeight);
	    return ConstStateRef(sp);
	}
	virtual std::string describe() const {
	    return Core::form("%s(%s)", ArcFilter::describe().c_str(), fsa_->describe().c_str());
	}
    };
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class ArcRemovalNode : public FilterNode {
    public:
	static const Core::ParameterBool paramToLogSemiring;
	static const Core::ParameterFloat paramAlpha;
    private:
	bool toLogSemiring_;
	f32 alpha_;
	ConstSemiringRef lastSemiring_;
	ConstSemiringRef logSemiring_;
    protected:
	ConstLatticeRef preProcess(ConstLatticeRef l) {
	    ensure(l);
	    if (toLogSemiring_) {
		if (!lastSemiring_ || (lastSemiring_.get() != l->semiring().get())) {
		    lastSemiring_ = l->semiring();
		    logSemiring_ = toLogSemiring(lastSemiring_, alpha_);
		}
		l = changeSemiring(l, logSemiring_);
	    }
	    return l;
	}
	ConstLatticeRef postProcess(ConstLatticeRef l) {
	    ensure(l);
	    if (toLogSemiring_)
		l = changeSemiring(l, lastSemiring_);
	    return l;
	}
    public:
	ArcRemovalNode(const std::string &name, const Core::Configuration &config) :
	    FilterNode(name, config), toLogSemiring_(false) {}
	~ArcRemovalNode() {}
	virtual void init(const std::vector<std::string> &arguments) {
	    toLogSemiring_ = paramToLogSemiring(config);
	    if (toLogSemiring_) {
		alpha_ = paramAlpha(select("log-semiring"));
		log() << "Use log-semiring with alpha=" << alpha_;
	    }
	}
    };
    const Core::ParameterBool ArcRemovalNode::paramToLogSemiring(
	"log-semiring",
	"use log semiring",
	false);
   const Core::ParameterFloat ArcRemovalNode::paramAlpha(
       "alpha",
       "scale dimensions for posterior calculation",
       0.0);
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class EpsilonArcFilter {
    public:
	struct Filter {
	    Filter(ConstLatticeRef l, ConstStateRef sr) {}
	    bool operator()(const Arc &a) const {
		return (a.input() == Fsa::Epsilon) && (a.output() == Fsa::Epsilon);
	    }
	};
	struct WeakOrder {
	    WeakOrder(ConstLatticeRef l) {}
	    bool operator()(const Arc &a1, const Arc &a2) const {
		if (a1.input()  < a2.input())  return true;
		if (a1.input()  > a2.input())  return false;
		if (a1.output() < a2.output()) return true;
		if (a1.output() > a2.output()) return false;
		return a1.target() < a2.target();
	    }
	    static Fsa::Property properties() { return Fsa::PropertySortedByInputAndOutputAndTarget; }
	    static std::string describe() { return "byInputAndOutputAndTarget"; }
	};
	static std::string describe() { return "fast-remove-epsilons"; }
    };

    ConstLatticeRef fastRemoveEpsilons(ConstLatticeRef l) {
	verify_(l->hasProperty(Fsa::PropertyAcyclic));
	return ConstLatticeRef(new ArcRemovalLattice<EpsilonArcFilter>(l));
    }

    ConstLatticeRef removeEpsilons(ConstLatticeRef l) {
	return FtlWrapper::removeEpsilons(l);
    }

    class EpsilonRemovalNode : public ArcRemovalNode {
	typedef ArcRemovalNode Precursor;
    protected:
	virtual ConstLatticeRef filter(ConstLatticeRef l) {
	    if (!l)
		return ConstLatticeRef();
	    l = preProcess(l);
	    if (l->hasProperty(Fsa::PropertyAcyclic))
		l = fastRemoveEpsilons(l);
	    else
		l = removeEpsilons(l);
	    return postProcess(l);
	}
    public:
	EpsilonRemovalNode(const std::string &name, const Core::Configuration &config) :
	    Precursor(name, config) {}
	~EpsilonRemovalNode() {}
    };
    NodeRef createEpsilonRemovalNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new EpsilonRemovalNode(name, config));
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class NullArcFilter {
    public:
	struct Filter {
	    ConstLatticeRef l;
	    const Boundaries &boundaries;
	    const Time t;
	    Filter(ConstLatticeRef l, ConstStateRef sr) :
		l(l), boundaries(*l->getBoundaries()), t(l->getBoundaries()->get(sr->id()).time()) {}
	    bool operator()(const Arc &a) const {
		if (boundaries.get(a.target()).time() == t) {
		    if ((a.input() != Fsa::Epsilon) || (a.output() != Fsa::Epsilon))
			Core::Application::us()->warning(
			    "Remove null-length arc with non-epsilon input or output label.");
		    return true;
		} else
		    return false;
	    }
	};
	struct WeakOrder {
	    ConstLatticeRef l;
	    const Boundaries &boundaries;
	    Time t;
	    WeakOrder(ConstLatticeRef l) : l(l), boundaries(*l->getBoundaries()) {}
	    bool operator()(const Arc &a1, const Arc &a2) const {
		Time t1 = boundaries.get(a1.target()).time(), t2 = boundaries.get(a1.target()).time();
		if (t1  < t2)  return true;
		if (t1  > t2)  return false;
		if (a1.target() < a2.target()) return true;
		if (a1.target() > a2.target()) return false;
		if (a1.input()  < a2.input())  return true;
		if (a1.input()  > a2.input())  return false;
		return a1.output() < a2.output();
	    }
	    static Fsa::Property properties() { return Fsa::PropertyNone; }
	    static std::string describe() { return "byTargetTimeAndTargetAndInputAndOutput"; }
	};
	static std::string describe() { return "fast-remove-null-arcs"; }
    };

    ConstLatticeRef fastRemoveNullArcs(ConstLatticeRef l) {
	verify_(l->hasProperty(Fsa::PropertyAcyclic));
	return ConstLatticeRef(new ArcRemovalLattice<NullArcFilter>(l));
    }

    class NullArcsRemovalNode : public ArcRemovalNode {
	typedef ArcRemovalNode Precursor;
    protected:
	virtual ConstLatticeRef filter(ConstLatticeRef l) {
	    if (!l)
		return ConstLatticeRef();
	    return postProcess(fastRemoveNullArcs(preProcess(l)));
	}
    public:
	NullArcsRemovalNode(const std::string &name, const Core::Configuration &config) :
	    ArcRemovalNode(name, config) {}
	~NullArcsRemovalNode() {}
    };
    NodeRef createNullArcsRemovalNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new NullArcsRemovalNode(name, config));
    }
    // -------------------------------------------------------------------------

} // namespace Flf
