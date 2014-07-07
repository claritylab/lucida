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
#ifndef _FLF_COMBINATION_HH
#define _FLF_COMBINATION_HH

#include "FlfCore/Lattice.hh"
#include "Network.hh"

namespace Flf {

    /**
     * Combine semiring weights and add additional dimensions for combination scores
     *
     * So far, two methods are implemented:
     * 1) discard
     *    Discard the scores from source semirings; the new semiring consists only of
     *    the combination scores.
     * 2) concatenate
     *    Concatenate the scores of the individual semirings and append the
     *    combination scores.
     **/
    class SemiringCombinationHelper;
    typedef Core::Ref<SemiringCombinationHelper> SemiringCombinationHelperRef;

    class SemiringCombinationHelper : public Core::ReferenceCounted {
    public:
	class Internal;

	typedef enum {
	    TypeDiscard     = 0,
	    TypeConcatenate = 1
	} Type;
	static const Core::ParameterString paramType;
	static Type getType(const std::string &type);

	static ScoreId getIdOrDie(ConstSemiringRef semiring, const Key &key);

    private:
	Internal *internal_;

    protected:
	SemiringCombinationHelper(Internal *internal);

    public:
	~SemiringCombinationHelper();

	// update
	bool update(const ConstSemiringRefList &semirings);

	ConstSemiringRef semiring() const;
	ConstSemiringRef subSemiring(u32 subSemiringIndex) const;

	// additional scores (aka combination scores)
	u32 nCombinationScores() const;
	ScoreId combinationId(ScoreId comboScoreIndex) const;

	// modify combination scores
	void set(ScoresRef scores, ScoreId comboScoreIndex, Score score) const;

	// sub semiring ids
	ScoreId subId(ScoreId subSemiringIndex, ScoreId subScoreIndex) const;
	ScoreId subId(ScoreId subSemiringIndex, const Key &subScoreKey) const;

	// modify sub semiring scores
	void set(ScoresRef scores, ScoreId subSemiringIndex, ScoreId subScoreIndex, Score subScore) const;
	void set(ScoresRef scores, ScoreId subSemiringIndex, ScoresRef subScores) const;

	// create helper function
	static SemiringCombinationHelperRef create(Type comboType, const KeyList &comboScoreKeys, const ScoreList &comboScales = ScoreList());
    };


    /**
     * Do some elementary checks on lattices or CNs to combine.
     * Update score combination; see class SemiringCombination
     **/
    class CombinationHelper;
    typedef Core::Ref<CombinationHelper> CombinationHelperRef;

    class CombinationHelper : public Core::ReferenceCounted {
    private:
	SemiringCombinationHelperRef semiringCombination_;
	Fsa::Type type_;
	Fsa::ConstAlphabetRef inputAlphabet_;
	Fsa::ConstAlphabetRef outputAlphabet_;

    protected:
	CombinationHelper(SemiringCombinationHelper::Type comboType, const KeyList &comboKeys, const ScoreList &comboScales);

    public:
	bool update(const ConstLatticeRefList &lats);
	bool update(const ConstConfusionNetworkRefList &cns);

	Fsa::Type type() const
	    { return type_; }

	Fsa::ConstAlphabetRef inputAlphabet() const
	    { return inputAlphabet_; }

	Fsa::ConstAlphabetRef outputAlphabet() const
	    { return outputAlphabet_; }

	ConstSemiringRef semiring() const
	    { return semiringCombination_->semiring(); }

	SemiringCombinationHelperRef semiringCombination() const
	    { return semiringCombination_; }

	static CombinationHelperRef create(SemiringCombinationHelper::Type comboType, const KeyList &comboKeys, const ScoreList &comboScales);
    };

} //namespace Flf

#endif // _FLF_COMBINATION_HH
