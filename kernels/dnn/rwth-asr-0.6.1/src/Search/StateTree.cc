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
// $Id: StateTree.cc 9621 2014-05-13 17:35:55Z golik $

#include "StateTree.hh"
#include "StateTreeIo.hh"
#include <Core/Hash.hh>
#include <Core/ProgressIndicator.hh>
#include <Core/Utility.hh>
#include <Legacy/DecisionTree.hh>
#include <Am/Utilities.hh>
#include <deque>

using namespace Search;
using Core::tie;


/*
 * There are three types of transitions in the state tree:
 * a) emitting (acoustic) transitions
 * b) exit, or word end, transitions are taken when the word identity
 * is known and lead into the tree copy for the corresponding new LM
 * history.
 * c) epsilon, pseudo exit, in-tree transit or "physical word end"
 * transitions (not sure what the best name is) have been introduced
 * to enforce the creation of book-keeping entries (traces aka
 * backpointer) to record the physical word boundary time and score
 * also when using early recombination.
 *
 * Emissions are coupled to the states following the principle:
 * propagate, then emit.  Thus a state's mixture can be interpreted as
 * the emission for all transitions *entering* the state.  Exit
 * transitions are attached to their originating state, and they are
 * taken *after* the emission.
 *
 * Depth is counted in acoustic (forward) transitions from beginning
 * of word: I.e. the root state has depth 0, the first state of a word
 * has depth 1, and pronunciations of length n end at depth n.  */


// ===========================================================================

/**
 * Suffix of a pronunciation defined by a phoneme, a phoneme state
 * and phoneme state substate (?)
 */
class StateTree::PronunciationSuffix {
private:
	const Bliss::Pronunciation *pronunciation_;
	Bliss::Phoneme::Id predecessorPhoneme_;
	Bliss::Phoneme::Id successorPhoneme_;
	friend class StateTree;

	s8 phoneme, phoneState, subState;

	Bliss::Phoneme::Id currentPhoneme() const {
		return (*pronunciation_)[phoneme];
	}

	bool hasLeaped_;
	bool isEmpty_;

	mutable bool isHashValid_;
	mutable u32 hash_;

public:
	bool hasLeaped() const {
		return hasLeaped_;
	}
	Bliss::Phoneme::Id finalPhoneme() const {
		return (*pronunciation_)[pronunciation_->length()-1];
	}
	/**
	 * the successor phoneme is set for across word modelling
	 * to create pronunciation alternatives for all possible
	 * subsequent phonemes (in the next word)
	 */
	Bliss::Phoneme::Id successorPhoneme() const {
		return successorPhoneme_;
	}

	struct Ordering; friend struct PronunciationSuffix::Ordering;
	struct Equality; friend struct PronunciationSuffix::Equality;
	struct Hash;	 friend struct PronunciationSuffix::Hash;

	/**
	 * Pronuncation suffix for pronuncation @c p with history (= predecessor phoneme) @h.
	 * the predecessor phoneme is used for across word context
	 */
	PronunciationSuffix(const Bliss::Pronunciation *p, Bliss::Phoneme::Id h = Bliss::Phoneme::term) :
		pronunciation_(p),
		predecessorPhoneme_(h),
		successorPhoneme_(Bliss::Phoneme::term),
		phoneme(-1), phoneState(0), subState(0),
		hasLeaped_(false),
		isEmpty_(false),
		isHashValid_(false) {}

	const Bliss::Pronunciation *pronunciation() const {
		return (hasLeaped()) ? 0 : pronunciation_;
	}

	bool isEmpty() const {
		return isEmpty_;
	}

	s32 nPhonemesRemaining() const {
		return pronunciation_->length() - phoneme;
	}
};

// ===========================================================================

struct StateTree::PronunciationSuffix::Ordering {
	const StateTree *model;
	Ordering(const StateTree *m) : model(m) {}
	bool operator() (const PronunciationSuffix &l, const PronunciationSuffix &r) const {
		return model->compareSequences(l, r) < 0;
	}
};

struct StateTree::PronunciationSuffix::Equality {
	const StateTree *model;
	Equality(const StateTree *m) : model(m) {}
	bool operator() (const PronunciationSuffix &l, const PronunciationSuffix &r) const {
		return model->compareSequences(l, r) == 0;
	}
};

struct StateTree::PronunciationSuffix::Hash {
	const StateTree *model;
	Hash(const StateTree *m) : model(m) {}
	u32 operator() (const PronunciationSuffix &s) const {
		if (!s.isHashValid_) {
			s.hash_ =  model->hashSequence(s);
			s.isHashValid_ = true;
		}
		return s.hash_;
	}
};

inline void  StateTree::leap(PronunciationSuffix &s) const {
	require(!s.hasLeaped_);
	s.hasLeaped_ = true;
	s.isHashValid_ = false;
	ensure(!s.pronunciation());
}

inline s8 StateTree::nPhoneStates(const PronunciationSuffix &s) const {
	if (s.phoneme < 0)
		return 0;
	const Am::ClassicHmmTopology *hmmTopology = acousticModel_->hmmTopology(s.currentPhoneme());
	ensure_(hmmTopology != 0);
	return hmmTopology->nPhoneStates();
}

inline s8 StateTree::nSubStates(const PronunciationSuffix &s) const {
	if (s.phoneme < 0)
		return 0;
	const Am::ClassicHmmTopology *hmmTopology = acousticModel_->hmmTopology(s.currentPhoneme());
	ensure_(hmmTopology != 0);
	return hmmTopology->nSubStates();
}

/**
 * Go to the next sub state of the current phoneme state in the current phoneme
 * of @c s.
 * \return last substate was passed
 */
inline bool StateTree::advanceSubState(PronunciationSuffix &s) const {
	++s.subState;
	++s.hash_;
	if (s.subState >= nSubStates(s)) {
		s.hash_ -= u32(s.subState);
		s.subState = 0;
		return true;
	}
	return false;
}

/**
 * Go to the next phoneme state of the current phoneme in @c s.
 * \return last phone state was passed
 */
inline bool StateTree::advancePhoneState(PronunciationSuffix &s) const {
	++s.phoneState;
	s.isHashValid_ = false;
	if (s.phoneState >= nPhoneStates(s)) {
		s.phoneState = 0;
		return true;
	}
	return false;
}

/**
 * Find the allophone for the current phoneme in @c s.
 */
inline const Am::Allophone * StateTree::getAllophone(
	const PronunciationSuffix &s) const
{
	const Am::Phonology & phonology = *(acousticModel_->phonology());

	// create an allophone with the correct boundaries
	Am::Allophone allophone(phonology(*s.pronunciation_, s.phoneme), boundary(s));

	if (s.predecessorPhoneme_ != Bliss::Phoneme::term) {
		verify(s.phoneme == 0);
		phonology.appendHistory(allophone, s.predecessorPhoneme_);
	}
	if (s.successorPhoneme_ != Bliss::Phoneme::term) {
		verify(s.phoneme == s.pronunciation_->length() - 1);
		phonology.appendFuture(allophone, s.successorPhoneme_);
	}

	ensure_(phonemeInventory()->phoneme(allophone.phoneme(0))->isContextDependent() ||
			(allophone.history().size() == 0 && allophone.future().size() == 0));

	// now the get pointer to the unique allophone from the allophone alphabet
	const Am::Allophone *result = acousticModel_->allophoneAlphabet()->allophone(allophone);
	verify(result);
	return result;
}

/**
 * Phoneme boundary (initial, final, or both) for the current phoneme in @c s.
 */
inline s16 StateTree::boundary(const PronunciationSuffix &s) const {
	s16 result = 0;
	if (s.phoneme == 0)
		result |= Am::Allophone::isInitialPhone;
	if (s.phoneme == s.pronunciation_->length() - 1)
		result |= Am::Allophone::isFinalPhone;
	return result;
}

/**
 * Transition model for the current (sub-) state of @s c
 */
inline u16 StateTree::transitionModelIndex(const PronunciationSuffix &s) const {
	if (s.phoneme < 0) {
		return Am::TransitionModel::entryM1;
	} else {
		const Am::Allophone *allo = getAllophone(s);
		Am::AllophoneState alloState = acousticModel_->allophoneStateAlphabet()->allophoneState(allo, s.phoneState);
		require_(acousticModel_->stateTransitionIndex(alloState, s.subState) < Core::Type<StateDesc::TransitionModelIndex>::max);
		return acousticModel_->stateTransitionIndex(alloState, s.subState);
	}
}

/**
 * Create a state description for the current phoneme in @c s.
 * Sets the acousticModel and the transitionModel of the state description.
 */
StateTree::StateDesc StateTree::head(const PronunciationSuffix &s) const {
	require(!s.isEmpty());
	StateDesc result;
	if (s.phoneme < 0) {
		result.acousticModel = invalidAcousticModel;
	} else {
		const Am::Allophone *allo = getAllophone(s);
		Am::AllophoneState alloState = acousticModel_->allophoneStateAlphabet()->allophoneState(allo, s.phoneState);
		result.acousticModel = acousticModel_->emissionIndex(alloState);
	}
	result.transitionModelIndex = transitionModelIndex(s);
	return result;
}

/**
 * Go to the next (sub-) state in the pronciation.
 * Successor pronuncation suffixes appended to @c out
 * If the pronunciation ends, it is appended to @c ends
 */
void StateTree::advancePronunciationSuffix(
	const PronunciationSuffix &_s,
	std::back_insert_iterator< std::vector<PronunciationSuffix> > out,
	std::back_insert_iterator< std::vector<PronunciationSuffix> > ends) const
{
	PronunciationSuffix s(_s);
	require(!s.isEmpty());

	if (advanceSubState(s)) {
		// next phoneme state reached
		bool mustAdvancePhoneme = advancePhoneState(s);
		if (mustAdvancePhoneme) {
			// next phoneme reached
			++s.phoneme;
			s.isHashValid_ = false;
			if (s.phoneme >= s.pronunciation_->length()) {
				// end of pronunciation reached.
				s.isEmpty_ = true;
				*ends++ = s;
				return;
			}

			if (s.phoneme > 0) // ?
				s.predecessorPhoneme_ = Bliss::Phoneme::term;
		}

		if (acousticModel_->isAcrossWordModelEnabled() &&
			mustAdvancePhoneme && s.phoneme == s.pronunciation_->length() - 1) {
			// last phoneme in the pronunciation
			// create fan-out suffixes, i.e. create pronuncation alternatives
			// using all initial phonemes as successor phoneme
			if (phonemeInventory()->phoneme(s.currentPhoneme())->isContextDependent()) {
				for (u32 r = 0; r < initialPhonemes_->size(); ++r) {
					s.successorPhoneme_ = (*initialPhonemes_)[r];
					*out++ = s;
				}
			}
			s.successorPhoneme_ = Bliss::Phoneme::term;
		}
	}
	*out++ = s;
}

u32 StateTree::nStatesRemaining(const PronunciationSuffix &_s) const {
	u32 result = 0;
	PronunciationSuffix s(_s);
	while (s.phoneme < s.pronunciation_->length()) {
		if (advanceSubState(s))
			if (advancePhoneState(s))
				++s.phoneme;
		++result;
	}
	return result;
}

int StateTree::compareSequences(const PronunciationSuffix &l, const PronunciationSuffix &r) const {
	if (		l.pronunciation()	  != r.pronunciation())
		return (l.pronunciation()	   < r.pronunciation())   ? -1 : 1;
	if (		l.successorPhoneme_	!= r.successorPhoneme_)
		return (l.successorPhoneme_	 < r.successorPhoneme_) ? -1 : 1;
	if (		l.phoneState		   != r.phoneState)
		return  l.phoneState			- r.phoneState;
	if (		l.subState			 != r.subState)
		return  l.subState			  - r.subState;
	if (		l.nPhonemesRemaining() != r.nPhonemesRemaining())
		return  l.nPhonemesRemaining()  - r.nPhonemesRemaining();
	for (s32 p = 0; p < l.nPhonemesRemaining(); ++p) {
		if		((*l.pronunciation_)[l.phoneme + p] != (*r.pronunciation_)[r.phoneme + p])
			return (*l.pronunciation_)[l.phoneme + p]  - (*r.pronunciation_)[r.phoneme + p];
	}

	const Am::Allophone *la = getAllophone(l);
	const Am::Allophone *ra = getAllophone(r);
	verify(nPhoneStates(l) == nPhoneStates(r));
	if (la != ra) {
		for (s8 phoneState = l.phoneState; phoneState < nPhoneStates(l); ++phoneState) {
			Am::AllophoneState las = acousticModel_->allophoneStateAlphabet()->allophoneState(la, phoneState);
			Am::AllophoneState ras = acousticModel_->allophoneStateAlphabet()->allophoneState(ra, phoneState);
			Mm::MixtureIndex lm = acousticModel_->emissionIndex(las);
			Mm::MixtureIndex rm = acousticModel_->emissionIndex(ras);
			if (lm != rm)
				return lm - rm;
		}
	}

	verify_(hashSequence(l) == hashSequence(r));
	return 0;
}

u32 StateTree::hashSequence(const PronunciationSuffix &s) const {
	u32 result = u32(long(s.pronunciation() /* this is a pointer */));

	for (s32 p = 0; p < s.nPhonemesRemaining(); ++p) {
		result = (result << 7) | (result >> 25);
		result ^= u32((*s.pronunciation_)[s.phoneme + p]);
	}
	result ^= u32(s.successorPhoneme_) << 16;

	const Am::Allophone *a = getAllophone(s);
	for (s16 state = s.phoneState; state < nPhoneStates(s); ++state) {
		Am::AllophoneState as = acousticModel_->allophoneStateAlphabet()->allophoneState(a, state);
		result = (result << 11) | (result >> 21);
		result ^= u32(acousticModel_->emissionIndex(as));
	}

	result += u32(s.subState);

	return result;
}

// ===========================================================================

/**
 * A set of pronuncation suffixes (@see PronunciationSuffix)
 */
class StateTree::SuffixSet {
public:
	struct End;
	typedef std::vector<PronunciationSuffix> ItemList;
private:
	const StateTree *model_;

	ItemList items_;

	bool hasUniqueHead_;
	mutable enum { dirty, sorted, hashValid } consolidation_;

private:
	void sort() {
		std::sort(items_.begin(), items_.end(), PronunciationSuffix::Ordering(model_));
		consolidation_ = sorted;
	}

	mutable u32 hash_;
	void updateHash(const PronunciationSuffix &s) const {
		hash_ = (hash_ << 5) | (hash_  >> 27);
		hash_ ^= PronunciationSuffix::Hash(model_)(s);
	}
	void updateHash() const {
		verify(consolidation_ >= sorted);
		hash_ = 0;
		for (ItemList::const_iterator i = items_.begin(); i != items_.end(); ++i)
			updateHash(*i);
		consolidation_ = hashValid;
	}

public:
	void verifyGood() const {
	    verify(consolidation_ >= hashValid);
	    ItemList sorted = items_;
	    std::sort(sorted.begin(), sorted.end(), PronunciationSuffix::Ordering(model_));
	    verify( std::equal( sorted.begin(), sorted.end(), items_.begin(), PronunciationSuffix::Equality(model_) ) );

	    u32 hash = 0;
	    for (ItemList::const_iterator i = items_.begin(); i != items_.end(); ++i)
	    {
		hash = (hash << 5) | (hash  >> 27);
		hash ^= PronunciationSuffix::Hash(model_)(*i);
	    }
	    verify( hash == hash_ );
	}

	bool isSorted()	const { return consolidation_ >= sorted; }
	bool isHashValid() const { return consolidation_ >= hashValid; }

	u32 size() const {
		return items_.size();
	}

	u32 hash() const {
		verify(isHashValid());
		return hash_;
	}

	bool operator== (const SuffixSet &r) const {
		require(model_ == r.model_);
		verify(isSorted());
		verify(r.isSorted());
		if (hash() != r.hash()) return false;
		if (items_.size() != r.items_.size()) return false;
		return std::equal(items_.begin(), items_.end(), r.items_.begin(),
						  PronunciationSuffix::Equality(model_));
	}

	SuffixSet(const StateTree *m) :
		model_(m),
		hasUniqueHead_(true),
		consolidation_(hashValid),
		hash_(0) {}

	bool hasUniqueHead() const { return hasUniqueHead_; }

	StateDesc head() const {
		verify(hasUniqueHead());
		return model_->head(items_.front());
	}

private:
	void add(const PronunciationSuffix &s) {
		verify(!hasUniqueHead() || items_.empty() || head() == model_->head(s));
		items_.push_back(s);
	}

public:
	void addDirty(const PronunciationSuffix &s) {
		add(s);
		consolidation_ = dirty;
	}

	void sortAndHash() {
		sort();
		updateHash();
	}

	void addSorted(const PronunciationSuffix &s) {
		verify(items_.empty() || !PronunciationSuffix::Ordering(model_)(s, items_.back()));
		add(s);
		updateHash(s);
	}

	typedef std::vector<End> EndList;
	typedef std::vector<SuffixSet> SuffixSetList;
	void partition(SuffixSetList &conts) const;
	void advance(ItemList &ends);
	void leap(std::vector<const Bliss::Pronunciation*> &leaps);
	bool hasLeaped() const;
	u32 nStatesRemaining() const;
};


struct StateTree::SuffixSet::End {
	const Bliss::Pronunciation *pronunciation;
	StateTree::StateId transitEntry;
};

/**
 * Split the suffix set in a set of suffix sets, such
 * that each resulting suffix set contains pronunciation
 * suffixes with equal first state (w.r.t the state description @see StateDesc)
 */
void StateTree::SuffixSet::partition(SuffixSetList &conts) const {
	require(isSorted());
	require(conts.empty());

	typedef std::map<StateDesc, u32> ContsMap;
	ContsMap contsMap;
	for (ItemList::const_iterator i = items_.begin(); i < items_.end(); ++i) {

		verify(!i->isEmpty());
		StateDesc head(model_->head(*i));

		if( model_->isPathRecombinationInFanInEnabled_ )
		{
		///@todo Change the hash structure, instead of abusing TransitionModelIndex
		    if( (i->phoneme == 0 && i->subState == model_->nSubStates(*i)-1 && i->phoneState == model_->nPhoneStates(*i)-1)
			|| ( i->phoneme == 1 && i->subState == 0 && i->phoneState == 0 ) )
		    head.transitionModelIndex = i->currentPhoneme();
		}

		ContsMap::iterator c = contsMap.find(head);
		if (c == contsMap.end()) {
			c = contsMap.insert(c, std::make_pair(head, conts.size()));
			conts.push_back(SuffixSet(model_));
		}
		conts[c->second].addSorted(*i);
	}
}

/**
 * Go to the next state in all pronunciation suffixes.
 * Ending pronunciations are appended to @c endingItems
 */
void StateTree::SuffixSet::advance(ItemList &endingItems) {
	endingItems.clear();
	ItemList newItems;
	for (ItemList::iterator i = items_.begin(); i != items_.end() ; ++i)
		model_->advancePronunciationSuffix(
			*i,
			std::back_inserter(newItems),
			std::back_inserter(endingItems));
	items_.swap(newItems);
	hasUniqueHead_ = false;
	consolidation_ = dirty;
}

// ?
void StateTree::SuffixSet::leap(
	std::vector<const Bliss::Pronunciation*> &leaps)
{
	leaps.clear();
	for (ItemList::iterator i = items_.begin(); i != items_.end() ; ++i) {
		leaps.push_back(i->pronunciation());
		model_->leap(*i);
	}
	consolidation_ = dirty;
}

bool StateTree::SuffixSet::hasLeaped() const {
	require(items_.size() >= 1);
	if (items_.size() > 1)
		return false;
	return items_.front().hasLeaped();
}

u32 StateTree::SuffixSet::nStatesRemaining() const {
	require(items_.size() == 1);
	return model_->nStatesRemaining(items_.front());
}

// ===========================================================================

inline StateTree::StateId StateTree::CoarticulationStructure::root(Bliss::Phoneme::Id final, Bliss::Phoneme::Id initial) const {
	require_(std::find(  finalPhonemes.begin(),   finalPhonemes.end(),   final) !=   finalPhonemes.end());
	require_(std::find(initialPhonemes.begin(), initialPhonemes.end(), initial) != initialPhonemes.end());
	int l = std::find(  finalPhonemes.begin(),   finalPhonemes.end(),   final) -   finalPhonemes.begin();
	int r = std::find(initialPhonemes.begin(), initialPhonemes.end(), initial) - initialPhonemes.begin();
	return roots[l][r];
}


StateTree::CoarticulationStructure::CoarticulationStructure(
	const StateTree *sm, Bliss::LexiconRef l) {
	Am::LexiconUtilities lexiconUtilities(sm->getConfiguration(), l);
	lexiconUtilities.getInitialAndFinalPhonemes(initialPhonemes, finalPhonemes);
	roots.resize(finalPhonemes.size());
	for (u32 l = 0; l < finalPhonemes.size(); ++l)
		roots[l].resize(initialPhonemes.size(), -1);
}

std::pair<Bliss::Phoneme::Id, Bliss::Phoneme::Id> StateTree::describeRootState(StateId s) const {
	Bliss::Phoneme::Id final, initial;
	if (s == root() || !coarticulationStructure_) {
		// root state or no across word modelling
		final = initial = Bliss::Phoneme::term;
	} else {
		if (s < coarticulationStructure_->boundaryPhonemes.size()) {
			final   = coarticulationStructure_->boundaryPhonemes[s].final;
			initial = coarticulationStructure_->boundaryPhonemes[s].initial;
		} else {
			std::string text = Core::form("Unknown transit index %d. Context is set to non-coarticulated. ", s);
			text +=  "It is likely that it occurs in combination with \"No active word end hypothesis at sentence end.\"";
			warning(text.c_str());
			final = initial = Bliss::Phoneme::term;
		}
	}
	return std::make_pair(final, initial);
}

// ===========================================================================

/**
 * Request for building a set of successors in the state tree.
 * Abstract interface and ordering.
 * Main methods are satisfy() to do the building, and
 * merge() to merge to build requests.
 */
struct StateTree::BuildRequest {
	Depth depth;
	enum RequestType {unknown, batchRequest, leapRequest};
	RequestType type;
	BuildRequest(RequestType t) : type(t) {}
	virtual ~BuildRequest() {}
	virtual void merge(const BuildRequest*) = 0;
	virtual void satisfy(StateTree*) = 0;
	virtual u32 hash() const = 0;
	virtual bool equals(const BuildRequest*) const = 0;

	struct DepthOrdering {
		bool operator() (const BuildRequest *l, const BuildRequest *r) const {
			if (l->depth >= 0) {
				if (r->depth < 0) return true;
			} else {
				if (r->depth >= 0) return false;
			}
			return l->depth < r->depth;
		}
	};

	struct Hash {
		u32 operator() (const BuildRequest *r) const {
			return r->hash();
		}
	};

	struct Equality {
		bool operator() (const BuildRequest *l, const BuildRequest *r) const {
			return (l == r) || l->equals(r);
		}
	};
};

/**
 * Build a new batch, i.e. the sets of successors for a set of states
 */
struct StateTree::BatchRequest :
	public BuildRequest
{
	std::vector<StateId> states;
	SuffixSet suffixes;
	SuffixSet parentsSuffixes;

	BatchRequest(StateTree *m, StateId s):
		BuildRequest(batchRequest),
		suffixes(m), parentsSuffixes(m)
		{

			states.push_back(s);
		}

	virtual void merge(const BuildRequest*);
	virtual void satisfy(StateTree *t);

	virtual u32 hash() const {
		return		suffixes.hash()
			+  parentsSuffixes.hash();
	}

	virtual bool equals(const BuildRequest *_r) const {
		if (type != _r->type) return false;
		const BatchRequest *r = dynamic_cast<const BatchRequest*>(_r);
		require(r);
		return (   (	   suffixes == r->	   suffixes)
				   && (parentsSuffixes == r->parentsSuffixes));
	}
};

void StateTree::BatchRequest::merge(const BuildRequest *_r) {
	const BatchRequest *r = dynamic_cast<const BatchRequest*>(_r);
	require(r);
	states.insert(states.end(), r->states.begin(), r->states.end());
}

void StateTree::BatchRequest::satisfy(StateTree *t) {
	BatchId batch = t->createBatch(this);
	for (std::vector<StateId>::const_iterator s = states.begin(); s != states.end(); ++s) {
		t->states_[*s].successors = batch;
	}
}

/**
 * ??
 */
struct StateTree::LeapRequest :
	public BatchRequest
{
	std::vector< std::vector<const Bliss::Pronunciation*> > leaps;

	LeapRequest(StateTree *m, StateId s): BatchRequest(m, s) {
	    verify(0);
		type = leapRequest;
		leaps.push_back(std::vector<const Bliss::Pronunciation*>());
	}

	virtual void merge(const BuildRequest*);
	virtual void satisfy(StateTree*t);

	virtual u32 hash() const {
		return ~BatchRequest::hash();
	}
};

void StateTree::LeapRequest::merge(const BuildRequest *_r) {
	const LeapRequest *r = dynamic_cast<const LeapRequest*>(_r);
	require(r);
	BatchRequest::merge(r);
	leaps.insert(leaps.end(), r->leaps.begin(), r->leaps.end());
	verify(states.size() == leaps.size());
}

void StateTree::LeapRequest::satisfy(StateTree *t) {
	StateId s = t->createLeapEntry(this);
	BatchId b = t->createBatch(this);
	t->states_[s].successors = b;
};

class StateTree::RequestQueue {
protected:
	StateTree *tree_;
	RequestQueue(StateTree *t) : tree_(t) {}
public:
	virtual ~RequestQueue() {}
	virtual void submit(BatchRequest*) = 0;
	virtual u32 size() const = 0;
	virtual BuildRequest *pop() = 0;
};

class StateTree::StraightRequestQueue : public RequestQueue {
private:
	typedef std::deque<BuildRequest*> Queue;
	Queue queue_;
public:
	StraightRequestQueue(StateTree*);
	virtual ~StraightRequestQueue() {}
	virtual void submit(BatchRequest*);
	virtual u32 size() const { return queue_.size(); }
	virtual BuildRequest *pop();
};

StateTree::StraightRequestQueue::StraightRequestQueue(StateTree *t) :
	RequestQueue(t)
{}

void StateTree::StraightRequestQueue::submit(BatchRequest *request) {
	queue_.push_back(request);
}

StateTree::BuildRequest *StateTree::StraightRequestQueue::pop() {
	BuildRequest *r = 0;
	if (!queue_.empty()) {
		r = queue_.front();
		queue_.pop_front();
	} else {
		tree_->wordHeadsEnd_ = tree_->states_.size();
	}
	return r;
}

class StateTree::RecombiningRequestQueue : public RequestQueue {
private:
	typedef std::deque<BuildRequest*> Queue;
	typedef std::map<Depth, Queue> QueueSet;
	typedef Core::hash_set<
		BatchRequest*,
		BuildRequest::Hash,
		BuildRequest::Equality> Map;
	QueueSet queues_;
	QueueSet::iterator queue_;
	Map map_;
	u32 size_;

public:
	RecombiningRequestQueue(StateTree*);
	virtual ~RecombiningRequestQueue() {}
	virtual void submit(BatchRequest*);
	virtual u32 size() const { return size_; }
	virtual BuildRequest *pop();
};

StateTree::RecombiningRequestQueue::RecombiningRequestQueue(StateTree *t) :
	RequestQueue(t),
	size_(0)
{
	queues_[0];
	queue_ = queues_.begin();
}

void StateTree::RecombiningRequestQueue::submit(BatchRequest *request) {
	Map::iterator r = map_.find(request);
	if (r == map_.end()) {
		map_.insert(request);
		queues_[request->depth].push_back(request);
		++size_;
	} else {
//                 if( request->depth > 7 )
//                 {
//                     std::cout << "too deep recombination on depth " << request->depth << std::endl;
//                 }
		(*r)->merge(request);
		delete request;
	}
}

StateTree::BuildRequest *StateTree::RecombiningRequestQueue::pop() {
	BuildRequest *r = 0;
	if (queue_->second.empty()) {
		++queue_;
		if (queue_ == queues_.end()) {
			queue_ =  queues_.begin();
			tree_->wordHeadsEnd_ = tree_->states_.size();
		}
	}
	if (!queue_->second.empty()) {
		r = queue_->second.front();
		queue_->second.pop_front();
		map_.erase(dynamic_cast<BatchRequest*>(r));
		--size_;
	}
	return r;
}

// ===========================================================================
StateTree::State::State() {
	successors = invalidBatch;
}

const StateTree::Depth StateTree::invalidDepth = Core::Type<StateTree::Depth>::max;
const StateTree::BatchId StateTree::invalidBatch = Core::Type<StateTree::BatchId>::max;
const StateTree::StateDesc::AcousticModelIndex StateTree::invalidAcousticModel = Core::Type<StateTree::StateDesc::AcousticModelIndex>::max;

const Core::ParameterBool StateTree::paramEarlyRecombination(
	"early-recombination",
	"enable recobination of word hypothesis before actual word end (EXPERIMENTAL)",
	false);
const Core::ParameterBool StateTree::paramEnforcePathRecombinationInFanIn(
	"enforce-path-recombination-in-fanin",
	"",
	false);
const Core::ParameterInt StateTree::paramEarlyRecombinationLimit(
	"early-recombination-limit",
	"minimum number of states before early recobination is allows (negative: maximum number of states remaining)",
	0);
const Core::ParameterString StateTree::paramFile(
	"file",
	"cached state tree", "");
const Core::ParameterBool StateTree::paramSkipTransitions(
	"skip-transitions", "build second order structure for skips transitions", true);
const Core::ParameterBool StateTree::paramCiCrossWordTransitions(
	"ci-cross-word-transitions", "allow context independent cross word transitions a{b+#}@f c{#+d}@i", true);



StateTree::StateTree(
	const Core::Configuration &c,
	Bliss::LexiconRef lexicon,
	Core::Ref<const Am::AcousticModel> acousticModel) :
	Core::Component(c),
	acousticModel_(acousticModel),
	haveSuccessorBatches_(false),
	coarticulationStructure_(0),
	requests_(0)
{
	isEarlyRecombinationEnabled_	= paramEarlyRecombination(config);
	isPathRecombinationInFanInEnabled_ = paramEnforcePathRecombinationInFanIn(config);
	if (isEarlyRecombinationEnabled_) {
		warning("Implementation of early recombination is still experimental!");
		earlyRecombinationLimit_ = paramEarlyRecombinationLimit(config);
	}

	if (acousticModel_->nStateTransitions() >= Core::Type<StateDesc::AcousticModelIndex>::max) {
		criticalError("Too many transition models. Current state tree implementation handles max %d transition models",
					  Core::Type<StateDesc::TransitionModelIndex>::max);
	}

	allowCiCrossWordTransitions_ = paramCiCrossWordTransitions(config);
	if (!allowCiCrossWordTransitions_)
	    log("not building context independent across word transitions");
	allowSkipTransitions_ = paramSkipTransitions(config);

	getTransitionModels();

	if (acousticModel_->nEmissions() >= Core::Type<StateDesc::AcousticModelIndex>::max) {
		criticalError("Too many mixtures. Current state tree implementation handles max %d mixtures",
					  Core::Type<StateDesc::AcousticModelIndex>::max);
	}

	root_ = ciRoot_ = -1;

	std::string filename = paramFile(config);
	if (!filename.empty()) {
		StateTreeReader reader(lexicon, acousticModel);
		if (!reader.read(*this, filename)) {
			buildTreeLexicon(lexicon);
			StateTreeWriter writer(lexicon, acousticModel);
			writer.write(*this, filename);
			log("wrote state tee to \"%s\"", filename.c_str());
		} else {
			log("read state tree from \"%s\"", filename.c_str());
		}
	} else {
		buildTreeLexicon(lexicon);
	}

	logStatistics();
	Core::Channel dc(config, "dot");
	if (dc.isOpen()) draw(dc, phonemeInventory());
}

StateTree::~StateTree() {
#if 1
	delete coarticulationStructure_; coarticulationStructure_ = 0; initialPhonemes_ = 0;
#endif
}

/**
 * get the transit entry for the final phoneme of s with successor phoneme s.successorPhoneme_.
 * the transit entry is the root state for the coarticulated first state of s.successorPhoneme
 * with s.finalPhonhme() as predecessor.
 */
StateTree::StateId StateTree::pronunciationSuffixTransitEntry(const PronunciationSuffix &s) const {
	require(s.isEmpty());
	if (s.successorPhoneme() == Bliss::Phoneme::term) {
		if (allowCiCrossWordTransitions_ || !phonemeInventory()->isValidPhonemeId(s.finalPhoneme()) ||
			!phonemeInventory()->phoneme(s.finalPhoneme())->isContextDependent())
		    return root();
		else
		    return ciRoot();
	} else {
		verify(phonemeInventory()->phoneme(s.finalPhoneme())->isContextDependent());
		return coarticulationStructure_->root(s.finalPhoneme(), s.successorPhoneme());
	}
	defect();
}

void StateTree::getTransitionModels() {
	transitionModels_.resize(acousticModel_->nStateTransitions(), 0);
	for (u32 t = 0; t < transitionModels_.size(); ++ t)
		transitionModels_[t] = acousticModel_->stateTransition(t);
}


bool StateTree::isConsistent() const {
	bool result = true;
	if (!haveSuccessorBatches_) return result;
	// check 2nd order consistency
	for (StateId s = 0; s < nStates(); ++s) {
		StateTree::StateId ss, ss_end, sss, sss_end, ss2, ss2_end;
		tie(ss2, ss2_end) = successors2(s);
		for (tie(ss, ss_end) = successors(s); ss < ss_end; ++ss)
			for (tie(sss, sss_end) = successors(ss); sss < sss_end; ++sss) {
				bool r = (sss == ss2++);
				result &= r;
				if (!r) warning("2nd order mesh structure s=%d, ss=%d, sss=%d, ss2=%d",
								s, ss, sss, ss2);
			}
		bool r = (ss2 == ss2_end);
		if (!r) warning("2nd order mesh structure s=%d, ss2=%d, ss2_end=%d",
						s, ss2, ss2_end);
		result &= r;
	}

	return result;
}

void StateTree::buildNonCoarticulatedRootState(Bliss::LexiconRef lexicon) {
	log("creating non-coarticulated root state...");
	root_ = ciRoot_ = states_.size();
	BatchRequest *request = new BatchRequest(this, root_);
	SuffixSet &suffixes(request->suffixes);
	Bliss::Lexicon::PronunciationIterator pi, pi_end;
	for (tie(pi, pi_end) = lexicon->pronunciations(); pi != pi_end; ++pi) {
		PronunciationSuffix suffix(*pi);
		suffixes.addDirty(suffix);
	}

	State state;
	state.depth = 0;
	state.desc = suffixes.head();
	SuffixSet::ItemList ends;
	suffixes.advance(ends);
	suffixes.sortAndHash();
	createExits(ends, state.exits);
	states_.push_back(state);
	request->depth = state.depth + 1;
	requests_->submit(request);
}

void StateTree::buildCoarticulatedRootStates(Bliss::LexiconRef lexicon) {
	CoarticulationStructure &cs(*coarticulationStructure_);
	CoarticulationStructure::PhonemePair boundaryPhonemes;
	boundaryPhonemes.final = boundaryPhonemes.initial = Bliss::Phoneme::term;
	cs.boundaryPhonemes.resize(states_.size(), boundaryPhonemes);
	Core::ProgressIndicator pi("creating coarticulated root states", "states");
	pi.start(cs.initialPhonemes.size() * cs.finalPhonemes.size());
	Bliss::Lexicon::PronunciationIterator pron, pron_end;
	if (!allowCiCrossWordTransitions_) {
	    ciRoot_ = states_.size();
	    Am::LexiconUtilities lexiconUtilities(getConfiguration(), lexicon);
	    boundaryPhonemes.initial = lexiconUtilities.determineSilencePhoneme();
	    boundaryPhonemes.final = Bliss::Phoneme::term;
	    verify(cs.boundaryPhonemes.size() == ciRoot_);
	    cs.boundaryPhonemes.push_back(boundaryPhonemes);
	    BatchRequest *request = new BatchRequest(this, ciRoot_);
	    SuffixSet &suffixes(request->suffixes);
	    for (tie(pron, pron_end) = lexicon->pronunciations(); pron != pron_end; ++pron) {
		const Bliss::Pronunciation &p(**pron);
		if (p.length() < 1 || !phonemeInventory()->phoneme(p[0])->isContextDependent()) {
		    PronunciationSuffix suffix(*pron);
		    suffixes.addDirty(suffix);
		}
	    }
	    State state;
	    state.depth = 0;
	    state.desc = suffixes.head();
	    SuffixSet::ItemList ends;
	    suffixes.advance(ends);
	    suffixes.sortAndHash();
	    createExits(ends, state.exits);
	    states_.push_back(state);
	    request->depth = state.depth + 1;
	    requests_->submit(request);
	}
	for (u32 l = 0; l < cs.finalPhonemes.size(); ++l) {
		boundaryPhonemes.final = cs.finalPhonemes[l];
		for (u32 r = 0; r < cs.initialPhonemes.size(); ++r) {
			boundaryPhonemes.initial = cs.initialPhonemes[r];
			StateId root = states_.size();
			cs.roots[l][r] = root;
			verify(cs.boundaryPhonemes.size() == root);
			cs.boundaryPhonemes.push_back(boundaryPhonemes);
			BatchRequest *request = new BatchRequest(this, root);
			SuffixSet &suffixes(request->suffixes);
			for (tie(pron, pron_end) = lexicon->pronunciations(); pron != pron_end; ++pron) {
				const Bliss::Pronunciation &p(**pron);
				if (p.length() < 1 || p[0] == boundaryPhonemes.initial) {
					PronunciationSuffix suffix(*pron, boundaryPhonemes.final);
					suffixes.addDirty(suffix);
				}
			}

			State state;
			state.depth = 0;
			state.desc = suffixes.head();
			SuffixSet::ItemList ends;
			suffixes.advance(ends);
			suffixes.sortAndHash();
			createExits(ends, state.exits);
			states_.push_back(state);
			request->depth = state.depth + 1;
			requests_->submit(request);

			pi.notify();
		}
	}
}

void StateTree::buildTreeLexicon(Bliss::LexiconRef lexicon) {
	if (acousticModel_->isAcrossWordModelEnabled()) {
		coarticulationStructure_ = new CoarticulationStructure(this, lexicon);
		initialPhonemes_ = &coarticulationStructure_->initialPhonemes;
	}

	emptyBatch_ = batches_.size();
	if (acousticModel_->isAcrossWordModelEnabled() || isEarlyRecombinationEnabled_)
		requests_ = new RecombiningRequestQueue(this);
	else
		requests_ = new StraightRequestQueue(this);

	buildNonCoarticulatedRootState(lexicon);
	if (acousticModel_->isAcrossWordModelEnabled())
		buildCoarticulatedRootStates(lexicon);

	batches_.push_back(states_.size());
	batches_.push_back(states_.size());

	Core::ProgressIndicator pi("building tree lexicon", "states");
	pi.start();

	while (BuildRequest *r = requests_->pop()) {

		r->satisfy(this);
		delete r;
		pi.notify(states_.size()); // requests_->size();
	}
	pi.finish();
	verify(requests_->size() == 0);
	delete requests_; requests_ = 0;
	if (allowSkipTransitions_)
	    createSecondOrderBatches();
	else
	    log("building state tree without skip transitions");
	ensure(isConsistent());
}

/**
 * Append for each ending pronunciation an exit object with the matching
 * lemma pronunciation
 */
void StateTree::createExits(
	const SuffixSet::ItemList &endingItems,
	ExitList &exits)
{
	Exit exit;
	for (SuffixSet::ItemList::const_iterator s = endingItems.begin(); s != endingItems.end() ; ++s) {
		require(s->isEmpty());
		exit.transitEntry = pronunciationSuffixTransitEntry(*s);
		if (s->pronunciation()) {
			Bliss::Pronunciation::LemmaIterator lpi, lpi_end;
			for (tie(lpi, lpi_end) = s->pronunciation()->lemmas(); lpi != lpi_end; ++lpi) {
				exit.pronunciation = lpi;
				exits.push_front(exit);
			}
		} else {
			exit.pronunciation = 0;
			exits.push_front(exit);
		}
	}
}

bool StateTree::shouldLeap(const SuffixSet &c, Depth d) const {
	if (!isEarlyRecombinationEnabled_) return false;
	if (c.size() != 1) return false;
	if (c.hasLeaped()) return false;
	if (d < earlyRecombinationLimit_) return false;
	if ((s32)c.nStatesRemaining() < earlyRecombinationLimit_) return false;
	return true;
}

/**
 * Create successor tree nodes and submit respective build requests.
 */
StateTree::BatchId StateTree::createBatch(BatchRequest *request) {
	SuffixSet::SuffixSetList conts;
	SuffixSet::ItemList ends;

	request->suffixes.partition(conts);
	require(conts.size() > 0);

	BatchId batch = batches_.size() - 1;
	verify(batches_[batch] == StateId(states_.size()));
	State m;
	m.depth = request->depth;
	for (SuffixSet::SuffixSetList::iterator c = conts.begin(); c != conts.end(); ++c) {
		m.desc = c->head();  // before c->advance(), all pronunciation suffixes have the same head
		c->advance(ends);
		createExits(ends, m.exits);
		if (shouldLeap(*c, m.depth)) {
			LeapRequest *l = new LeapRequest(this, states_.size());
			c->leap(l->leaps.front());
			c->sortAndHash();
			l->depth = - c->nStatesRemaining();
			l->suffixes = *c;
			m.successors = emptyBatch_;
			requests_->submit(l);
		} else if (c->size() > 0) {
			c->sortAndHash();
			BatchRequest *r = new BatchRequest(this, states_.size());
			r->depth = m.depth + 1;
			r->parentsSuffixes = request->suffixes;
			r->suffixes = *c;
			m.successors = invalidBatch;
			requests_->submit(r);
		} else {
			m.successors = emptyBatch_;
		}
		states_.push_back(m);
		m.exits.clear();
	}
	batches_.push_back(states_.size());
	return batch;
}

StateTree::StateId StateTree::createLeapEntry(LeapRequest *request) {
	State state;
	state.depth = request->depth - 1;
	state.desc.acousticModel   = invalidAcousticModel;
	state.desc.transitionModelIndex = Am::TransitionModel::entryM1;
	state.successors = invalidBatch;
	Exit exit;
	exit.transitEntry = states_.size();
	states_.push_back(state);
	for (u32 i = 0; i < request->states.size(); ++i) {
		State &state(states_[request->states[i]]);
		for (std::vector<const Bliss::Pronunciation*>::const_iterator p = request->leaps[i].begin(); p != request->leaps[i].end(); ++p) {
			verify(*p);
			Bliss::Pronunciation::LemmaIterator lpi, lpi_end;
			for (tie(lpi, lpi_end) = (*p)->lemmas(); lpi != lpi_end; ++lpi) {
				exit.pronunciation = lpi;
				state.exits.push_front(exit);
			}
		}
	}
	batches_.push_back(states_.size());
	return exit.transitEntry;
}


/**
 * Fill successorBatches_, the successors of batches
 */
void StateTree::createSecondOrderBatches() {
	log("building second order structure...");

	BatchId nFirstOrderBatches = batches_.size() - 1;
	haveSuccessorBatches_ = true;

	for (BatchId b = 0; b < nFirstOrderBatches; ++b) {
		BatchId b2 = emptyBatch_;
		s32 nStates = batches_[b+1] - batches_[b];
		if (nStates == 1) {
			b2 = states_[batches_[b]].successors;
		} else if (nStates > 1) {
			StateId begin = Core::Type<StateId>::max;
			StateId end   = Core::Type<StateId>::min;
			for (StateId s = batches_[b]; s < batches_[b+1]; ++s) {
				BatchId sb = states_[s].successors;
				if (sb == emptyBatch_) continue;
				if (begin > batches_[sb])
					begin = batches_[sb];
				if (end < batches_[sb+1])
					end = batches_[sb+1];
			}
			if (begin >= end) {
				b2 = emptyBatch_;
			} else {
				if (begin == batches_[batches_.size()-2] &&
					end   == batches_[batches_.size()-1]) {
					b2 = batches_.size() - 2;
				} else if (begin == batches_[batches_.size()-1]) {
					b2 = batches_.size() - 1;
					batches_.push_back(end);
				} else {
					b2 =  batches_.size();
					batches_.push_back(begin);
					batches_.push_back(end);
				}
				verify(begin == batches_[b2  ]);
				verify(end   == batches_[b2+1]);
			}
		}
		verify(b == BatchId(successorBatches_.size()));
		successorBatches_.push_back(b2);
	}
}

// ===========================================================================
// StateTree statistics and visualization

struct StateTree::LevelStatistics {
	u32 nStates, nChildren, nExits;
	LevelStatistics() : nStates(0), nChildren(0), nExits(0) {}
};

void StateTree::logStatistics() const {
	typedef std::map<Depth, LevelStatistics> LevelStatisticsMap;
	LevelStatisticsMap levels;

	struct {
		u32 nBranchPoints, nExitPoints, nExits, nArcs;
	} total;
	total.nBranchPoints = total.nExitPoints = total.nExits = total.nArcs = 0;

	for (StateId si = 0; si < nStates(); ++si) {
		const State &s(states_[si]);
		LevelStatistics &thisLevel(levels[s.depth]);
		thisLevel.nStates += 1;
		StateId a, b; tie(a, b) = successors(si);
		u32 ch = b - a;
		thisLevel.nChildren += ch;
		total.nArcs += ch;
		u32 we = s.exits.size();
		thisLevel.nExits += we;
		total.nExits += we;
		if (ch + we > 1)
			++total.nBranchPoints;
		if (we > 0)
			++total.nExitPoints;
	}

	Message msg(log("statistics:"));
	for (LevelStatisticsMap::const_iterator l = levels.begin(); l != levels.end(); ++l) {
		Depth depth = l->first;
		const LevelStatistics &thisLevel(l->second);
		msg.form("\nlevel %3d: %6d states, branching factor %3.2f, %4d exits",
				 depth, thisLevel.nStates, float(thisLevel.nChildren)  / float(thisLevel.nStates), thisLevel.nExits);
	}
	msg.form("\ntotal braching points: %d", total.nBranchPoints);
	msg.form("\ntotal exit points: %d", total.nExitPoints);
#if 1
	msg.form("\ntotal: %d states, %d exits", nStates(), total.nExits);
#endif
}

void StateTree::draw(
	std::ostream &os,
	Core::Ref<const Bliss::PhonemeInventory> phi) const
{
	os << "digraph \"" << fullName() << "\" {" << std::endl
	   << "ranksep = 1.5" << std::endl
	   << "rankdir = LR" << std::endl
	   << "node [fontname=\"Helvetica\"]" << std::endl
	   << "edge [fontname=\"Helvetica\"]" << std::endl;

	for (StateId si = 0; si < nStates(); ++si) {
		const State &s(states_[si]);
		os << Core::form("n%d [label=\"%d\\nd=%d a=%d t=%s", si, si, s.depth,
						 s.desc.acousticModel, transitionModel(si)->name().c_str());
		for (const_ExitIterator i = s.exits.begin(); i != s.exits.end(); ++i) {
			if (i->pronunciation) {
				os << "\\n" << i->pronunciation->lemma()->preferredOrthographicForm();
				if (phi) os << " /" << i->pronunciation->pronunciation()->format(phi) << "/";
				os << " -> " << i->transitEntry;
			}
		}
		os << "\"]" << std::endl;

		for (StateId ssi = batches_[s.successors]; ssi != batches_[s.successors+1]; ++ssi)
			os << Core::form("n%d -> n%d\n", si, ssi);

		for (const_ExitIterator i = s.exits.begin(); i != s.exits.end(); ++i) {
			if (!i->pronunciation) {
				os << Core::form("n%d -> n%d [style=dashed]\n", si, i->transitEntry);
			}
		}
	}

	os << "}" << std::endl;
}
