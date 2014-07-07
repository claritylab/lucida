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
#include <Bliss/Fsa.hh>
#include <Fsa/Basic.hh>
#include <Fsa/Cache.hh>
#include <Fsa/Input.hh>
#include <Fsa/Sort.hh>
#include <Fsa/Determinize.hh>
#include "FsaLm.hh"

using namespace Lm;


const Core::ParameterString FsaLm::paramFilename("file", "name of fsa file file to load as language model");

Fsa::ConstStateRef FsaLm::invalidHistory(new Fsa::State(Fsa::InvalidStateId, Fsa::StateTagFinal));

class FsaLm::HistoryManager : public ReferenceCountingHistoryManager {
protected:
    virtual HistoryHash hashKey(HistoryHandle hd) const {
	const Fsa::State *state = (const Fsa::State*) hd;
	return state->id();
    }

    virtual bool isEquivalent(HistoryHandle hda, HistoryHandle hdb) const {
	const Fsa::State *sa = (const Fsa::State*) hda;
	const Fsa::State *sb = (const Fsa::State*) hdb;
	return sa->id() == sb->id();
    }

    virtual std::string format(HistoryHandle hd) const {
	const Fsa::State *state = (const Fsa::State*) hd;
	return Core::itoa(state->id());
    }
};


/*****************************************************************************/
FsaLm::FsaLm(const Core::Configuration &c, Bliss::LexiconRef lexicon) :
    Core::Component(c), LanguageModel(c, lexicon), syntacticTokens_(lexicon->syntacticTokenAlphabet())
/*****************************************************************************/
{
    historyManager_ = new HistoryManager;
}

/*****************************************************************************/
FsaLm::~FsaLm()
/*****************************************************************************/
{
    delete historyManager_;
}

/*****************************************************************************/
void FsaLm::load()
/*****************************************************************************/
{
    const std::string filename(paramFilename(config));
    log("reading fsa as language model from file")
	<< " \"" << filename << "\" ...";

    Fsa::StorageAutomaton *f = new Fsa::StaticAutomaton();
    if (!Fsa::read(f, filename)) {
	error("Failed to read FSA from file.");
	delete f;
	return;
    }
    setFsa(Fsa::ConstAutomatonRef(f));
}

/*****************************************************************************/
void FsaLm::setFsa(Fsa::ConstAutomatonRef f)
/*****************************************************************************/
{
    fsa_ = Fsa::cache(Fsa::sort(Fsa::determinize(Fsa::mapInput(f, syntacticTokens_)), Fsa::SortTypeByInput));
}

/*****************************************************************************/
History FsaLm::startHistory() const
/*****************************************************************************/
{
    Fsa::StateId initial = fsa_->initialStateId();
    if (initial == Fsa::InvalidStateId)
	error("language model fsa does not have an initial state");
    Fsa::ConstStateRef sp = fsa_->getState(initial);
    sp->acquireReference();
    return history(sp.get());
}

/*****************************************************************************/
History FsaLm::extendedHistory(const History &h, Token w) const
/*****************************************************************************/
{
    Fsa::ConstStateRef sp(descriptor<Self>(h));
    if (sp == invalidHistory) {
	invalidHistory->acquireReference();
	return history(invalidHistory.get());
    }
    while (sp) {
	Fsa::Arc tmp;
	tmp.input_ = w->id();
	Fsa::State::const_iterator a = sp->lower_bound(tmp, Fsa::byInput());
	if ((a == sp->end()) || (a->input() != w->id())) {
	    a = sp->begin();
	    if ((a == sp->end()) || (a->input() != Fsa::Epsilon)) {
		invalidHistory->acquireReference();
		return history(invalidHistory.get());
	    }
	    sp = fsa_->getState(a->target());
	} else {
	    Fsa::ConstStateRef ts = fsa_->getState(a->target());
	    ts->acquireReference();
	    return history(ts.get());
	}
    }
    return h;
}

/*****************************************************************************/
Lm::Score FsaLm::score(const History &h, Token w) const
/*****************************************************************************/
{
    if (w == sentenceEndToken())
	return sentenceEndScore(h);

    Fsa::ConstStateRef sp(descriptor<Self>(h));
    if (sp == invalidHistory) {
	return infinityScore();
    }
    Score score = 0.0;
    while (sp) {
	Fsa::Arc tmp;
	tmp.input_ = w->id();
	Fsa::State::const_iterator a = sp->lower_bound(tmp, Fsa::byInput());
	if ((a == sp->end()) || (a->input() != w->id())) {
	    a = sp->begin();
	    if ((a == sp->end()) || (a->input() != Fsa::Epsilon)) {
		return infinityScore();
	    }
	    sp = fsa_->getState(a->target());
	    score += Score(a->weight());
	} else {
	    return score + Score(a->weight());
	}
    }
    return infinityScore();
}

/*****************************************************************************/
Lm::Score FsaLm::sentenceEndScore(const History &h) const
/*****************************************************************************/
{
    Fsa::ConstStateRef sp(descriptor<Self>(h));
    if (sp == invalidHistory) {
	return infinityScore();
    }
    Score score = 0.0;
    while (sp) {
	if (sp->isFinal())
	    return score + Score(sp->weight_);
	Fsa::State::const_iterator a = sp->begin();
	if ((a == sp->end()) || (a->input() != Fsa::Epsilon)) {
	    return infinityScore();
	}
	sp = fsa_->getState(a->target());
	score += Score(a->weight());
    }
    return infinityScore();
}

