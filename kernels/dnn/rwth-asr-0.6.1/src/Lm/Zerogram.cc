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
#include "Zerogram.hh"
#include <Bliss/Fsa.hh>
#include <Fsa/Sort.hh>

using namespace Lm;


Zerogram::Zerogram(const Core::Configuration &c, Bliss::LexiconRef l) :
    Core::Component(c), LanguageModel(c, l)
{
    historyManager_ = this;
    log("Zerogram LM probability is 1/%d", lexicon()->nSyntacticTokens());
    score_ = ::log(f64(lexicon()->nSyntacticTokens()));
}

std::string Zerogram::format(HistoryHandle) const {
    return "zerogram history";
}

class ZerogramAutomaton : public LanguageModelAutomaton {
private:
    Fsa::ConstStateRef state_;
public:
    ZerogramAutomaton(Bliss::LexiconRef lexicon, Score score) :
	LanguageModelAutomaton(lexicon)
    {
	Fsa::State *state = new Fsa::State();
	state->setFinal(Fsa::Weight(score));
	Core::Ref<const Bliss::SyntacticTokenAlphabet> si = lexicon->syntacticTokenAlphabet();
	for (Bliss::Lexicon::SyntacticTokenIterator i = lexicon_->syntacticTokens().first;
	     i < lexicon_->syntacticTokens().second; ++i)
	    state->newArc(0, Fsa::Weight(score), si->index(*i), si->index(*i));
	state->sort(Fsa::byInput());
	state_ = Fsa::ConstStateRef(state);
	addProperties(Fsa::PropertySortedByInput);
	setProperties(Fsa::PropertyLinear, Fsa::PropertyNone);
	setProperties(Fsa::PropertyAcyclic, Fsa::PropertyNone);
    }
    virtual ~ZerogramAutomaton() {}
    virtual Fsa::StateId initialStateId() const { return 0; }
    virtual Fsa::ConstStateRef getState(Fsa::StateId s) const { return state_; }
    virtual void releaseState(Fsa::StateId s) const {}
};

Fsa::ConstAutomatonRef Zerogram::getFsa() const {
    return Fsa::ConstAutomatonRef(new ZerogramAutomaton(lexicon(), score_));
}
