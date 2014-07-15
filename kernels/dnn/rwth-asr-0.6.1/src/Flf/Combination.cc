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
#include <Core/Choice.hh>

#include "Combination.hh"
#include "Lexicon.hh"


namespace Flf {

    // -------------------------------------------------------------------------
    class SemiringCombinationHelper::Internal {
    public:
	const KeyList comboKeys;
	ScoreList comboScales;
	ConstSemiringRef semiring;
	ConstSemiringRefList subSemirings;
	ScoreIdList comboIds;

    public:
	Internal(const KeyList &comboKeys, const ScoreList &comboScales) :
	    comboKeys(comboKeys), comboScales(comboScales) {}
	virtual ~Internal() {}

	virtual bool update(const ConstSemiringRefList &semirings) = 0;

	virtual ScoreId subId(ScoreId subSemiringIndex, ScoreId subScoreIndex) const
	    { return Semiring::InvalidId; }
	virtual ScoreId subId(ScoreId subSemiringIndex, const Key &subScoreKey) const
	    { return Semiring::InvalidId; }

	virtual void set(ScoresRef scores, ScoreId subSemiringIndex, ScoreId subScoreIndex, Score subScore) const {}
	virtual void set(ScoresRef scores, ScoreId subSemiringIndex, ScoresRef subScores) const {}
    };
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    /**
     * Discarding semiring combination
     **/
    class DiscardSemiringCombination : public SemiringCombinationHelper::Internal {
	typedef SemiringCombinationHelper::Internal Precursor;
    public:
	DiscardSemiringCombination(const KeyList &comboKeys, const ScoreList &comboScales) :
	    Precursor(comboKeys, comboScales) {
	    comboIds.resize(comboKeys.size());
	    for (ScoreId id = 0; id < comboIds.size(); ++id)
		comboIds[id] = id;
	}

	virtual bool update(const ConstSemiringRefList &subSemirings) {
	    this->subSemirings = subSemirings;
	    if (!semiring) {
		semiring = Semiring::create(Fsa::SemiringTypeTropical, comboKeys.size(), comboScales, comboKeys);
		return true;
	    } else
		return false;
	}
    };
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    /**
     * Concatenating semiring combination
     **/
    class ConcatenateSemiringCombination : public SemiringCombinationHelper::Internal {
	typedef SemiringCombinationHelper::Internal Precursor;
    private:
	std::vector<std::pair<ScoreId, ScoreId> > ranges_;
    public:
	ConcatenateSemiringCombination(const KeyList &comboKeys, const ScoreList &comboScales) :
	    Precursor(comboKeys, comboScales) {
	    if (this->comboScales.empty())
		this->comboScales.resize(this->comboKeys.size(), 1.0);
	    comboIds.resize(this->comboKeys.size(), Semiring::InvalidId);
	}

	virtual bool update(const ConstSemiringRefList &subSemirings) {
	    bool updated = false;
	    if (this->subSemirings.size() == subSemirings.size()) {
		for (ConstSemiringRefList::const_iterator itOld = this->subSemirings.begin(), endOld = this->subSemirings.end(), itNew = subSemirings.begin();
		     itOld != endOld; ++itOld, ++itNew)
		    if (!(itOld->get() == itNew->get()) || !(**itOld == **itNew))
			{ updated = true; break; }
	    } else
		updated = true;
	    if (updated) {
		verify(!subSemirings.empty());
		this->subSemirings = subSemirings;
		ScoreId n = 0;
		KeyList keys;
		ScoreList scales;
		for (u32 i = 0; i < subSemirings.size(); ++i) {

		    verify(this->subSemirings[i]);

		    const Semiring &subSemiring = *subSemirings[i];
		    ScoreId size = subSemiring.size();
		    ranges_.push_back(std::make_pair(n, size));
		    n += size;
		    for (KeyList::const_iterator itKey = subSemiring.keys().begin(), endKey = subSemiring.keys().end();
			 itKey != endKey; ++itKey) keys.push_back(Core::form("%d-%s", i, itKey->c_str()));
		    for (ScoreList::const_iterator itScale = subSemiring.scales().begin(), endScale = subSemiring.scales().end();
			 itScale != endScale; ++itScale) scales.push_back(*itScale);
		}
		keys.insert(keys.end(), comboKeys.begin(), comboKeys.end());
		scales.insert(scales.end(), comboScales.begin(), comboScales.end());
		for (ScoreIdList::iterator itComboId = comboIds.begin(), endComboId = comboIds.end(); itComboId != endComboId; ++itComboId, ++n)
		    *itComboId = n;
		semiring = Semiring::create(subSemirings.front()->type(), n, scales, keys);
	    }
	    return updated;
	}

	ScoreId subId(ScoreId subSemiringIndex, ScoreId subScoreIndex) const {
	    return (subScoreIndex == Semiring::InvalidId) ?
		Semiring::InvalidId :
		ranges_[subSemiringIndex].first + subScoreIndex;
	}

	ScoreId subId(ScoreId subSemiringIndex, const Key &subScoreKey) const {
	    return subId(subSemiringIndex, subSemirings[subSemiringIndex]->id(subScoreKey));
	}

	void set(ScoresRef scores, ScoreId subSemiringIndex, ScoreId subScoreIndex, Score subScore) const {
	    scores->set(ranges_[subSemiringIndex].first + subScoreIndex, subScore);
	}

	void set(ScoresRef scores, ScoreId subSemiringIndex, ScoresRef subScores) const {
	    const std::pair<ScoreId, ScoreId> &range = ranges_[subSemiringIndex];
	    Scores::iterator itScore = scores->begin() + range.first;
	    for (Scores::const_iterator itSubScore = subScores->begin(), endSubScore = subScores->begin() + range.second;
		 itSubScore != endSubScore; ++itScore, ++itSubScore) *itScore = *itSubScore;
	}
    };
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    namespace {
	Core::Choice semiringCombinationTypeChoice(
	    "discard",     SemiringCombinationHelper::TypeDiscard,
	    "concatenate", SemiringCombinationHelper::TypeConcatenate,
	    Core::Choice::endMark());
    } // namespace
    const Core::ParameterString SemiringCombinationHelper::paramType(
	"type",
	"type",
	"concatenate");
    SemiringCombinationHelper::Type SemiringCombinationHelper::getType(const std::string &s) {
	Core::Choice::Value comboType = semiringCombinationTypeChoice[s];
	if (comboType == Core::Choice::IllegalValue)
	    Core::Application::us()->criticalError("Unknown semiring combination type \"%s\"", s.c_str());
	return Type(comboType);
    }

    ScoreId SemiringCombinationHelper::getIdOrDie(ConstSemiringRef semiring, const Key &key) {
	verify_(semiring);
	ScoreId id = semiring->id(key);
	if (id == Semiring::InvalidId)
	    Core::Application::us()->criticalError(
		"Semiring \"%s\" has no dimension labeled \"%s\".",
		semiring->name().c_str(), key.c_str());
	return id;
    }

    SemiringCombinationHelperRef SemiringCombinationHelper::create(SemiringCombinationHelper::Type comboType, const KeyList &comboKeys, const ScoreList &comboScales) {
	verify(comboScales.empty() || (comboScales.size() == comboKeys.size()));
	switch (comboType) {
	case TypeDiscard:
	    return SemiringCombinationHelperRef(new SemiringCombinationHelper(new DiscardSemiringCombination(comboKeys, comboScales)));
	case TypeConcatenate:
	    return SemiringCombinationHelperRef(new SemiringCombinationHelper(new ConcatenateSemiringCombination(comboKeys, comboScales)));
	default:
	    defect();
	    return SemiringCombinationHelperRef();
	}
    }

    SemiringCombinationHelper::SemiringCombinationHelper(Internal *internal) :
	internal_(internal) {}

    SemiringCombinationHelper::~SemiringCombinationHelper() {
	delete internal_;
    }

    bool SemiringCombinationHelper::update(const ConstSemiringRefList &semirings) {
	return internal_->update(semirings);
    }

    ConstSemiringRef SemiringCombinationHelper::semiring() const {
	return internal_->semiring;
    }

    ConstSemiringRef SemiringCombinationHelper::subSemiring(u32 subSemiringIndex) const {
	return internal_->subSemirings[subSemiringIndex];
    }

    u32 SemiringCombinationHelper::nCombinationScores() const {
	return internal_->comboKeys.size();
    }

    ScoreId SemiringCombinationHelper::combinationId(ScoreId comboIndex) const {
	verify_(comboIndex < internal_->comboIds.size());
	return internal_->comboIds[comboIndex];
    }

    void SemiringCombinationHelper::set(ScoresRef scores, ScoreId comboIndex, Score score) const {
	verify_(comboIndex < internal_->comboIds.size());
	scores->set(internal_->comboIds[comboIndex], score);
    }

    ScoreId SemiringCombinationHelper::subId(ScoreId subSemiringIndex, ScoreId subScoreIndex) const {
	return internal_->subId(subSemiringIndex, subScoreIndex);
    }

    ScoreId SemiringCombinationHelper::subId(ScoreId subSemiringIndex, const Key &subScoreKey) const {
	return internal_->subId(subSemiringIndex, subScoreKey);
    }

    void SemiringCombinationHelper::set(ScoresRef scores, ScoreId subSemiringIndex, ScoreId subScoreIndex, Score subScore) const {
	internal_->set(scores, subSemiringIndex, subScoreIndex, subScore);
    }

    void SemiringCombinationHelper::set(ScoresRef scores, ScoreId subSemiringIndex, ScoresRef subScores) const {
	internal_->set(scores, subSemiringIndex, subScores);
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    namespace {
	void verifyAlphabet(Lexicon::AlphabetId alphabetId, Fsa::ConstAlphabetRef alphabet) {
	    if (Lexicon::us()->alphabetId(alphabet) != alphabetId)
		Core::Application::us()->criticalError(
		    "Cannot combine different alphabets; \"%s\" vs. \"%s\"",
		    Lexicon::us()->alphabetName(alphabetId).c_str(),
		    Lexicon::us()->alphabetName(Lexicon::us()->alphabetId(alphabet)).c_str());
	}
    } // namespace

    CombinationHelper::CombinationHelper(SemiringCombinationHelper::Type comboType, const KeyList &comboKeys, const ScoreList &comboScales) {
	semiringCombination_ = SemiringCombinationHelper::create(comboType, comboKeys, comboScales);
    }

    CombinationHelperRef CombinationHelper::create(SemiringCombinationHelper::Type comboType, const KeyList &comboKeys, const ScoreList &comboScales) {
	return CombinationHelperRef(new CombinationHelper(comboType, comboKeys, comboScales));
    }

    bool CombinationHelper::update(const ConstLatticeRefList &lats) {
	verify(!lats.empty());
	type_ = lats.front()->type();
	Lexicon::AlphabetId inputAlphabetId = Lexicon::us()->alphabetId(lats.front()->getInputAlphabet());
	Lexicon::AlphabetId outputAlphabetId = (type_ == Fsa::TypeAcceptor) ?
	    Lexicon::InvalidAlphabetId :
	    Lexicon::us()->alphabetId(lats.front()->getOutputAlphabet());
	ConstSemiringRefList semirings(lats.size());
	ConstSemiringRefList::iterator itSemiring = semirings.begin();
	for (ConstLatticeRefList::const_iterator itLat = lats.begin(); itLat != lats.end(); ++itLat, ++itSemiring) {
	    verify((*itLat));
	    verifyAlphabet(inputAlphabetId, (*itLat)->getInputAlphabet());
	    if ((*itLat)->type() == Fsa::TypeAcceptor)
		type_ = Fsa::TypeAcceptor;
	    else
		verifyAlphabet(outputAlphabetId, (*itLat)->getOutputAlphabet());
	    *itSemiring = (*itLat)->semiring();
	}
	inputAlphabet_ = lats.front()->getInputAlphabet();
	outputAlphabet_ = (type_ == Fsa::TypeAcceptor) ?
	    Fsa::ConstAlphabetRef() :
	    lats.front()->getOutputAlphabet();
	return  semiringCombination_->update(semirings);
    }

    bool CombinationHelper::update(const ConstConfusionNetworkRefList &cns) {
	verify(!cns.empty());
	type_ = Fsa::TypeAcceptor;
	inputAlphabet_ = cns.front()->alphabet;
	outputAlphabet_ = Fsa::ConstAlphabetRef();
	Lexicon::AlphabetId alphabetId = Lexicon::us()->alphabetId(inputAlphabet_);
	ConstSemiringRefList semirings(cns.size());
	ConstSemiringRefList::iterator itSemiring = semirings.begin();
	for (ConstConfusionNetworkRefList::const_iterator itCn = cns.begin(); itCn != cns.end(); ++itCn, ++itSemiring) {
	    verify((*itCn));
	    verifyAlphabet(alphabetId, (*itCn)->alphabet);
	    *itSemiring = (*itCn)->semiring;

	    verify(*itSemiring);

	}
	return semiringCombination_->update(semirings);
    }
    // -------------------------------------------------------------------------

} // namespace Flf
