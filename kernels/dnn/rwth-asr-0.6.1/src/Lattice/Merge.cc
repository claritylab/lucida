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
#include "Compose.hh"
#include "Merge.hh"
#include <Fsa/Arithmetic.hh>
#include <Fsa/Cache.hh>
#include <Fsa/Compose.hh>
#include <Fsa/Determinize.hh>
#include <Fsa/Dfs.hh>
#include <Fsa/Mapping.hh>
#include <Fsa/Minimize.hh>
#include <Fsa/Project.hh>
#include <Fsa/Rational.hh>
#include <Fsa/RemoveEpsilons.hh>
#include <Core/Vector.hh>
#include <Core/Hash.hh>
#include <Core/StringUtilities.hh>
#include <Lm/LanguageModel.hh>
#include <Bliss/Orthography.hh>
#include "Utilities.hh"

namespace Lattice {

    namespace Internal {

	struct Context : public std::pair<Lm::History, WordBoundary>
	{
	    typedef std::pair<Lm::History, WordBoundary> Precursor;
	    Context(Lm::History &history, const WordBoundary &wordBoundary) :
		Precursor(history, wordBoundary) {}
	    Context() : Precursor(Lm::History(), WordBoundary()) {}

	    bool valid() const {
		return first.isValid() && second.valid();
	    }
	    bool operator==(const Context &rhs) const {
		return first == rhs.first && second == rhs.second;
	    }
	    bool operator!=(const Context &rhs) const {
		return !(*this == rhs);
	    }
	};

	struct ContextHash
	{
	    size_t operator() (const Context &c) const {
		return ((c.first.hashKey() << 8) | (c.second.hashKey() & 0x000000ff));
	    }
	};

	struct ContextEquality
	{
	    bool operator() (const Context &lhs, const Context &rhs) const {
		return lhs == rhs;
	    }
	};

	struct ArcWithContext : public Fsa::Arc
	{
	    Context rightContext_;
	    bool operator==(const ArcWithContext &rhs) const {
		return input_ == rhs.input_ && output_ == rhs.output_
		    && rightContext_ == rhs.rightContext_;
	    }
	    ArcWithContext(Fsa::StateId target, Fsa::Weight weight,
			   Fsa::LabelId input, Fsa::LabelId output,
			   const Context &rightContext) :
		Fsa::Arc(target, weight, input, output),
		rightContext_(rightContext) {}
	    const Context &rightContext() const { return rightContext_; }
	};

	struct ArcWithContextHash
	{
	    size_t operator()(const ArcWithContext &a) const {
		ContextHash contextHash;
		return ((a.input_ << 24) &
			((a.output_ & 0x000000ff) << 16) &
			(contextHash(a.rightContext_) & 0x0000ffff));
	    }
	};

	struct ArcWithContextEquality
	{
	    bool operator()(const ArcWithContext &a1, const ArcWithContext &a2) const {
		return a1 == a2;
	    }
	};

	typedef Core::hash_map<Context, Fsa::StateId, ContextHash, ContextEquality> IndexMap;
	typedef Core::hash_set<ArcWithContext, ArcWithContextHash, ArcWithContextEquality> ArcsWithContext;
	typedef Core::hash_map<Context, ArcsWithContext, ContextHash, ContextEquality> ArcsWithContextMap;
	typedef Core::hash_set<Context, ContextHash, ContextEquality> Contexts;

	class ArcsWithContextDfsState : public DfsState
	{
	private:
	    Core::Ref<const Lm::LanguageModel> languageModel_;
	    Core::Vector<Lm::History> histories_;
	    ArcsWithContextMap &arcsWithContext_;
	    const Bliss::LemmaPronunciationAlphabet *alphabet_;
	    Contexts &finals_;
	public:
	    ArcsWithContextDfsState(Core::Ref<const Lm::LanguageModel>,
				    ArcsWithContextMap &, Contexts &);
	    virtual ~ArcsWithContextDfsState() {}

	    virtual void discoverState(Fsa::ConstStateRef);
	    void insert(ConstWordLatticeRef);
	};

	ArcsWithContextDfsState::ArcsWithContextDfsState(
	    Core::Ref<const Lm::LanguageModel> languageModel,
	    ArcsWithContextMap &arcsWithContext,
	    Contexts &finals)
	    :
	    DfsState(),
	    languageModel_(languageModel),
	    arcsWithContext_(arcsWithContext),
	    finals_(finals)
	{}

	void ArcsWithContextDfsState::discoverState(Fsa::ConstStateRef sp)
	{
	    if (sp->id() == fsa_->initialStateId()) {
		histories_.grow(sp->id());
		histories_[sp->id()] = languageModel_->startHistory();
	    }
	    require(histories_[sp->id()].isValid());
	    Context context(histories_[sp->id()], (*wordBoundaries_)[sp->id()]);
	    if (sp->isFinal()) {
		finals_.insert(context);
	    }
	    for (Fsa::State::const_iterator a = sp->begin(); a != sp->end(); ++ a) {
		Lm::History hist = histories_[sp->id()];
		const Bliss::LemmaPronunciation *lp = alphabet_->lemmaPronunciation(a->input());
		if (lp) {
		    Lm::extendHistoryByLemmaPronunciation(languageModel_, lp, hist);
		}
		if (fsa_->getState(a->target())->isFinal()) {
		    hist = languageModel_->startHistory();
		}

		histories_.grow(a->target());
		if (!histories_[a->target()].isValid()) {
		    histories_[a->target()] = hist;
		}
		if (!(hist == histories_[a->target()])) {
		    languageModel_->error() <<
			"Mismatch between lattice and language model: "	\
			"ambiguous history at state '" << a->target() << "'.\n" \
			"Possible causes: 1) lattice is time-conditioned,\n" \
			"2) lattice has been generated by using another language model.";
		}
		arcsWithContext_[context].insert(
		    ArcWithContext(Fsa::InvalidStateId,
				   a->weight(), a->input(), a->output(),
				   Context(histories_[a->target()], (*wordBoundaries_)[a->target()])));
	    }
	}

	void ArcsWithContextDfsState::insert(ConstWordLatticeRef wordLattice)
	{
	    fsa_ = wordLattice->mainPart();
	    wordBoundaries_ = wordLattice->wordBoundaries();
	    histories_.clear();
	    alphabet_ = required_cast(
		const Bliss::LemmaPronunciationAlphabet*,
		fsa_->getInputAlphabet().get());
	    dfs();
	}

	Fsa::ConstAutomatonRef correct(
	    const std::string &orth,
	    Bliss::OrthographicParser *orthToLemma,
	    Fsa::ConstAutomatonRef lemmaPronToLemma,
	    Fsa::ConstAutomatonRef lemmaToLemmaConfusion)
	{
	    Fsa::ConstAutomatonRef correct =
		Fsa::cache(
		    Fsa::multiply(
			Fsa::removeEpsilons(
			    Fsa::projectInput(
				Fsa::composeMatching(
				    lemmaPronToLemma,
				    Fsa::projectOutput(
					Fsa::composeMatching(
					    orthToLemma->createLemmaAcceptor(orth),
					    lemmaToLemmaConfusion))))),
			Fsa::Weight(0)));

	    /**
	     * For Fsa::minimize() see Lattice/Compose.hh.
	     * Do not use Fsa::minimize() because it is
	     * significantly slower than Fsa::minimizeSimple().
	     */
	    return Fsa::minimizeSimple(correct);
	}

    } //namespace Internal

    /**
     * merge
     */
    class MergeWordLattice : public Fsa::SlaveAutomaton
    {
    private:
	Internal::Contexts finals_;
	mutable Internal::IndexMap index_;
	mutable Core::Vector<Internal::Context> context_;
	mutable Internal::ArcsWithContextMap arcsWithContext_;
	mutable Core::Ref<WordBoundaries> wordBoundaries_;
    private:
	Fsa::StateId index(const Internal::Context &context) const {
	    if (index_.find(context) == index_.end()) {
		const Fsa::StateId newId = index_.size();
		context_.grow(newId);
		context_[newId] = context;
		index_[context] = newId;
	    }
	    return index_[context];
	}
	bool isFinal(Fsa::StateId s) const {
	    return finals_.find(context_[s]) != finals_.end();
	}
    public:
	MergeWordLattice(ConstWordLatticeRef, ConstWordLatticeRef,
			 Core::Ref<const Lm::LanguageModel>);
	virtual ~MergeWordLattice() {}

	virtual Fsa::StateId initialStateId() const { return 0; }
	virtual Fsa::ConstStateRef getState(Fsa::StateId s) const;
	virtual std::string describe() const {
	    return Core::form("merge(%s)", fsa_->describe().c_str());
	}
	Core::Ref<const WordBoundaries> wordBoundaries() const {
	    return wordBoundaries_;
	}
    };

    MergeWordLattice::MergeWordLattice(ConstWordLatticeRef master,
				       ConstWordLatticeRef slave,
				       Core::Ref<const Lm::LanguageModel> languageModel) :
	Fsa::SlaveAutomaton(master->mainPart()),
	wordBoundaries_(new WordBoundaries())
    {
	require(master->mainPart()->initialStateId() != Fsa::InvalidStateId);
	const WordBoundary &wordBoundaryMaster =
	    master->wordBoundary(master->mainPart()->initialStateId());

	require(languageModel);
	Lm::History history = languageModel->startHistory();
	index(Internal::Context(history, wordBoundaryMaster));

	Internal::ArcsWithContextDfsState s(languageModel, arcsWithContext_, finals_);
	s.insert(master);

	if (slave) {
	    require(slave->mainPart()->getInputAlphabet() ==
		    master->mainPart()->getInputAlphabet());
	    require(slave->mainPart()->getOutputAlphabet() ==
		    master->mainPart()->getOutputAlphabet());
	    require(slave->mainPart()->initialStateId() != Fsa::InvalidStateId);
	    s.insert(slave);
	}
    }

    Fsa::ConstStateRef MergeWordLattice::getState(Fsa::StateId s) const
    {
	require_(context_[s].valid());
	Fsa::State *sp = new Fsa::State(s);
	wordBoundaries_->set(s, context_[s].second);
	if (isFinal(s)) sp->setFinal(semiring()->one());
	Internal::ArcsWithContext arcs = arcsWithContext_[context_[s]];
	for (Internal::ArcsWithContext::iterator a = arcs.begin(); a != arcs.end(); ++ a)
	    sp->newArc(index(a->rightContext()), a->weight(), a->input(), a->output());
	return Fsa::ConstStateRef(sp);
    }

    ConstWordLatticeRef merge(
	ConstWordLatticeRef master, ConstWordLatticeRef slave,
	Core::Ref<const Lm::LanguageModel> languageModel)
    {
	Core::Ref<MergeWordLattice> f(
	    new MergeWordLattice(master, slave, languageModel));
	WordLattice *result = new WordLattice;
	result->setWordBoundaries(f->wordBoundaries());
	result->setFsa(f, master->name(0));
	return ConstWordLatticeRef(result);
    }

    /**
     * extractNumerator
     */
    ConstWordLatticeRef extractNumerator(
	const std::string &orth,
	ConstWordLatticeRef denominator,
	Bliss::OrthographicParser *orthToLemma,
	Fsa::ConstAutomatonRef lemmaPronToLemma,
	Fsa::ConstAutomatonRef lemmaToLemmaConfusion)
    {
	Fsa::ConstAutomatonRef correct =
	    Internal::correct(
		orth,
		orthToLemma,
		lemmaPronToLemma,
		lemmaToLemmaConfusion);
	if (correct) {
	    return composeMatching(correct, denominator);
	}
	return ConstWordLatticeRef();
    }

    /**
     * turnOffCompetingHypotheses
     */
    class TurnOffCompetingHypothesesLattice : public ModifyWordLattice
    {
    private:
	struct Hypothesis {
	    Fsa::StateId from;
	    Fsa::StateId to;
	    Fsa::LabelId label;
	};
	struct HypothesisHash
	{
	    size_t operator() (const Hypothesis &h) const {
		return (h.from & 0x00ff) | ((h.to & 0x00ff) << 8) | ((h.label & 0xffff) << 16);
	    }
	};
	struct HypothesisEquality
	{
	    bool operator() (const Hypothesis &lhs, const Hypothesis &rhs) const {
		return lhs.from == rhs.from && lhs.to == rhs.to && lhs.label == rhs.label;
	    }
	};
	typedef Core::hash_set<Hypothesis, HypothesisHash, HypothesisEquality> CorrectHypotheses;

	class InitializeCorrectHypothesesDfsState : public Fsa::DfsState
	{
	private:
	    Fsa::ConstMappingRef mapping_;
	    CorrectHypotheses &hypotheses_;
	public:
	    InitializeCorrectHypothesesDfsState(
		Fsa::ConstAutomatonRef fsa, Fsa::ConstMappingRef mapping,
		CorrectHypotheses &hypotheses) :
		Fsa::DfsState(fsa), mapping_(mapping), hypotheses_(hypotheses) {}

	    virtual ~InitializeCorrectHypothesesDfsState() {}

	    virtual void discoverState(Fsa::ConstStateRef sp) {
		Hypothesis h;
		h.from = mapping_->map(sp->id());
		for (Fsa::State::const_iterator a = sp->begin(); a != sp->end(); ++ a) {
		    h.to = mapping_->map(a->target());
		    h.label = a->input();
		    hypotheses_.insert(h);
		}
	    }
	};
    private:
	CorrectHypotheses correctHypotheses_;
    public:
	TurnOffCompetingHypothesesLattice(
	    const std::string &orth, ConstWordLatticeRef denominator,
	    Bliss::OrthographicParser *orthToLemma,
	    Fsa::ConstAutomatonRef lemmaPronToLemma,
	    Fsa::ConstAutomatonRef lemmaToLemmaConfusion);
	virtual ~TurnOffCompetingHypothesesLattice() {}

	virtual void modifyState(Fsa::State *sp) const;
	virtual std::string describe() const { return "turn-off-competing(" + fsa_->describe() + ")"; }
    };

    TurnOffCompetingHypothesesLattice::TurnOffCompetingHypothesesLattice(
	const std::string &orth,
	ConstWordLatticeRef denominator,
	Bliss::OrthographicParser *orthToLemma,
	Fsa::ConstAutomatonRef lemmaPronToLemma,
	Fsa::ConstAutomatonRef lemmaToLemmaConfusion)
	:
	ModifyWordLattice(denominator)
    {
	Fsa::ConstAutomatonRef fsa =
	    Fsa::composeMatching(
		Internal::correct(orth, orthToLemma, lemmaPronToLemma, lemmaToLemmaConfusion),
		denominator->mainPart());
	Fsa::ConstMappingRef mapping = Fsa::mapToRight(fsa);
	InitializeCorrectHypothesesDfsState d(fsa, mapping, correctHypotheses_);
	d.dfs();
    }

    void TurnOffCompetingHypothesesLattice::modifyState(Fsa::State *sp) const
    {
	Hypothesis h;
	h.from = sp->id();
	for (Fsa::State::iterator a = sp->begin(); a != sp->end(); ++ a) {
	    h.to = a->target();
	    h.label = a->input();
	    if (correctHypotheses_.find(h) == correctHypotheses_.end()) {
		a->weight_ = semiring()->zero();
	    }
	}
    }

    ConstWordLatticeRef turnOffCompetingHypotheses(
	const std::string &orth,
	ConstWordLatticeRef denominator,
	Bliss::OrthographicParser *orthToLemma,
	Fsa::ConstAutomatonRef lemmaPronToLemma,
	Fsa::ConstAutomatonRef lemmaToLemmaConfusion)
    {
	Core::Ref<TurnOffCompetingHypothesesLattice> turnedOff(
	    new TurnOffCompetingHypothesesLattice(
		orth, denominator, orthToLemma,
		lemmaPronToLemma, lemmaToLemmaConfusion));
	Core::Ref<WordLattice> result(new WordLattice);
	result->setFsa(turnedOff, denominator->mainName());
	result->setWordBoundaries(turnedOff->wordBoundaries());
	return result;
    }

} // namespace Lattice
