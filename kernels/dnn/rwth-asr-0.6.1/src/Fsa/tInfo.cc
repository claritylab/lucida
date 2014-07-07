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
#include <Core/Choice.hh>
#include <Core/ProgressIndicator.hh>

#include "AlphabetUtility.hh"
#include "tDfs.hh"
#include "tInfo.hh"

namespace Ftl {
	template<class _Automaton> class CountDfsState :
			public DfsState<_Automaton>, public Fsa::AutomatonCounts {
		typedef DfsState<_Automaton> Precursor;
		public:
		typedef typename _Automaton::State State;
		typedef typename _Automaton::ConstStateRef ConstStateRef;
		typedef typename _Automaton::ConstRef ConstAutomatonRef;
	public:
		CountDfsState(ConstAutomatonRef f) :
			Precursor(f) {
		}
		virtual void discoverState(ConstStateRef sp) {
			if (sp->isFinal())
				++nFinals_;
			++nStates_;
			if ((sp->id() > maxStateId_)
					|| (maxStateId_ == Fsa::InvalidStateId))
				maxStateId_ = sp->id();
			for (typename State::const_iterator a = sp->begin(); a != sp->end(); ++a) {
				++nArcs_;
				if (a->input() == Fsa::Epsilon)
					++nIEps_;
				if (a->output() == Fsa::Epsilon)
					++nOEps_;
				if ((a->input() == Fsa::Epsilon) && (a->output()
						== Fsa::Epsilon))
					++nIoEps_;
				if (a->input() == Fsa::Failure)
					++nIFail_;
				if (a->output() == Fsa::Failure)
					++nOFail_;
				if ((a->input() == Fsa::Failure) && (a->output()
						== Fsa::Failure))
					++nIoFail_;
			}
		}
	};

	template<class _Automaton> Fsa::AutomatonCounts count(
			typename _Automaton::ConstRef f, bool progress) {
		Core::ProgressIndicator *p = 0;
		if (progress)
			p = new Core::ProgressIndicator("counting", "states");
		CountDfsState<_Automaton> v(f);
		v.dfs(p);
		if (p)
			delete p;
		return v;
	}

	template<class _Automaton> bool isEmpty(typename _Automaton::ConstRef f) {
		CountDfsState<_Automaton> v(f);
		v.dfs(0);
		return v.nFinals_ == 0;
	}

	template<class _Automaton> class InDegreeDfsState :
			public DfsState<_Automaton> {
	private:
		typedef DfsState<_Automaton> Precursor;
		typedef typename _Automaton::Arc Arc;
		typedef typename _Automaton::ConstStateRef ConstStateRef;
		typedef typename _Automaton::ConstRef ConstAutomatonRef;
		Core::Vector<u32> nInDegree_;

	public:
		InDegreeDfsState(ConstAutomatonRef f) :
			Precursor(f) {
		}
		const Core::Vector<u32>& nInDegree() const {
			return nInDegree_;
		}
		virtual void exploreTreeArc(ConstStateRef from, const Arc &a) {
			nInDegree_.grow(a.target(), 0);
			++nInDegree_[a.target()];
		}
		virtual void exploreNonTreeArc(ConstStateRef from, const Arc &a) {
			nInDegree_.grow(a.target(), 0);
			++nInDegree_[a.target()];
		}
	};

	template<class _Automaton> class CountLinearStatesDfsState :
			public DfsState<_Automaton> {
	private:
		typedef DfsState<_Automaton> Precursor;
		typedef typename _Automaton::ConstStateRef ConstStateRef;
		typedef typename _Automaton::ConstRef ConstAutomatonRef;
		size_t nLinearStates_;
		const Core::Vector<u32> &nInDegree_;

	public:
		CountLinearStatesDfsState(ConstAutomatonRef f,
				const Core::Vector<u32> &nInDegree) :
			Precursor(f), nLinearStates_(0), nInDegree_(nInDegree) {
		}
		size_t nLinearStates() const {
			return nLinearStates_;
		}
		virtual void discoverState(ConstStateRef sp) {
			if (nInDegree_[sp->id()] == 1)
				++nLinearStates_;
		}
	};

	template<class _Automaton> size_t countLinearStates(
			typename _Automaton::ConstRef f, bool progress) {
		Core::ProgressIndicator *p = 0;
		if (progress)
			p = new Core::ProgressIndicator("counting linear", "states");
		CountLinearStatesDfsState<_Automaton> v(f, inDegree<_Automaton>(f,
				progress));
		v.dfs(p);
		if (p)
			delete p;
		return v.nLinearStates();
	}

	template<class _Automaton> Core::Vector<u32> inDegree(
			typename _Automaton::ConstRef f, bool progress) {
		Core::ProgressIndicator *p = 0;
		if (progress)
			p = new Core::ProgressIndicator("calc. in-degree", "states");
		InDegreeDfsState<_Automaton> inDegree(f);
		inDegree.dfs(p);
		return inDegree.nInDegree();
	}

	template<class _Automaton> class CountInputDfsState :
			public DfsState<_Automaton> {
		typedef DfsState<_Automaton> Precursor;
	public:
		typedef typename _Automaton::State State;
		typedef typename _Automaton::ConstStateRef ConstStateRef;
		typedef typename _Automaton::ConstRef ConstAutomatonRef;
	private:
		size_t nArcs_, nMatched_, nEpsilons_;
		Fsa::LabelId label_;
	public:
		CountInputDfsState(ConstAutomatonRef f, Fsa::LabelId label) :
			Precursor(f), nArcs_(0), nMatched_(0), nEpsilons_(0), label_(label) {
		}
		virtual void discoverState(ConstStateRef sp) {
			for (typename State::const_iterator a = sp->begin(); a != sp->end(); ++a) {
				++nArcs_;
				if (a->input() == label_)
					++nMatched_;
				if (a->input() == Fsa::Epsilon)
					++nEpsilons_;
			}
		}
		size_t matched() const {
			return nMatched_;
		}
	};

	template<class _Automaton> size_t countInput(
			typename _Automaton::ConstRef f, Fsa::LabelId label, bool progress) {
		Core::ProgressIndicator *p = 0;
		if (progress)
			p = new Core::ProgressIndicator("counting", "states");
		CountInputDfsState<_Automaton> v(f, label);
		v.dfs(p);
		if (p)
			delete p;
		return v.matched();
	}

	template<class _Automaton> class CountOutputDfsState :
			public DfsState<_Automaton> {
		typedef DfsState<_Automaton> Precursor;
	public:
		typedef typename _Automaton::State State;
		typedef typename _Automaton::ConstStateRef ConstStateRef;
		typedef typename _Automaton::ConstRef ConstAutomatonRef;
	private:
		size_t nArcs_, nMatched_, nEpsilons_;
		Fsa::LabelId label_;
	public:
		CountOutputDfsState(ConstAutomatonRef f, Fsa::LabelId label) :
			Precursor(f), nArcs_(0), nMatched_(0), nEpsilons_(0), label_(label) {
		}
		virtual void discoverState(ConstStateRef sp) {
			for (typename State::const_iterator a = sp->begin(); a != sp->end(); ++a) {
				++nArcs_;
				if (a->output() == label_)
					++nMatched_;
				if (a->output() == Fsa::Epsilon)
					++nEpsilons_;
			}
		}
		size_t matched() const {
			return nMatched_;
		}
	};

	template<class _Automaton> size_t countOutput(
			typename _Automaton::ConstRef f, Fsa::LabelId label, bool progress) {
		Core::ProgressIndicator *p = 0;
		if (progress)
			p = new Core::ProgressIndicator("counting", "states");
		CountOutputDfsState<_Automaton> v(f, label);
		v.dfs(p);
		if (p)
			delete p;
		return v.matched();
	}

	template<class _Automaton> void basicInfo(typename _Automaton::ConstRef f,
			Core::XmlWriter &o) {
		o << Core::XmlFull("type", Fsa::TypeChoice[f->type()]) << Core::XmlFull("describe", f->describe());
		std::string tmp;
		for (Core::Choice::const_iterator i = Fsa::PropertyChoice.begin(); i
				!= Fsa::PropertyChoice.end(); ++i) {
			if (f->knowsProperty(i->value())) {
				if (!tmp.empty())
					tmp += " ";
				if (!f->hasProperty(i->value()))
					tmp += "!";
				tmp += i->ident();
			}
		}
		o << Core::XmlFull("properties", tmp);
		o << Core::XmlFull("semiring", f->semiring()->name());
		if (f->getInputAlphabet()) {
			Fsa::AlphabetCounts c = Fsa::count(f->getInputAlphabet());
			o << Core::XmlFull("input-labels", c.nLabels_);
		}
		if (f->type() == Fsa::TypeTransducer && f->getOutputAlphabet()) {
			Fsa::AlphabetCounts c = Fsa::count(f->getOutputAlphabet());
			o << Core::XmlFull("output-labels", c.nLabels_);
		}
		if (f->initialStateId() != Fsa::InvalidStateId)
			o << Core::XmlFull("initial-state-id", f->initialStateId());
	}

	template<class _Automaton> void info(typename _Automaton::ConstRef f,
			Core::XmlWriter &o, bool progress) {
		o << Core::XmlOpen("fsa-info");
		basicInfo<_Automaton>(f, o);
		Fsa::AutomatonCounts c = count<_Automaton>(f, progress);
		if (c.maxStateId_ != Fsa::InvalidStateId)
			o << Core::XmlFull("max-state-id", c.maxStateId_);
		o << Core::XmlFull("states", c.nStates_);
		o << Core::XmlFull("arcs", c.nArcs_);
		o << Core::XmlFull("final-states", c.nFinals_);
		if (f->type() == Fsa::TypeAcceptor) {
			o << Core::XmlFull("epsilon-arcs", c.nIEps_);
			if (c.nIFail_ != 0)
				o << Core::XmlFull("failure-arcs", c.nIFail_);
		} else if (f->type() == Fsa::TypeTransducer) {
			o << Core::XmlFull("io-epsilon-arcs", c.nIoEps_);
			o << Core::XmlFull("input-epsilon-arcs", c.nIEps_);
			o << Core::XmlFull("output-epsilon-arcs", c.nOEps_);
			if ((c.nIFail_!=0) or (c.nOFail_!=0)) {
				o << Core::XmlFull("io-failure-arcs", c.nIoFail_);
				o << Core::XmlFull("input-failure-arcs", c.nIFail_);
				o << Core::XmlFull("output-failure-arcs", c.nOFail_);
			}
		}
		size_t size = f->getMemoryUsed();
		if (size)
			o << Core::XmlFull("memory", size);
		o << Core::XmlClose("fsa-info");
	}

	template<class _Automaton> void cheapInfo(typename _Automaton::ConstRef f,
			Core::XmlWriter &o) {
		o << Core::XmlOpen("fsa-info");
		basicInfo<_Automaton>(f, o);
		o << Core::XmlClose("fsa-info");
	}

	template<class _Automaton> void memoryInfo(typename _Automaton::ConstRef f,
			Core::XmlWriter &o) {
		o << Core::XmlOpen("fsa-memory-info");
		f->dumpMemoryUsage(o);
		o << Core::XmlFull("total", f->getMemoryUsed());
		o << Core::XmlClose("fsa-memory-info");
	}
} // namespace Ftl
