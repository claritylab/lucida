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
#include <Core/CompressedStream.hh>
#include <Core/ProgressIndicator.hh>
#include <Core/Unicode.hh>
#include <Core/Utility.hh>

#include "tBest.hh"
#include "tDfs.hh"
#include "tDraw.hh"
#include "tRational.hh"
#include "Alphabet.hh"
#include "Stack.hh"
#include "Types.hh"

namespace Ftl {

    template<class _Automaton>
    class DrawDotDfsState : public DfsState<_Automaton> {
    private:
	typedef DfsState<_Automaton> Precursor;
    public:
	typedef typename _Automaton::Weight _Weight;
	typedef typename _Automaton::State _State;
	typedef typename _Automaton::ConstStateRef _ConstStateRef;
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
    private:
	DotDrawer<_Automaton> &dd_;
	std::vector<bool> isOnBestPath_;
	StatePotentials<_Weight> potentials_;
    public:
	DrawDotDfsState(DotDrawer<_Automaton> &dd, _ConstAutomatonRef f) :
	    Precursor(f), dd_(dd)
	{
	    if (dd_.hints_ & Fsa::HintMarkBest) {
		potentials_ = sssp<_Automaton>(transpose<_Automaton>(Precursor::fsa_, false));
		_ConstAutomatonRef tmp = best<_Automaton>(Precursor::fsa_, potentials_);
		isOnBestPath_.resize(potentials_.size(), false);
		std::fill(isOnBestPath_.begin(), isOnBestPath_.end(), false);
		Fsa::Stack<Fsa::StateId> S;
		if (tmp->initialStateId() != Fsa::InvalidStateId)
		    S.push(tmp->initialStateId());
		while (!S.isEmpty()) {
		    Fsa::StateId s = S.pop();
		    _ConstStateRef sp = tmp->getState(s);
		    isOnBestPath_[s] = true;
		    if (sp->nArcs() == 1)
			S.push(sp->begin()->target());
		}
	    }
	}

	std::string replaceQuotationMarks(std::string &s) {
	    for (std::string::iterator p = s.begin(); p != s.end(); ++p) if (*p =='"') *p = ' ';
	    return s;
	}

	void discoverState(_ConstStateRef sp) {
	    dd_.os_ << "n" << sp->id() << " [label=\"";
	    if (dd_.hints_ & Fsa::HintShowDetails) Precursor::fsa_->dumpState(sp->id(), dd_.os_);
	    else dd_.os_ << sp->id();
	    if (sp->isFinal())
		if (!Precursor::fsa_->semiring()->isDefault(sp->weight_))
		    dd_.os_ << "/" << Precursor::fsa_->semiring()->describe(sp->weight_, dd_.hints_);
	    dd_.os_ << "\"";
	    if (sp->isFinal()) dd_.os_ << ",shape=doublecircle";
	    else dd_.os_ << ",shape=circle";
	    if (sp->id() == Precursor::fsa_->initialStateId()) dd_.os_ << ",style=bold";
	    if ((dd_.hints_ & Fsa::HintMarkBest) && isOnBestPath_[sp->id()]) dd_.os_ << ",color=red";
	    dd_.os_ << "]" << std::endl;
	    typename _State::const_iterator bestArc = sp->end();
	    if ((dd_.hints_ & Fsa::HintMarkBest) && isOnBestPath_[sp->id()]) {
		_Weight minWeight = Precursor::fsa_->semiring()->max();
		for (typename _State::const_iterator a = sp->begin(); a != sp->end(); ++a)
		    if (isOnBestPath_[a->target()]) {
			_Weight w = Precursor::fsa_->semiring()->extend(a->weight_, potentials_[a->target()]);
			if (Precursor::fsa_->semiring()->compare(w, minWeight) < 0) {
			    minWeight = w;
			    bestArc = a;
			}
		    }
	    }
	    for (typename _State::const_iterator a = sp->begin(); a != sp->end(); ++a) {
		std::string symbol;
		dd_.os_ << "n" << sp->id() << " -> n" << a->target() << " [";
		if (a == bestArc) dd_.os_ << "color=red,";
		dd_.os_ << "label=\"";
		if (Precursor::fsa_->getInputAlphabet()) symbol = Precursor::fsa_->getInputAlphabet()->symbol(a->input());
		if ((!Precursor::fsa_->getInputAlphabet()) || (symbol.empty())) dd_.os_ << a->input();
		else dd_.os_ << replaceQuotationMarks(symbol);
		if (Precursor::fsa_->type() == Fsa::TypeTransducer) {
		    dd_.os_ << ":";
		    if (Precursor::fsa_->getOutputAlphabet()) symbol = Precursor::fsa_->getOutputAlphabet()->symbol(a->output());
		    if ((!Precursor::fsa_->getOutputAlphabet()) || (symbol.empty())) dd_.os_ << a->output();
		    else dd_.os_ << replaceQuotationMarks(symbol);
		}
		if (!Precursor::fsa_->semiring()->isDefault(a->weight()))
		    dd_.os_ << "/" << Precursor::fsa_->semiring()->describe(a->weight(), dd_.hints_);
		dd_.os_ << "\"]" << std::endl;
	    }
	}
    };

    template<class _Automaton>
    DotDrawer<_Automaton>::DotDrawer(std::ostream &os, Fsa::Hint hints, bool progress) :
	os_(os), hints_(hints), progress_(progress) {}

    template<class _Automaton>
    bool DotDrawer<_Automaton>::draw(typename _Automaton::ConstRef f) {
	if (os_) {
	    os_ << "digraph \"fsa\" {\nranksep = 1.0;\nrankdir = LR;\ncenter = 1;" << std::endl
		<< "orientation = Landscape\nnode [fontname=\"Helvetica\"]" << std::endl
		<< "edge [fontname=\"Helvetica\"]" << std::endl;
	    Core::ProgressIndicator *p = 0;
	    if (progress_) p = new Core::ProgressIndicator("writing dot graph", "states");
	    DrawDotDfsState<_Automaton> s(*this, f);
	    s.dfs(p);
	    delete p;
	    os_ << "}" << std::endl;
	}
	return os_;
    }

    template<class _Automaton>
    bool drawDot(typename _Automaton::ConstRef f, std::ostream &o, Fsa::Hint hint, bool progress) {
	DotDrawer<_Automaton> dd(o, hint, progress);
	return dd.draw(f);
    }

    template<class _Automaton>
    bool drawDot(typename _Automaton::ConstRef f, const std::string &file, Fsa::Hint hint, bool progress) {
	Core::CompressedOutputStream o(file);
	return drawDot<_Automaton>(f, o, hint, progress);
    }

} // namespace Ftl
