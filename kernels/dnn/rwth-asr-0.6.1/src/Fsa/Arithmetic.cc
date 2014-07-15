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
#include "tArithmetic.hh"
#include "Arithmetic.hh"
#include "Automaton.hh"

namespace Fsa {
    ConstAutomatonRef collect(ConstAutomatonRef f, Weight value)
    { return Ftl::collect<Automaton>(f, value); }

    ConstAutomatonRef extend(ConstAutomatonRef f, Weight value)
    { return Ftl::extend<Automaton>(f, value); }

    struct Multiplier {
	f32 c;
	Multiplier(f32 c = 1.0) : c(c) {}
	Weight operator() (const Weight &w) const {
	    return Weight(f32(f32(w) * c));
	}
	std::string describe() const {
	    return Core::form("multiply(%f)", c);
	}
    };
    ConstAutomatonRef multiply(ConstAutomatonRef f, Weight value) {
	if ((f->semiring() != LogSemiring) && (f->semiring() != TropicalSemiring)) {
	    std::cerr << "input to multiply must be log or tropical semiring" << std::endl;
	    return ConstAutomatonRef();
	}
	return Ftl::modifyWeight<Automaton, Multiplier>(f, Multiplier(f32(value)));
    }

    struct ApplyExp {
	Weight operator() (const Weight &w) const {
	    return std::isinf(f32(w)) ? Weight(0.0) : Weight(exp(-f32(w)));
	}
	std::string describe() const {
	    return Core::form("expm");
	}
    };
    ConstAutomatonRef expm(ConstAutomatonRef f) {
	if ((f->semiring() != LogSemiring) && (f->semiring() != TropicalSemiring)) {
	    std::cerr << "input to exponentiate must be log or tropical semiring" << std::endl;
	    return ConstAutomatonRef();
	}
	return Ftl::modifyWeight<Automaton, ApplyExp>(f, ApplyExp());
    }
    struct ApplyLog {
	Weight operator() (const Weight &w) const {
	    return (f32(w) <= 0) ? Weight(Core::Type<f32>::max) : Weight(-log(f32(w)));
	}
	std::string describe() const {
	    return Core::form("logm");
	}
    };
    ConstAutomatonRef logm(ConstAutomatonRef f) {
	if ((f->semiring() != LogSemiring) && (f->semiring() != TropicalSemiring)) {
	    std::cerr << "input to log must be log or tropical semiring" << std::endl;
	    return ConstAutomatonRef();
	}
	return Ftl::modifyWeight<Automaton, ApplyLog>(f, ApplyLog());
    }


    class ExtendByFsaAutomaton : public ModifyAutomaton {
    private:
	ConstAutomatonRef fsa2_;
    public:
	ExtendByFsaAutomaton(ConstAutomatonRef f1, ConstAutomatonRef f2) :
	    ModifyAutomaton(f1), fsa2_(f2) {
	    require(f1->semiring() == f2->semiring());
	}
	virtual ~ExtendByFsaAutomaton() {}

	virtual void modifyState(State *sp) const {
	    ConstStateRef _sp2 = fsa2_->getState(sp->id());
	    require(sp->id() == _sp2->id() && sp->tags() == _sp2->tags());
	    require(sp->nArcs() == _sp2->nArcs());
	    if (sp->isFinal()) sp->weight_ = fsa_->semiring()->extend(sp->weight_, _sp2->weight_);
	    State::const_iterator a2 = _sp2->begin();
	    for (State::iterator a = sp->begin(); a != sp->end(); ++a, ++a2) {
		require(a->target() == a2->target());
		require(a->input() == a2->input() && a->output() == a2->output());
		a->weight_ = fsa_->semiring()->extend(a->weight(), a2->weight());
	    }
	}

	virtual std::string describe() const {
	    return Core::form("extend-by-fsa(%s,%s)",
			      fsa_->describe().c_str(), fsa2_->describe().c_str());
	}
    };

    ConstAutomatonRef extend(ConstAutomatonRef f1, ConstAutomatonRef f2)
    { return ConstAutomatonRef(new ExtendByFsaAutomaton(f1, f2)); }

    ConstAutomatonRef extendFinal(ConstAutomatonRef f, Weight value)
    { return Ftl::extendFinal<Automaton>(f, value); }

    struct GreaterEqual {
	f32 t;
	GreaterEqual(f32 _t) : t(_t) {}
	Weight operator() (const Weight &w) const {
	    return Weight(f32(f32(w) >= t));
	}
	std::string describe() const {
	    return Core::form("greater-equal(%f)", t);
	}
    };
    ConstAutomatonRef isGreaterEqual(ConstAutomatonRef f, Weight threshold) {
	if ((f->semiring() != LogSemiring) && (f->semiring() != TropicalSemiring)) {
	    std::cerr << "input for greater_equal must be log or tropical semiring" << std::endl;
	    return ConstAutomatonRef();
	}
	return Ftl::modifyWeight<Automaton, GreaterEqual>(f, GreaterEqual(f32(threshold)));
    }

} // namespace Fsa
