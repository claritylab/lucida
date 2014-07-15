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
#include <Core/Utility.hh>

#include "tSssp.hh"
#include "Properties.hh"
#include "Sssp.hh"
#include "Basic.hh"
#include "Arithmetic.hh"

namespace Fsa {
    StatePotentials sssp(ConstAutomatonRef f, StateId start,
			 const SsspArcFilter &arcFilter, bool progress)
    { return Ftl::sssp<Automaton>(f, start, arcFilter, progress); }

    StatePotentials sssp(ConstAutomatonRef f, bool progress)
    { return Ftl::sssp<Automaton>(f, progress); }

    ConstAutomatonRef pushToInitial(ConstAutomatonRef f, bool progress)
    { return Ftl::pushToInitial<Automaton>(f, progress); }

    ConstAutomatonRef pushToFinal(ConstAutomatonRef f, bool progress)
    { return Ftl::pushToFinal<Automaton>(f, progress); }

    ConstAutomatonRef posterior(ConstAutomatonRef f)
    { return Ftl::posterior<Automaton>(f); }

    ConstAutomatonRef posterior(ConstAutomatonRef f, const StatePotentials &forward)
    { return Ftl::posterior<Automaton>(f, forward); }

    ConstAutomatonRef posterior(ConstAutomatonRef f, Weight &totalInv)
    { return Ftl::posterior<Automaton>(f, totalInv); }

    size_t countPaths(ConstAutomatonRef f)
    { return Ftl::countPaths<Automaton>(f); }

}



#include "Dfs.hh"
#include "Rational.hh"
namespace Fsa {
    // numerically more stable version of posterior
    // i.e. -internal calculations with doubles
    //      -all incoming/outgoing arc weights are processed simultaneously
    // planned: batch collect

    const f64 Zero = Core::Type<f64>::max;

    class StatePotentials64DfsState : public DfsState {
    protected:
	StatePotentials64 &potentials_;
	// for debug purposes
	f64 totalFlow_;
    public:
	StatePotentials64DfsState(ConstAutomatonRef, StatePotentials64 &);
	virtual ~StatePotentials64DfsState() {}

	virtual void finishState(StateId s);
	f64 totalFlow() const { return totalFlow_; }
    };

    StatePotentials64DfsState::StatePotentials64DfsState(
	ConstAutomatonRef transposed, StatePotentials64 &potentials) :
	DfsState(transposed), potentials_(potentials), totalFlow_(0.0) {
	require(fsa_->semiring() == LogSemiring);
	potentials_.clear();
    }

    void StatePotentials64DfsState::finishState(StateId s) {
	// remember that fsa_ contains the transposed fsa
	ConstStateRef sp = fsa_->getState(s);
	ConstSemiringRef sr = fsa_->semiring();
	potentials_.grow(s, Zero);

#if 1
	f64 minimumScore = sp->isFinal() ? f32(sp->weight_) : Zero;
	State::const_iterator minimumIterator = sp->end();
	for (State::const_iterator a = sp->begin(); a != sp->end(); ++a) {
	    f64 score = potentials_[a->target()] + f32(a->weight());
	    if (std::isinf(score)) score = Core::Type<f64>::max;
	    if (score <= minimumScore) {
		minimumScore = score;
		minimumIterator = a;
	    }
	}
	f64 sum = (sp->isFinal() && minimumIterator != sp->end()) ? exp(minimumScore - f32(sp->weight_)) : 0;
	for (State::const_iterator a = sp->begin(); a != sp->end(); ++a) {
	    if (a != minimumIterator) {
		f64 score = potentials_[a->target()] + f32(a->weight());
		if (std::isinf(score)) score = Core::Type<f64>::max;
		sum += exp(minimumScore - score);
	    }
	}
	potentials_[s] = minimumScore - log1p(sum);
	potentials_[s] = potentials_[s] < Zero ? potentials_[s] : Zero;
#endif

	if (s == fsa_->initialStateId()) {
	    totalFlow_ = potentials_[s];
	}
    }

    void sssp64(ConstAutomatonRef f, StatePotentials64 &potentials) {
	require(f->semiring() == LogSemiring);
	require(hasProperties(f, PropertyAcyclic));
	StatePotentials64DfsState backward(f, potentials);
	backward.dfs();
    }

    class Posterior64Automaton : public ModifyAutomaton {
    protected:
	StatePotentials64 forwardPotentials_;
	StatePotentials64 backwardPotentials_;
	f64 totalInv_;
    protected:
	Posterior64Automaton(ConstAutomatonRef fsa) : ModifyAutomaton(fsa) {}
	f32 logPosterior(StateId s, const Arc &a) const {
	    f64 tmp = forwardPotentials_[s] + f32(a.weight()) + backwardPotentials_[a.target()] + totalInv_;
	    return tmp < Core::Type<f32>::max ? tmp : Core::Type<f32>::max;
	}
	f32 finalLogPosterior(const State *sp) const {
	    if (sp->nArcs() == 0) return 0;
	    f64 tmp = forwardPotentials_[sp->id()] + f32(sp->weight_) + totalInv_;
	    return tmp < Core::Type<f32>::max ? tmp : Core::Type<f32>::max;
	}
    public:
	Posterior64Automaton(ConstAutomatonRef f, bool normalize, s32 tol);
	virtual ~Posterior64Automaton() {}

	virtual void modifyState(State *sp) const;
	virtual std::string describe() const { return "posterior64(" + fsa_->describe() + ")"; }
	//	f64 totalInv() const { return totalInv_; }
	f64 totalInv() const { return -backwardPotentials_[fsa_->initialStateId()]; }
	//	Weight totalInv() const { return Weight(totalInv_ > Core::Type<f32>::min ? totalInv_ : Core::Type<f32>::min); }
    };

    Posterior64Automaton::Posterior64Automaton(ConstAutomatonRef fsa, bool normalize, s32 tol) : ModifyAutomaton(fsa) {
	ConstAutomatonRef transposed = transpose(fsa_);
	sssp64(transposed, forwardPotentials_);
	sssp64(fsa_, backwardPotentials_);
	totalInv_ = normalize ? -backwardPotentials_[fsa_->initialStateId()] : fsa_->semiring()->one();
	const f32 fwdFlow = (f32)forwardPotentials_[transposed->initialStateId()];
	const f32 bwdFlow = (f32)backwardPotentials_[fsa_->initialStateId()];
	if (!Core::isAlmostEqual(fwdFlow, bwdFlow, tol)) {
	    std::cerr << "forward-total-flow: " << fwdFlow
		      << " | backward-total-flow: " << bwdFlow << std::endl;
	}
    }

    void Posterior64Automaton::modifyState(State *sp) const {
	StateId s = sp->id();
	if (sp->isFinal()) {
	    sp->weight_ = Weight(finalLogPosterior(sp));
	}
	for (State::iterator a = sp->begin(); a != sp->end(); ++a) {
	    a->weight_ = Weight(logPosterior(s, *a));
	}
    }

    ConstAutomatonRef posterior64(ConstAutomatonRef fsa, Weight &totalInv, s32 tol) {
	Posterior64Automaton *f = new Posterior64Automaton(fsa, true, tol);
	totalInv = Fsa::Weight(f->totalInv() > Core::Type<f32>::min ? f->totalInv() : Core::Type<f32>::min);
	return ConstAutomatonRef(f);
    }

    ConstAutomatonRef posterior64(ConstAutomatonRef fsa, f64 &totalInv, s32 tol) {
	Posterior64Automaton *f = new Posterior64Automaton(fsa, true, tol);
	totalInv = f->totalInv();
	return ConstAutomatonRef(f);
    }

    ConstAutomatonRef posterior64(ConstAutomatonRef fsa, s32 tol) {
	Weight totalInv;
	return posterior64(fsa, totalInv, tol);
    }

    ConstAutomatonRef posterior64(ConstAutomatonRef fsa, Weight &totalInv, bool normalize, s32 tol) {
	Posterior64Automaton *f = new Posterior64Automaton(fsa, normalize, tol);
	totalInv = Fsa::Weight(f->totalInv() > Core::Type<f32>::min ? f->totalInv() : Core::Type<f32>::min);
	return ConstAutomatonRef(f);
    }

    // posterior with expectation semiring:
    // an fsa with risks is passed
    // it is assumed that both fsas are identical except for the weights
    // planned: use expectation semiring directly, batch collect
    class StatePotentialsEDfsState : public StatePotentials64DfsState {
    private:
	ConstAutomatonRef risk_;
	StatePotentials64 &generalized_;
    public:
	StatePotentialsEDfsState(
	    ConstAutomatonRef, ConstAutomatonRef, StatePotentials64&, StatePotentials64&);
	virtual ~StatePotentialsEDfsState() {}

	virtual void finishState(StateId s);
    };

    StatePotentialsEDfsState::StatePotentialsEDfsState(
	ConstAutomatonRef transposed, ConstAutomatonRef risk,
	StatePotentials64 &potentials, StatePotentials64 &generalized)
	:
	StatePotentials64DfsState(transposed, potentials),
	risk_(risk),
	generalized_(generalized)
    {
	require(fsa_->semiring() == LogSemiring);
	generalized_.clear();
    }

    void StatePotentialsEDfsState::finishState(StateId s) {
	// remember that fsa_/risk_ contain the transposed fsa
	ConstStateRef sp = fsa_->getState(s);
	potentials_.grow(s, Zero);
	f64 minWeight = sp->isFinal() ? f32(sp->weight_) : Zero;
	for (State::const_iterator a = sp->begin(); a != sp->end(); ++a) {
	    f64 weight = potentials_[a->target()] + f32(a->weight());
	    if (std::isinf(weight)) weight = Zero;
	    if (minWeight > weight) minWeight = weight;
	}
	ConstStateRef rp = risk_->getState(s);
	verify(rp);
	generalized_.grow(s, 0);
	f64 denSum = sp->isFinal() ? exp(minWeight - f32(sp->weight_)) : 0;
	f64 numSum = rp->isFinal() ? f32(rp->weight_) * denSum : 0;
	State::const_iterator r = rp->begin();
	verify(rp->nArcs() == sp->nArcs());
	for (State::const_iterator a = sp->begin(); a != sp->end(); ++a, ++r) {
	    f64 weight = potentials_[a->target()] + f32(a->weight());
	    if (std::isinf(weight)) weight = Zero;
	    const f64 p = exp(minWeight - weight);
	    numSum += p * (f32(r->weight()) + generalized_[r->target()]);
	    denSum += p;
	}
	if (denSum > 0) {
	    potentials_[s] = std::min(minWeight - log1p(denSum - 1), Zero);
	    generalized_[s] = std::max(std::min(numSum / denSum, Zero), -Zero);
	}
	if (s == fsa_->initialStateId()) {
	    totalFlow_ = generalized_[s];
	}
    }

    void ssspE(
	ConstAutomatonRef f, ConstAutomatonRef r,
	StatePotentials64 &potentials, StatePotentials64 &generalized)
    {
	require(f->semiring() == LogSemiring);
	require(hasProperties(f, PropertyAcyclic));
	StatePotentialsEDfsState backward(f, r, potentials, generalized);
	backward.dfs();
    }

    class PosteriorEAutomaton : public Posterior64Automaton {
    private:
	ConstAutomatonRef r_;
	StatePotentials64 generalizedForwardPotentials_;
	StatePotentials64 generalizedBackwardPotentials_;
	f64 normalization_;
    private:
	f32 risk(StateId s, const Arc &a) const {
	    f64 tmp = generalizedForwardPotentials_[s] + f32(a.weight()) + generalizedBackwardPotentials_[a.target()] - normalization_;
	    return tmp < Core::Type<f32>::max ? tmp : Core::Type<f32>::max;
	}
	f32 finalRisk(ConstStateRef sp) const {
	    if (sp->nArcs() == 0) return 0;
	    f64 tmp = f32(sp->weight_) - normalization_;
	    return tmp < Core::Type<f32>::max ? tmp : Core::Type<f32>::max;
	}
    public:
	PosteriorEAutomaton(ConstAutomatonRef, ConstAutomatonRef, bool vNormalized, s32 tol);
	virtual ~PosteriorEAutomaton() {}

	virtual void modifyState(State *sp) const;
	virtual std::string describe() const { return "posterior-with-expectation-semiring(" + fsa_->describe() + ")"; }

	Weight expectation() const { return Weight(generalizedBackwardPotentials_[fsa_->initialStateId()]); }
	Weight totalInv() const { return Fsa::Weight(totalInv_);}
    };

    PosteriorEAutomaton::PosteriorEAutomaton(
	ConstAutomatonRef f, ConstAutomatonRef r, bool vNormalized, s32 tol) : Posterior64Automaton(f), r_(r), normalization_(0)
    {
	ConstAutomatonRef transposed = transpose(fsa_);
	ssspE(transposed, transpose(r_), forwardPotentials_, generalizedForwardPotentials_);
	ssspE(fsa_, r_, backwardPotentials_, generalizedBackwardPotentials_);
	totalInv_ = -backwardPotentials_[fsa_->initialStateId()];
	const f32 fwdFlow = (f32)forwardPotentials_[transposed->initialStateId()];
	const f32 bwdFlow = (f32)backwardPotentials_[fsa_->initialStateId()];
	if (!Core::isAlmostEqualUlp(fwdFlow, bwdFlow, tol)) {
	    std::cerr << "forward-total-flow: " << fwdFlow
		      << " | backward-total-flow: " << bwdFlow << std::endl;
	}
	if (vNormalized) {
	    normalization_ = generalizedBackwardPotentials_[fsa_->initialStateId()];
	}
    }

    void PosteriorEAutomaton::modifyState(State *sp) const {
	StateId s = sp->id();
	ConstStateRef rp = r_->getState(s);
	if (sp->isFinal()) {
	    sp->weight_ = Weight(exp(-finalLogPosterior(sp)) * finalRisk(rp));
	}
	State::const_iterator r = rp->begin();
	for (State::iterator a = sp->begin(); a != sp->end(); ++a, ++r) {
	    a->weight_ = Weight(exp(-logPosterior(s, *a)) * risk(s, *r));
	}
    }

    ConstAutomatonRef posteriorE(
	ConstAutomatonRef f, ConstAutomatonRef r, Weight &expectation, bool vNormalized, s32 tol) {
	PosteriorEAutomaton *p = new PosteriorEAutomaton(f, r, vNormalized, tol);
	expectation = p->expectation();
	return ConstAutomatonRef(p);
    }


    ConstAutomatonRef posteriorE(
	ConstAutomatonRef f, ConstAutomatonRef r, Weight &expectation, Weight &totalInv, bool vNormalized, s32 tol) {
	PosteriorEAutomaton *p = new PosteriorEAutomaton(f, r, vNormalized, tol);
	expectation = p->expectation();
	totalInv = p->totalInv();
	return ConstAutomatonRef(p);
    }

    Weight expectation(ConstAutomatonRef f, ConstAutomatonRef r) {
	StatePotentials64 backwardPotentials;
	StatePotentials64 generalizedBackwardPotentials;
	StatePotentialsEDfsState backward(f, r, backwardPotentials, generalizedBackwardPotentials);
	backward.dfs();
	return Weight(generalizedBackwardPotentials[f->initialStateId()]);
    }

} // namespace Fsa
