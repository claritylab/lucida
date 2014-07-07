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
#include <Core/ProgressIndicator.hh>
#include <Core/Utility.hh>

#include "FlfCore/Utility.hh"
#include "Best.hh"
#include "Combination.hh"
#include "ConfusionNetwork.hh"
#include "ConfusionNetworkCombination.hh"
#include "Lexicon.hh"


namespace Flf {

    class ConfusionNetworkAlignment;
    typedef std::vector<ConfusionNetworkAlignment*> ConfusionNetworkAlignmentPtrList;

    // -------------------------------------------------------------------------
    class ConfusionNetworkAlignment {
    public:
	class Cost : public Core::ReferenceCounted {
	public:
	    static const Score Infinity;
	protected:
	    mutable const ConfusionNetwork *refPtr_, *hypPtr_;
	protected:
	    inline const ConfusionNetwork::Slot & refSlot(u32 refId) const
		{ verify_(refPtr_); return (*refPtr_)[refId]; }
	    inline const ConfusionNetwork::Slot & hypSlot(u32 hypId) const
		{ verify_(hypPtr_); return (*hypPtr_)[hypId]; }
	public:
	    Cost() : refPtr_(0), hypPtr_(0) {}
	    virtual ~Cost() {}
	    // called before the alignment of ref and hyp starts, i.e. before the first call to the cost functions
	    virtual void init(const ConfusionNetwork &ref, const ConfusionNetwork &hyp) const
		{ refPtr_ = &ref; hypPtr_ = &hyp; }
	    // called after the alignment, i.e. after the last call to the cost functions
	    virtual void reset() const
		{ refPtr_ = 0; hypPtr_ = 0; }
	    virtual std::string describe() const = 0;
	    virtual Score insCost(u32 hypId) const = 0;
	    virtual Score delCost(u32 refId) const = 0;
	    virtual Score subCost(u32 refId, u32 hypId) const = 0;
	};
	typedef Core::Ref<const Cost> ConstCostRef;
	typedef std::vector<std::pair<u32,u32> > Alignment;
	typedef Alignment::const_reverse_iterator const_iterator;

    private:
	typedef enum Operation {
	    Invalid,
	    Start,
	    Substitution,
	    Deletion,
	    Insertion
	} OperationType;

    private:
	static const Score MaxCost;

    private:
	ConstCostRef costFcn_;
	u32 minBeamWidth_;
	mutable u32 beamWidth_, beamDiameter_;
	mutable u32 maxLen_;
	mutable Score *D_;
	mutable OperationType *B_;
	mutable Alignment revAlignment_;

    private:
	/**
	 * Allocate memory,
	 * adjust beam such that a successful alignment is guaranteed
	 **/
	void allocate(u32 lenRef, u32 lenHyp) const {
	    verify((lenRef > 0) && (lenHyp > 0));
	    u32 requiredBeamWidth = (lenRef > lenHyp) ? lenRef - lenHyp : lenHyp - lenRef;
	    u32 requestedBeamWidth = (requiredBeamWidth > minBeamWidth_) ? requiredBeamWidth : minBeamWidth_;
	    if (requestedBeamWidth != beamWidth_) {
		beamWidth_ = requestedBeamWidth;
		beamDiameter_ = 2 * beamWidth_ + 1;
		delete [] D_;
		D_ = new Score[beamDiameter_];
		maxLen_ = 0;
	    }
	    verify(D_);
	    if (lenHyp + 1 > maxLen_) {
		maxLen_ = lenHyp + 1;
		delete [] B_;
		B_ = new OperationType[beamDiameter_ * maxLen_];
	    }
	    verify(B_);
	    revAlignment_.reserve(maxLen_ + beamDiameter_);
	}

    public:
	ConfusionNetworkAlignment(ConstCostRef costFcn, u32 minBeamWidth) :
	    costFcn_(costFcn),
	    minBeamWidth_(minBeamWidth), beamWidth_(0), beamDiameter_(0), maxLen_(0), D_(0), B_(0) {
	    verify(minBeamWidth_ >= 0);
	}

	~ConfusionNetworkAlignment() {
	    delete [] D_;
	    delete [] B_;
	}

	void dump(std::ostream &os) const {
	    os << "Alignment" << std::endl;
	    os << "  min. beam width: " << minBeamWidth_ << std::endl;
	    os << "  cost function:   " << costFcn_->describe() << std::endl;
	}

	ConstCostRef cost() {
	    return costFcn_;
	}

	u32 beamWidth() const {
	    return minBeamWidth_;
	}

	const_iterator begin() const {
	    return revAlignment_.rbegin();
	}

	const_iterator end() const {
	    return revAlignment_.rend();
	}

	/**
	 * Levenshtein alignment with fixed beam around main diagonal
	 **/
	Score align(const ConfusionNetwork &ref, const ConfusionNetwork &hyp) const {
	    const Cost &costFcn = *costFcn_;
	    costFcn.init(ref, hyp);
	    const u32 lenRef = ref.size(), lenHyp = hyp.size();
	    // special case: empty hypothesis
	    if (lenHyp == 0) {
		Score D = 0.0;
		revAlignment_.clear();
		revAlignment_.reserve(lenRef);
		for (s32 refIndex = lenRef - 1; refIndex >= 0; --refIndex) {
		    D += costFcn.delCost(refIndex);
		    revAlignment_.push_back(std::make_pair(refIndex, Core::Type<u32>::max));
		}
		// return alignment cost
		return D;
	    }
	    // special case: empty reference
	    if (lenRef == 0) {
		Score D = 0.0;
		revAlignment_.clear();
		revAlignment_.reserve(lenHyp);
		for (s32 hypIndex = lenHyp - 1; hypIndex >= 0; --hypIndex) {
		    D += costFcn.insCost(hypIndex);
		    revAlignment_.push_back(std::make_pair(Core::Type<u32>::max, hypIndex));
		}
		// return alignment cost
		return D;
	    }
	    // common case:
	    /*
	      The beam is assured to be wide enough to allow a successful alinment.
	      Allocate the required memory.
	    */
	    allocate(lenRef, lenHyp);
	    verify((lenHyp <= lenRef + beamWidth_) && (lenRef <= lenHyp + beamWidth_));
	    /*
	      Initialize alignment.
	    */
	    for (u32 beamIndex = 0; beamIndex < beamWidth_; ++beamIndex) {
		D_[beamIndex] = MaxCost;
		B_[beamIndex] = Invalid;
	    }
	    D_[beamWidth_] = 0.0;
	    B_[beamWidth_] = Start;
	    for (u32 refIndex = 0; refIndex < std::min(beamWidth_, lenRef); ++refIndex) {
		verify(beamWidth_ + 1 + refIndex < beamDiameter_);
		D_[beamWidth_ + 1 + refIndex] = D_[beamWidth_ + refIndex] + costFcn.delCost(refIndex);
		verify(beamWidth_ + 1 + refIndex < beamDiameter_ * maxLen_);
		B_[beamWidth_ + 1 + refIndex] = Deletion;
	    }
	    /*
	      Perform alignment; iterate over hypothesis.
	    */
	    Core::ProgressIndicator pi(Core::form("align(#hyp=%d,beam=%d)", lenHyp, beamDiameter_));
	    pi.start(lenHyp);
	    for (u32 hypIndex = 0, bptrIndex = beamDiameter_; hypIndex < lenHyp; ++hypIndex, bptrIndex += beamDiameter_, pi.notify()) {
		/*
		  Adjust beam start.
		  Insert insertion at bottom of beam, if still in initialization phase of beam.
		 */
		u32 beamStart = 0, beamIndex = 0;
		if (hypIndex < beamWidth_) {
		    beamStart = beamIndex = beamWidth_ - hypIndex;
		    verify(B_[bptrIndex + beamIndex - (beamDiameter_ - 1)] != Invalid);
		    verify(beamIndex < beamDiameter_);
		    D_[beamIndex - 1] = D_[beamIndex] + costFcn.insCost(hypIndex);
		    verify(bptrIndex + beamIndex - 1 < beamDiameter_ * maxLen_);
		    B_[bptrIndex + beamIndex - 1] = Insertion;
		}
		/*
		  Adjust begin and end of reference index.
		  Iterate over reference slice.
		 */
		for (u32 refIndex = (hypIndex < beamWidth_) ? 0 : hypIndex - beamWidth_, refEnd = std::min(hypIndex + beamWidth_ + 1, lenRef);
		     refIndex < refEnd; ++refIndex, ++beamIndex) {
		    verify(B_[bptrIndex + beamIndex - beamDiameter_] != Invalid); // substitution
		    Score minCost = D_[beamIndex] + costFcn.subCost(refIndex, hypIndex);
		    OperationType op = Substitution;
		    if (beamIndex > beamStart) {
			verify(B_[bptrIndex + beamIndex - 1] != Invalid); // deletion
			Score delCost = D_[beamIndex - 1] + costFcn.delCost(refIndex);
			if (delCost < minCost)
			    { minCost = delCost; op = Deletion; }
		    }
		    if (beamIndex + 1 < beamDiameter_) {
			verify(B_[bptrIndex + beamIndex - (beamDiameter_ - 1)] != Invalid); // insertion
			Score insCost = D_[beamIndex + 1] + costFcn.insCost(hypIndex);
			if (insCost < minCost)
			    { minCost = insCost; op = Insertion; }
		    }
		    verify(beamIndex < beamDiameter_);
		    D_[beamIndex] = minCost;
		    verify(bptrIndex + beamIndex < beamDiameter_ * maxLen_);
		    B_[bptrIndex + beamIndex] = op;
		}
		/*
		  Invalidate rest of beam.
		 */
		for (; beamIndex < beamDiameter_; ++beamIndex) {
		    verify(beamIndex < beamDiameter_);
		    D_[beamIndex] = MaxCost;
		    verify(bptrIndex + beamIndex < beamDiameter_ * maxLen_);
		    B_[bptrIndex + beamIndex] = Invalid;
		}
	    }
	    pi.finish(false);
	    costFcn.reset();
	    /*
	      Trace
	    */
	    revAlignment_.clear();
	    u32
		refIndex = lenRef - 1,
		hypIndex = lenHyp - 1,
		bptrIndex = beamDiameter_ * lenHyp + beamWidth_ + lenRef - lenHyp;
	    verify(bptrIndex < beamDiameter_ * maxLen_);
	    while (bptrIndex > beamWidth_) {
		switch (B_[bptrIndex]) {
		case Substitution:
		    revAlignment_.push_back(std::make_pair(refIndex, hypIndex));
		    --refIndex;
		    --hypIndex;
		    bptrIndex -= beamDiameter_;
		    break;
		case Deletion:
		    revAlignment_.push_back(std::make_pair(refIndex, Core::Type<u32>::max));
		    --refIndex;
		    bptrIndex -= 1;
		    break;
		case Insertion:
		    revAlignment_.push_back(std::make_pair(Core::Type<u32>::max, hypIndex));
		    --hypIndex;
		    bptrIndex -= (beamDiameter_ - 1);
		    break;
		default:
		    defect();
		}
	    }
	    verify(B_[bptrIndex] == Start);
	    // return alignment cost
	    verify(beamWidth_ + lenRef - lenHyp < beamDiameter_);
	    return D_[beamWidth_ + lenRef - lenHyp];
	}
    };
    const Score ConfusionNetworkAlignment::Cost::Infinity = Core::Type<Score>::max;
    const Score ConfusionNetworkAlignment::MaxCost = 0.5 * Core::Type<Score>::max;
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    namespace {
	const Core::ParameterInt paramBeamWidth(
	    "beam-width",
	    "minimum beam width",
	    100, 0);

	/**
	 * Weighted posterior combination
	 * - Normalizes the weights; discards sources with weight equals zero
	 * - Manages the scores of the combination result
	 * - Checks the lattices or CNs to combine for consistency
	 * - Provides the dimension storing the posterior prob. dist. for each source lattice/CN,
	 *   if required
	 **/
	class WeightedPosteriorCombinationHelper;
	typedef Core::Ref<WeightedPosteriorCombinationHelper> WeightedPosteriorCombinationHelperRef;

	class WeightedPosteriorCombinationHelper : public CombinationHelper {
	    typedef CombinationHelper Precursor;
	public:
	    static const Core::ParameterFloat paramWeight;
	    static const Core::ParameterString paramPosteriorKey;
	    static const Core::ParameterString paramConfidenceKey;
	    static const Core::ParameterBool paramConcatenateScores;
	public:
	    typedef u32 Id;
	    typedef std::vector<Id> IdList;
	private:
	    IdList indices_;
	    ScoreList weights_;
	    KeyList subPosteriorKeys_;
	    ScoreIdList subPosteriorIds_;
	public:
	    WeightedPosteriorCombinationHelper(
		const ScoreList &weights,
		const KeyList &subPosteriorKeys,
		SemiringCombinationHelper::Type comboType, const KeyList &comboKeys, const ScoreList &comboScales) :
		Precursor(comboType, comboKeys, comboScales) {
		verify(weights.size() == subPosteriorKeys.size());
		Score sum = 0.0;
		for (u32 i = 0; i < weights.size(); ++i) {
		    Score weight = weights[i];
		    if (weight > 0.0) {
			indices_.push_back(i);
			weights_.push_back(weight);
			subPosteriorKeys_.push_back(subPosteriorKeys[i]);
			sum += weight;
		    } else if (weight < 0.0)
			Core::Application::us()->criticalError("Negative weights are not allowed; %.3f < 0.0", weight);
		}
		if (sum == 0.0)
		    Core::Application::us()->criticalError("Sum of weights must not be zero.");
		for (ScoreList::iterator itWeight = weights_.begin(); itWeight != weights_.end(); ++itWeight)
		    *itWeight /= sum;
		subPosteriorIds_.resize(subPosteriorKeys_.size(), Semiring::InvalidId);
	    }

	    u32 size() const { return indices_.size(); }
	    const IdList & indices() const { return indices_; }
	    //const ConstSemiringRefList & semirings() const { return ; }
	    const ScoreList & weights() const { return weights_; }
	    const ScoreIdList & posteriorIds() const { return subPosteriorIds_; }
	    const KeyList & posteriorKeys() const { return subPosteriorKeys_; }

	    void dump(std::ostream &os) const {
		os << "Weighted combination" << std::endl;
		for (u32 i = 0; i < size(); ++i) {
		    const SemiringCombinationHelper &semiringCombo = *semiringCombination();
		    os << Core::form("  %2d. index=%2d, weight=%.3f", (i + 1), indices_[i], weights_[i]);
		    if (subPosteriorIds_[i] != Semiring::InvalidId)
			os << Core::form(", posterior-dimension=\"%s\"(d=%zu)",
					 semiringCombo.subSemiring(i)->key(subPosteriorIds_[i]).c_str(), subPosteriorIds_[i]);
		    os << std::endl;
		}
		if (semiring())
		    os << "  Combined semiring: " << semiring()->name() << std::endl;
	    }

	    bool update(const ConstLatticeRefList &lats, bool posteriorIdsOrDie) {
		verify(lats.size() == indices_.size());
		if (Precursor::update(lats)) {
		    // update posterior ids
		    const SemiringCombinationHelper &semiringCombo = *semiringCombination();
		    for (u32 i = 0; i < subPosteriorKeys_.size(); ++i)
			if (!subPosteriorKeys_[i].empty())
			    subPosteriorIds_[i] = SemiringCombinationHelper::getIdOrDie(semiringCombo.subSemiring(i), subPosteriorKeys_[i]);
			else if (posteriorIdsOrDie)
			    Core::Application::us()->criticalError(
				"No posterior dimension specified.");
		    return true;
		} else
		    return false;
	    }

	    bool update(const ConstConfusionNetworkRefList &cns, bool posteriorIdsOrDie) {
		verify(cns.size() == indices_.size());
		if (Precursor::update(cns)) {
		    // update posterior ids
		    const SemiringCombinationHelper &semiringCombo = *semiringCombination();
		    for (u32 i = 0; i < subPosteriorKeys_.size(); ++i) {
			if (subPosteriorKeys_[i].empty()) {
			    if (cns[i]->isNormalized()) {
				subPosteriorIds_[i] = cns[i]->normalizedProperties->posteriorId;
			    } else {
				if (posteriorIdsOrDie)
				    Core::Application::us()->criticalError(
					"Failed to find posterior dimension: Cn is not normalized and no posterior dimension is given.");
				subPosteriorIds_[i] = Semiring::InvalidId;
			    }
			} else
			    subPosteriorIds_[i] = SemiringCombinationHelper::getIdOrDie(semiringCombo.subSemiring(i), subPosteriorKeys_[i]);
		    }
		    return true;
		} else
		    return false;
	    }

	    static WeightedPosteriorCombinationHelperRef create(
		const ScoreList &weights,
		const KeyList &subPosteriorKeys,
		SemiringCombinationHelper::Type comboType, const KeyList &comboKeys, const ScoreList &comboScales) {
		return WeightedPosteriorCombinationHelperRef(new WeightedPosteriorCombinationHelper(weights, subPosteriorKeys, comboType, comboKeys, comboScales));
	    }

	    static WeightedPosteriorCombinationHelperRef create(
		const ScoreList &weights,
		const KeyList &subPosteriorKeys,
		SemiringCombinationHelper::Type comboType, const Key &posteriorKey) {
		if (posteriorKey.empty())
		    Core::Application::us()->criticalError("Posterior combination requires a valid posterior dimension key.");
		return create(weights, subPosteriorKeys, comboType, KeyList(1, posteriorKey), ScoreList(1, 0.0));
	    }
	};
	const Core::ParameterFloat WeightedPosteriorCombinationHelper::paramWeight(
	    "weight",
	    "weight",
	    1.0, 0.0);
	const Core::ParameterString WeightedPosteriorCombinationHelper::paramPosteriorKey(
	    "posterior-key",
	    "posterior key",
	    "posterior");
	const Core::ParameterString WeightedPosteriorCombinationHelper::paramConfidenceKey(
	    "confidence-key",
	    "confidence key",
	    "confidence");
    } // namespace
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class ConfusionNetworkCombination;
    typedef Core::Ref<const ConfusionNetworkCombination> ConstConfusionNetworkCombinationRef;

    class ConfusionNetworkCombination : public ConfusionNetworkAlignment, public Core::ReferenceCounted {
	typedef ConfusionNetworkAlignment Precursor;
    public:
	class WeightedCost;
	typedef Core::Ref<WeightedCost> WeightedCostRef;

	class WeightedCost : public ConfusionNetworkAlignment::Cost {
	protected:
	    ScoreId refPosteriorId, hypPosteriorId;
	    Score refWeight, hypWeight;
	    Score refNorm, hypNorm;
	    Score refNormedWeight, hypNormedWeight;
	public:
	    WeightedCost() :
		refWeight(0.5), hypWeight(0.5),
		refNorm(1.0), hypNorm(1.0),
		refNormedWeight(0.5), hypNormedWeight(0.5) {}
	    virtual ~WeightedCost() {}
	    void setPosteriorIds(ScoreId refPosteriorId, ScoreId hypPosteriorId) {
		verify((refPosteriorId != Semiring::InvalidId) && (hypPosteriorId != Semiring::InvalidId));
		this->refPosteriorId = refPosteriorId; this->hypPosteriorId = hypPosteriorId;
	    }
	    void setWeights(Score refWeight, Score hypWeight) {
		verify(Core::isAlmostEqualUlp(refWeight + hypWeight, Score(1.0), 10));
		this->refWeight = refWeight; this->hypWeight = hypWeight;
		this->refNormedWeight = refWeight / refNorm; this->hypNormedWeight = hypWeight / hypNorm;
	    }
	    void setNorms(Score refNorm, Score hypNorm) {
		this->refNorm = refNorm; this->hypNorm = hypNorm;
		this->refNormedWeight = refWeight / refNorm; this->hypNormedWeight = hypWeight / hypNorm;
	    }
	};

	/**
	 * The expected number of errors caused by this slot
	 * sum_{w,v; w != v} p_{ref}(w) * p_{hyp}(v))
	 **/
	class ExpectedLoss : public WeightedCost {
	public:
	    ExpectedLoss() {}
	    virtual std::string describe() const { return "expected-loss"; }

	    virtual Score insCost(u32 hypId) const {
		const ConfusionNetwork::Slot &hyp = hypSlot(hypId);
		Score cost = 0.001; // prefer substitutions rather than del/ins
		for (ConfusionNetwork::Slot::const_iterator itHyp = hyp.begin(), endHyp = hyp.end(); itHyp != endHyp; ++itHyp)
		    if (itHyp->label != Fsa::Epsilon)
			cost += itHyp->scores->get(hypPosteriorId);
		return cost;
	    }
	    virtual Score delCost(u32 refId) const {
		const ConfusionNetwork::Slot &ref = refSlot(refId);
		Score cost = 0.001; // prefer substitutions rather than del/ins
		for (ConfusionNetwork::Slot::const_iterator itRef = ref.begin(), endRef = ref.end(); itRef != endRef; ++itRef)
		    if (itRef->label != Fsa::Epsilon)
			cost += itRef->scores->get(refPosteriorId);
		return cost;
	    }
	    virtual Score subCost(u32 refId, u32 hypId) const {
		const ConfusionNetwork::Slot &ref = refSlot(refId), &hyp = hypSlot(hypId);
		Score cost = 0.0;
		for (ConfusionNetwork::Slot::const_iterator itHyp = hyp.begin(), endHyp = hyp.end(); itHyp != endHyp; ++itHyp) {
		    Fsa::LabelId hypLabel = itHyp->label;
		    Score hypP = itHyp->scores->get(hypPosteriorId);
		    for (ConfusionNetwork::Slot::const_iterator itRef = ref.begin(), endRef = ref.end(); itRef != endRef; ++itRef)
			if (hypLabel != itRef->label)
			    cost += hypP * itRef->scores->get(refPosteriorId);
		}
		return cost;
	    }
	    static WeightedCostRef create() { return WeightedCostRef(new ExpectedLoss); }
	};

	/**
	 * The probability that the slot's top hypothesis will be wrong
	 * 1.0 - max_{w} { w_{ref} * p_{ref}(w) + w_{hyp} * p_{hyp}(w) }
	 **/
	class ExpectedError : public WeightedCost {
	private:
	    Score delInsCost(const ConfusionNetwork::Slot &slot, ScoreId posteriorId, Score slotWeight, Score epsP) const {
		Score p = 0.0, maxP = epsP;
		if (slot.front().label == Fsa::Epsilon) {
		    maxP += slotWeight * slot.front().scores->get(posteriorId);
		} else {
		    p = slotWeight * slot.front().scores->get(posteriorId);
		    if (p > maxP) maxP = p;
		}
		for (ConfusionNetwork::Slot::const_iterator itSlot = slot.begin() + 1, endSlot = slot.end(); itSlot != endSlot; ++itSlot) {
		    p = slotWeight * itSlot->scores->get(posteriorId);
		    if (p > maxP) maxP = p;
		}
		verify((0.0 <= maxP) && (maxP <= 1.001));
		return std::max(0.0, 1.0 - maxP) + 0.001; // prefer substitutions rather than del/ins
	    }
	public:
	    ExpectedError() {}
	    virtual std::string describe() const { return "expected-error"; }

	    virtual Score insCost(u32 hypId) const {
		const ConfusionNetwork::Slot &hyp = hypSlot(hypId);
		return delInsCost(hyp, hypPosteriorId, hypNormedWeight, refWeight);
	    }
	    virtual Score delCost(u32 refId) const {
		const ConfusionNetwork::Slot &ref = refSlot(refId);
		return delInsCost(ref, refPosteriorId, refNormedWeight, hypWeight);
	    }
	    virtual Score subCost(u32 refId, u32 hypId) const {
		const ConfusionNetwork::Slot &ref = refSlot(refId), &hyp = hypSlot(hypId);
		ConfusionNetwork::Slot::const_iterator
		    itRef  = ref.begin(), endRef = ref.end(),
		    itHyp  = hyp.begin(), endHyp = hyp.end();
		Score p = 0.0, maxP = 0.0;
		for (Fsa::LabelId lastLabel = Core::Type<Fsa::LabelId>::max, nextLabel = itHyp->label;
		     nextLabel != Core::Type<Fsa::LabelId>::max; ) {
		    for (; (itRef != endRef) && (itRef->label <= nextLabel); ++itRef) {
			p = refNormedWeight * itRef->scores->get(refPosteriorId);
			if (p > maxP) maxP = p;
			lastLabel = itRef->label;
		    }
		    if ((itHyp != endHyp) && (itHyp->label == lastLabel)) {
			p += hypNormedWeight * itHyp->scores->get(hypPosteriorId);
			if (p > maxP) maxP = p;
		    }
		    nextLabel = (itRef != endRef) ? itRef->label : Core::Type<Fsa::LabelId>::max;
		    for (; (itHyp != endHyp) && (itHyp->label < nextLabel); ++itHyp) {
			p = hypNormedWeight * itHyp->scores->get(hypPosteriorId);
			if (p > maxP) maxP = p;
		    }
		    nextLabel = (itHyp != endHyp) ? itHyp->label : Core::Type<Fsa::LabelId>::max;
		}
		verify((0.0 <= maxP) && (maxP <= 1.001));
		return std::max(0.0, 1.0 - maxP);
	    }
	    static WeightedCostRef create() { return WeightedCostRef(new ExpectedError); }
	};

    private:
	WeightedPosteriorCombinationHelperRef weightedCombo_;
	WeightedCostRef weightedCostFcn_;
	mutable ScoreList sumWeights_;

    private:
	/**
	 * Build CN by merging the aligned slots
	 **/
	void init(ConfusionNetwork *combinedCn, ConstConfusionNetworkRef cn) const {
	    Score weight = weightedCombo_->weights()[0];
	    ScoreId posteriorId = cn->normalizedProperties->posteriorId;
	    const SemiringCombinationHelper &semiringCombo = *weightedCombo_->semiringCombination();
	    const Semiring &combinedSemiring = *semiringCombo.semiring();
	    ScoreId combinedPosteriorId = semiringCombo.combinationId(0);
	    ScoresRef insOne = combinedSemiring.one();
	    combinedCn->alphabet = cn->alphabet;
	    combinedCn->semiring = semiringCombo.semiring();
	    combinedCn->normalizedProperties = ConfusionNetwork::ConstNormalizedPropertiesRef(
		new ConfusionNetwork::NormalizedProperties(combinedPosteriorId));
	    combinedCn->resize(cn->size());
	    ConfusionNetwork::iterator itCombinedSlot = combinedCn->begin();
	    for (ConfusionNetwork::const_iterator itSlot = cn->begin(), endSlot = cn->end(); itSlot != endSlot; ++itSlot, ++itCombinedSlot) {

		// dbg("INS " << combinedCn->alphabet->symbol(itSlot->front().label) << " -> " << combinedCn->alphabet->symbol(Fsa::Epsilon));

		itCombinedSlot->resize(itSlot->size());
		ConfusionNetwork::Slot::iterator itCombinedArc = itCombinedSlot->begin();
		for (ConfusionNetwork::Slot::const_iterator itArc = itSlot->begin(), endArc = itSlot->end(); itArc != endArc; ++itArc, ++itCombinedArc) {
		    const ConfusionNetwork::Arc &arc = *itArc;
		    ConfusionNetwork::Arc &combinedArc = *itCombinedArc;
		    ScoresRef combinedScores = combinedSemiring.clone(insOne);
		    semiringCombo.set(combinedScores, 0, arc.scores);
		    combinedScores->set(combinedPosteriorId, weight * arc.scores->get(posteriorId));
		    combinedArc.label = arc.label;
		    combinedArc.scores = combinedScores;
		    combinedArc.begin = arc.begin;
		    combinedArc.duration = arc.duration;
		}
	    }
	}

	void add(const ConfusionNetworkAlignment &alignment, ConfusionNetwork *combinedCn, ConstConfusionNetworkRef cn, u32 i) const {
	    Score refWeight = sumWeights_[i - 1];
	    Score hypWeight = weightedCombo_->weights()[i];
	    ScoreId posteriorId = cn->normalizedProperties->posteriorId;
	    const SemiringCombinationHelper &semiringCombo = *weightedCombo_->semiringCombination();
	    const Semiring &combinedSemiring = *semiringCombo.semiring();
	    ScoreId combinedPosteriorId = combinedCn->normalizedProperties->posteriorId;
	    ScoresRef subOne = combinedSemiring.one();
	    ScoresRef insOne = combinedSemiring.clone(combinedSemiring.one());
	    ScoresRef delOne = combinedSemiring.clone(combinedSemiring.one());
	    {
		const ScoreIdList &posteriorIds = weightedCombo_->posteriorIds();
		for (u32 j = 0; j < i; ++j)
		    semiringCombo.set(insOne, j, posteriorIds[j], 1.0);
		semiringCombo.set(delOne, i, posteriorId, 1.0);
	    }
	    ConfusionNetwork::const_iterator itSlot = cn->begin();
	    ConfusionNetwork::iterator itCombinedSlot = combinedCn->begin();
	    for (const_iterator itPair = alignment.begin(), endPair = alignment.end(); itPair != endPair; ++itPair) {
		if (itPair->first == Core::Type<u32>::max) {
		    // insertion
		    // insert slot

		    //dbg("INS " << combinedCn->alphabet->symbol(itSlot->front().label) << " -> " << combinedCn->alphabet->symbol(Fsa::Epsilon)
		    //<< " " << hypWeight * itSlot->front().scores->get(posteriorId));

		    itCombinedSlot = combinedCn->insert(itCombinedSlot, *itSlot);
		    // first arc = epsilon arc, add missing eps mass
		    if (itCombinedSlot->front().label != Fsa::Epsilon) {
			ScoresRef combinedScores = combinedSemiring.clone(insOne);
			combinedScores->set(combinedPosteriorId, refWeight);
			itCombinedSlot->insert(itCombinedSlot->begin(), ConfusionNetwork::Arc(
						   Fsa::Epsilon, combinedScores,
						   itCombinedSlot->front().begin + itCombinedSlot->front().duration / 2, 0));
		    } else {
			ConfusionNetwork::Arc &combinedArc = itCombinedSlot->front();
			ScoresRef subScores = combinedArc.scores;
			ScoresRef combinedScores = combinedArc.scores = combinedSemiring.clone(insOne);
			semiringCombo.set(combinedScores, i, subScores);
			combinedScores->set(combinedPosteriorId, hypWeight * subScores->get(posteriorId) + refWeight);
			combinedArc.from = combinedArc.to = Core::Type<u32>::max;
		    }
		    // other arcs
		    for (ConfusionNetwork::Slot::iterator itCombinedArc = itCombinedSlot->begin() + 1, endCombinedArc = itCombinedSlot->end();
			 itCombinedArc != endCombinedArc; ++itCombinedArc) {
			ConfusionNetwork::Arc &combinedArc = *itCombinedArc;
			ScoresRef subScores = combinedArc.scores;
			ScoresRef combinedScores = combinedArc.scores = combinedSemiring.clone(subOne);
			semiringCombo.set(combinedScores, i, subScores);
			combinedScores->set(combinedPosteriorId, hypWeight * subScores->get(posteriorId));
			combinedArc.from = combinedArc.to = Core::Type<u32>::max;
		    }
		    ++itSlot; ++itCombinedSlot;
		} else if (itPair->second == Core::Type<u32>::max) {
		    // deletion
		    // add missing eps mass

		    //dbg("DEL " << combinedCn->alphabet->symbol(Fsa::Epsilon) << " -> " << combinedCn->alphabet->symbol(itCombinedSlot->front().label));

		    if (itCombinedSlot->front().label != Fsa::Epsilon) {
			ScoresRef combinedScores = combinedSemiring.clone(delOne);
			combinedScores->set(combinedPosteriorId, hypWeight);
			itCombinedSlot->insert(itCombinedSlot->begin(), ConfusionNetwork::Arc(
						   Fsa::Epsilon, combinedScores,
						   itCombinedSlot->front().begin + itCombinedSlot->front().duration / 2, 0));
		    } else {
			ScoresRef combinedScores = itCombinedSlot->front().scores;
			semiringCombo.set(combinedScores, i, posteriorId, 1.0);
			combinedScores->add(combinedPosteriorId, hypWeight);
		    }
		    ++itCombinedSlot;
		} else {
		    // substitution
		    ConfusionNetwork::Slot &combinedSlot = *itCombinedSlot;
		    ConfusionNetwork::Slot::iterator itCombinedArc = combinedSlot.begin();
		    for (ConfusionNetwork::Slot::const_iterator itArc = itSlot->begin(), endArc = itSlot->end(); itArc != endArc;) {
			for (; (itCombinedArc != combinedSlot.end()) && (itCombinedArc->label < itArc->label); ++itCombinedArc);
			if ((itCombinedArc != combinedSlot.end()) && (itCombinedArc->label == itArc->label)) {
			    ScoresRef combinedScores = itCombinedArc->scores;
			    ScoresRef subScores = itArc->scores;
			    Score
				refPosterior = combinedScores->get(combinedPosteriorId),
				hypPosterior = hypWeight * subScores->get(posteriorId);
			    Score sumPosterior = refPosterior + hypPosterior;
			    itCombinedArc->begin = Time(
				Core::round((refPosterior * Score(itCombinedArc->begin) + hypPosterior * Score(itArc->begin)) / sumPosterior));
			    itCombinedArc->duration = Time(
				Core::round((refPosterior * Score(itCombinedArc->duration) + hypPosterior * Score(itArc->duration)) / sumPosterior));
			    semiringCombo.set(combinedScores, i, subScores);
			    combinedScores->set(combinedPosteriorId, sumPosterior);
			    ++itArc; ++itCombinedArc;
			}
			for (; (itArc != endArc) && ((itCombinedArc == combinedSlot.end()) || (itArc->label < itCombinedArc->label)); ++itArc, ++itCombinedArc) {
			    const ConfusionNetwork::Arc &arc = *itArc;
			    ScoresRef combinedScores = combinedSemiring.clone(subOne);
			    semiringCombo.set(combinedScores, i, arc.scores);
			    combinedScores->set(combinedPosteriorId, hypWeight * arc.scores->get(posteriorId));
			    itCombinedArc = combinedSlot.insert(itCombinedArc, ConfusionNetwork::Arc(
						   arc.label,
						   combinedScores,
						   arc.begin, arc.duration));
			}
		    }

		    //dbg("SUB " << combinedCn->alphabet->symbol(itSlot->front().label) << " -> " << combinedCn->alphabet->symbol(itCombinedSlot->front().label)
		    //<< " " << hypWeight * itSlot->front().scores->get(posteriorId) << " -> " << itCombinedSlot->front().scores->get(combinedPosteriorId));

		    ++itSlot; ++itCombinedSlot;
		}
	    }
	    verify((itCombinedSlot == combinedCn->end()) && (itSlot == cn->end()));
	}

	ConfusionNetworkCombination(WeightedPosteriorCombinationHelperRef weightedCombo, WeightedCostRef weightedCostFcn, u32 minBeamWidth) :
	    Precursor(weightedCostFcn, minBeamWidth),
	    weightedCombo_(weightedCombo),
	    weightedCostFcn_(weightedCostFcn) {}

    public:
	void dump(std::ostream &os) const {
	    os << "Confusion Network Combination:" << std::endl;
	    weightedCombo_->dump(os);
	    Precursor::dump(os);
	}

	bool verifyCn(const ConfusionNetwork &cn, Score norm) const {
	    for (u32 i = 0; i < cn.size(); ++i) {
		const ConfusionNetwork::Slot &pdf = cn[i];
		verify(!pdf.empty());
		Score sum = pdf.front().scores->get(cn.normalizedProperties->posteriorId);
		for (ConfusionNetwork::Slot::const_iterator itPdf = pdf.begin() + 1; itPdf != pdf.end(); ++itPdf) {
		    verify((itPdf - 1)->label <= itPdf->label);
		    sum += itPdf->scores->get(cn.normalizedProperties->posteriorId);
		}
		verify((norm - 0.01 < sum) && (sum < norm + 0.01));
	    }
	    return true;
	}

	ConstConfusionNetworkRef combine(const ConstConfusionNetworkRefList &cns, ConfusionNetworkAlignmentPtrList *alignments = 0) const {
	    verify(!cns.empty());

	    if (alignments)
		if (alignments->size() < cns.size() - 1) alignments->resize(cns.size() - 1, 0);

	    weightedCombo_->update(cns, true);
	    ConfusionNetwork *combinedCnPtr = new ConfusionNetwork;
	    if (!cns.front()->isNormalized())
		Core::Application::us()->criticalError(
		    "Confusion network is not normalized. Confusion network combination requires normalized CNs.");
	    // add proportional posterior mass
	    init(combinedCnPtr, cns.front());

	    verify(verifyCn(*combinedCnPtr, weightedCombo_->weights().front()));

	    ScoreId combinedPosteriorId = combinedCnPtr->normalizedProperties->posteriorId;
	    sumWeights_.resize(cns.size());
	    sumWeights_.front() = weightedCombo_->weights().front();
	    for (u32 i = 1; i < cns.size(); ++i) {
		ConstConfusionNetworkRef cn = cns[i];
		if (!cn->isNormalized())
		    Core::Application::us()->criticalError(
			"Confusion network is not normalized. Confusion network combination requires normalized CNs.");
		Score weight = weightedCombo_->weights()[i];
		Score sumWeight = sumWeights_[i] = sumWeights_[i - 1] + weight;
		// set weights such the two aligned CNs make up a slot-wise posterior distribution,
		// i.e. alignment score is normalized
		weightedCostFcn_->setPosteriorIds(combinedPosteriorId, cn->normalizedProperties->posteriorId);
		weightedCostFcn_->setNorms(sumWeights_[i - 1], 1.0);
		weightedCostFcn_->setWeights(sumWeights_[i - 1] / sumWeight, weight / sumWeight);

		/*
		  Looks not only a little hackish: it is!
		*/
		const ConfusionNetworkAlignment *alignment = this;
		if (alignments) {
		    if ((*alignments)[i - 1]) {

			dbg("CNC: transfer alignment");

			alignment = (*alignments)[i - 1];
		    } else {
			align(*combinedCnPtr, *cn);
			(*alignments)[i - 1] = new ConfusionNetworkAlignment(*this);
		    }
		} else
		    align(*combinedCnPtr, *cn);

		// add proportional posterior mass
		add(*alignment, combinedCnPtr, cn, i);

		verify(verifyCn(*combinedCnPtr, sumWeight));

	    }
	    return ConstConfusionNetworkRef(combinedCnPtr);
	}

	static ConstConfusionNetworkCombinationRef create(
	    WeightedPosteriorCombinationHelperRef weightedCombo,
	    WeightedCostRef weightedCostFcn,
	    u32 minBeamWidth) {
	    verify(weightedCombo);
	    verify(weightedCostFcn);
	    return ConstConfusionNetworkCombinationRef(new ConfusionNetworkCombination(weightedCombo, weightedCostFcn, minBeamWidth));
	}
    };
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class ConfusionNetworkCombinationNode : public Node {
    public:
	typedef enum {
	    ExpectedLoss,
	    ExpectedError
	} CncCostFcn;
	static const Core::Choice choiceCncCostFcn;
	static const Core::ParameterChoice paramCncCostFcn;

    private:
	WeightedPosteriorCombinationHelperRef weightedCombo_;
	ConstConfusionNetworkCombinationRef cnCombo_;
	ConstConfusionNetworkRef cn_;

    private:
	ConstConfusionNetworkRef getCn() {
	    if (!cn_) {
		const WeightedPosteriorCombinationHelper::IdList &indices = weightedCombo_->indices();
		ConstConfusionNetworkRefList cns(indices.size());
		for (u32 i = 0; i < cns.size(); ++i) {
		    cns[i] = requestCn(indices[i]);
		    if (!cns[i]) {
			log("Empty CN from port %d; skip combination.", indices[i]);
			cns.clear();
			break;
		    }
		}
		if (!cns.empty()) {
		    {
			Core::Component::Message msg = log("Combine CNs:\n");
			for (u32 i = 0; i < cns.size(); ++i)
			    msg << "  " << (i + 1) << ". #slots=" << cns[i]->size() << "\n";
		    }
		    cn_ = cnCombo_->combine(cns);
		}
	    }
	    return cn_;
	}

    public:
	ConfusionNetworkCombinationNode(const std::string &name, const Core::Configuration &config) :
	    Node(name, config) {}
	virtual ~ConfusionNetworkCombinationNode() {}

	virtual void init(const std::vector<std::string> &arguments) {
	    u32 n = 0;
	    for (; connected(n); ++n);
	    if (n == 0)
		criticalError("At least one incoming posterior CN at port 0 required.");
	    ScoreList weights(n);
	    KeyList keys(n);
	    for (u32 i = 0; i < n; ++i) {
		const Core::Configuration cnConfig(config, Core::form("cn-%d", i));
		weights[i] = WeightedPosteriorCombinationHelper::paramWeight(cnConfig);
		keys[i] = WeightedPosteriorCombinationHelper::paramPosteriorKey(cnConfig, "");
	    }
	    weightedCombo_ = WeightedPosteriorCombinationHelper::create(
		weights, keys,
		SemiringCombinationHelper::getType(SemiringCombinationHelper::paramType(select("score-combination"))),
		WeightedPosteriorCombinationHelper::paramPosteriorKey(config, "confidence"));
	    ConfusionNetworkCombination::WeightedCostRef costFcn;
	    Core::Choice::Value costChoice = paramCncCostFcn(config);
	    if (costChoice == Core::Choice::IllegalValue)
		criticalError("Unknwon cost fucntion.");
	    switch (CncCostFcn(costChoice)) {
	    case ExpectedLoss:
		costFcn = ConfusionNetworkCombination::ExpectedLoss::create();
		break;
	    case ExpectedError:
		costFcn = ConfusionNetworkCombination::ExpectedError::create();
		break;
	    default:
		defect();
	    }
	    cnCombo_ = ConfusionNetworkCombination::create(weightedCombo_, costFcn, paramBeamWidth(config));
	    cnCombo_->dump(log());
	    if (weightedCombo_->indices().size() == 1)
		warning("CN combination has only a single CN input.");
	}

	virtual void finalize() {}

	virtual ConstLatticeRef sendLattice(Port to) {
	    switch (to) {
	    case 0:
		return decodeCn(getCn());
	    case 2:
		return cn2lattice(getCn());
	    default:
		defect();
		return ConstLatticeRef();
	    }
	}

	virtual ConstConfusionNetworkRef sendCn(Port to) {
	    switch (to) {
	    case 1:
		return getCn();
	    default:
		defect();
		return ConstConfusionNetworkRef();
	    }
	}

	virtual void sync() {
	    cn_.reset();
	}
    };
    const Core::Choice ConfusionNetworkCombinationNode::choiceCncCostFcn(
	"expected-loss",  ConfusionNetworkCombinationNode::ExpectedLoss,
	"expected-error", ConfusionNetworkCombinationNode::ExpectedError,
	Core::Choice::endMark());
    const Core::ParameterChoice ConfusionNetworkCombinationNode::paramCncCostFcn(
	"cost",
	&ConfusionNetworkCombinationNode::choiceCncCostFcn,
	"cost function",
	ConfusionNetworkCombinationNode::ExpectedError);
    NodeRef createConfusionNetworkCombinationNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new ConfusionNetworkCombinationNode(name, config));
    }
    // -------------------------------------------------------------------------



    // -------------------------------------------------------------------------
    class RoverCombination;
    typedef Core::Ref<const RoverCombination> ConstRoverCombinationRef;

    class RoverCombination : public ConfusionNetworkAlignment, public Core::ReferenceCounted {
	typedef ConfusionNetworkAlignment Precursor;
    public:
	/**
	   NIST/SCLITE Dynamic Programming string alignment:

	   The DP string alignment algorithm performs a global minimization of a
	   Levenshtein distance function which weights the cost of correct words,
	   insertions, deletions and substitutions as 0, 3, 3 and 4 respectively.
	   The computational complexity of DP is 0(NN).

	   When evaluating the output of speech recognition systems, the
	   precision of generated statistics is directly correlated to the
	   reference text accuracy.  But uttered words can be coarticulated or
	   mumbled to where they have ambiguous transcriptions, (e.i., "what are"
	   or "what're").  In order to more accurately represent ambiguous
	   transcriptions, and not penalize recognition systems, the ARPA
	   community agreed upon a format for specifying alternative reference
	   transcriptions.  The convention, when used on the case above, allows
	   the recognition system to output either transcripts, "what are" or
	   "what're", and still be correct.

	   The case above handles ambiguously spoken words which are loud enough
	   for the transcriber to think something should be recognized.  For
	   mumbled or quietly spoken words, the ARPA community agreed to neither
	   penalize systems which correctly recognized the word, nor penalize
	   systems which did not.  To accommodate this, a NULL word, "@", can be
	   added to an alternative reference transcript.  For example, "the" is
	   often spoken quickly with little acoustic evidence.  If "the" and "@"
	   are alternates, the recognition system will be given credit for
	   outputting "the" but not penalized if it does not.

	   The presence of alternate transcriptions represents added
	   computational complexity to the DP algorithm.  Rather than align all
	   alternate reference texts to the hypothesis text, then choose the
	   lowest error rate alignment, this implementation of DP aligns two word
	   networks, thus reducing the computational complexity from 2^(ref_alts +
	   hyp_alts) * O(N_ref * N_hyp) to O((N_ref+ref_alts) *
	   (N_hyp+hyp_alts)).

	   For a detailed explanation of DP alignment, see TIME WARPS, STRING
	   EDITS, AND MACROMOLECULES: THE THEORY AND PRACTICE OF SEQUENCE
	   COMPARISON, by Sankoff and Kruskal, ISBN 0-201-07809-0.

	   As noted above, DP alignment minimizes a distance function that is applied
	   to word pairs.  In addition to the "word" alignments which uses
	   a distance function defined by static weights, the sclite DP alignment module can
	   use two other distance functions.  The first, called Time-Mediated alignment
	   and the second called Word-Weight-Mediated alignment.


	   Time-Mediated Alignment

	   Time-Mediated alignment is a variation of DP alignment where
	   word-to-word distances are based on the time of occurence for
	   individual words.  Time-mediated alignments are performed when the '-T'
	   option is exercised and the input formats for both the reference and hypothesis
	   files are in "ctm" format.

	   Time-mediated alignments are computed by replacing the
	   standard word-to-word distance weights of 0, 3, 3, and 4 with measures
	   based on beginning and ending word times.  The formulas for
	   time-mediated word-to-word distances are:

	   D(correct) = | T1(ref) - T1(hyp) | + | T2(ref) - T2(hyp) |
	   D(insertion)  = T2(hyp) - T1(hyp)
	   D(deletion)  = T2(ref) - T1(ref)
	   D(substitution) = | T1(ref) - T1(hyp) | + | T2(ref) - T2(hyp) | + 0.001
	   Distance for an Insertion or Deletion of the NULL Token '@' = 0.001

	   Where,
	   T1(x) is the beginning time mark of word x
	   T2(x) is the ending time mark of word x

	**/
	class NistScliteCost;
	typedef Core::Ref<NistScliteCost> NistScliteCostRef;

	class NistScliteCost : public ConfusionNetworkAlignment::Cost {
	    typedef ConfusionNetworkAlignment::Cost Precursor;
	protected:
	    std::string null;
	    Fsa::LabelId nullLabel;
	    Fsa::ConstAlphabetRef alphabet;
	public:
	    NistScliteCost(std::string null = "@") :
		Precursor(), null(null), nullLabel(Fsa::Epsilon) {}

	    void setAlphabet(Fsa::ConstAlphabetRef alphabet) {
		this->alphabet = alphabet;
		nullLabel = (null.empty()) ? Fsa::Epsilon : alphabet->index(null);
	    }
	};

	/**
	   Description of the cost structure:
	   0      ->  Correct
	   0.001  ->  Insertion, deletion of '@' (a NULL)
	   1      ->  Substitution of '@' for '@'
	   3      ->  Deletion or insertion
	   4      ->  Substitution

	   see sctk-1.3/src/word.c
	   float wwd_WORD(void *p1, void *p2, int (*cmp)(void *p1, void *p2));
	**/
	class NistScliteWordCost : public NistScliteCost {
	    typedef NistScliteCost Precursor;
	public:
	    NistScliteWordCost(std::string null) : Precursor(null) {}

	    virtual std::string describe() const { return "NIST/SCLITE-word-distance"; }

	    virtual Score insCost(u32 hypId) const {
		const ConfusionNetwork::Slot &hyp = hypSlot(hypId);
		verify(!hyp.empty());
		Fsa::LabelId hypLabel = hyp.front().label;
		return ((hypLabel == Fsa::Epsilon) || (hypLabel == nullLabel)) ? 0.001 : 3.0;
	    }

	    virtual Score delCost(u32 refId) const {
		const ConfusionNetwork::Slot &ref = refSlot(refId);
		verify(!ref.empty());
		Score cost = Infinity, tmpCost = 0.0;
		for (ConfusionNetwork::Slot::const_iterator itRef = ref.begin(), endRef = ref.end(); itRef != endRef; ++itRef) {
		    Fsa::LabelId refLabel = itRef->label;
		    tmpCost = ((refLabel == Fsa::Epsilon) || (refLabel == nullLabel)) ? 0.001 : 3.0;
		    if (tmpCost < cost) cost = tmpCost;
		}
		return cost;
	    }

	    virtual Score subCost(u32 refId, u32 hypId) const {
		const ConfusionNetwork::Slot &ref = refSlot(refId), &hyp = hypSlot(hypId);
		verify(!ref.empty() && (hyp.size() == 1));
		Fsa::LabelId hypLabel = hyp.front().label;
		Score cost = Infinity;
		if (hypLabel == Fsa::Epsilon) {
		    cost = delCost(refId) - 0.001;
		} else {
		    Score tmpCost = 0.0;
		    for (ConfusionNetwork::Slot::const_iterator itRef = ref.begin(), endRef = ref.end(); itRef != endRef; ++itRef) {
			Fsa::LabelId refLabel = itRef->label;
			if (refLabel != Fsa::Epsilon) {
			    if (refLabel == hypLabel)
				if (refLabel != nullLabel) { cost = 0.0; break; } else tmpCost = 1.0;
			    else
				tmpCost = 4.0;
			    if (tmpCost < cost) cost = tmpCost;
			}
		    }
		    if (cost == Infinity)
			cost = insCost(hypId) - 0.001;
		}
		return cost;
	    }

	    static NistScliteCostRef create(std::string null = "@") {
		return NistScliteCostRef(new NistScliteWordCost(null));
	    }
	};

	/**
	   D(correct) = | T1(ref) - T1(hyp) | + | T2(ref) - T2(hyp) |
	   D(insertion)  = T2(hyp) - T1(hyp)
	   D(deletion)  = T2(ref) - T1(ref)
	   D(substitution) = | T1(ref) - T1(hyp) | + | T2(ref) - T2(hyp) | + 0.001
	   Distance for an Insertion or Deletion of the NULL Token '@' = 0.001

	   see sctk-1.3/src/word.c
	   float wwd_time_WORD(void *p1, void *p2, int (*cmp)(void *p1, void *p2));
	**/
	/**
	   Interesting cases:
	   1) original ROVER: cost for sub or del/ins is equal
	   the 2945.83-2945.91
	   *EPS*
	   --------------
	   a 2945.85-2945.89
	**/
	class NistScliteTimeMediatedCost : public NistScliteCost {
	    typedef NistScliteCost Precursor;
	public:
	    NistScliteTimeMediatedCost(std::string null) : Precursor(null) {}

	    virtual std::string describe() const { return "NIST/SCLITE-mediated-time-distance"; }

	    virtual Score insCost(u32 hypId) const {
		const ConfusionNetwork::Slot &hyp = hypSlot(hypId);
		verify(!hyp.empty());
		const ConfusionNetwork::Arc &hypArc = hyp.front();
		Score cost = (hypArc.label == Fsa::Epsilon) ? 0.002 : Score(hypArc.duration) / 100.0 + 0.003;

		// dbg("INS: -:" << alphabet->symbol(hypArc.label) << " " << cost);

		return cost;
	    }

	    virtual Score delCost(u32 refId) const {
		const ConfusionNetwork::Slot &ref = refSlot(refId);
		verify(!ref.empty());
		Score cost = Infinity, tmpCost = 0.0;
		Fsa::LabelId label = Fsa::InvalidLabelId;
		for (ConfusionNetwork::Slot::const_iterator itRef = ref.begin(), endRef = ref.end(); itRef != endRef; ++itRef) {
		    const ConfusionNetwork::Arc &refArc = *itRef;
		    tmpCost = (refArc.label == Fsa::Epsilon) ?
			0.002 : Score(refArc.duration) / 100.0 + 0.003;
		    if (tmpCost < cost) { label = refArc.label; cost = tmpCost; }
		}

		// dbg("DEL: " << alphabet->symbol(label)  << ":- " << cost);

		return cost;
	    }

	    virtual Score subCost(u32 refId, u32 hypId) const {
		const ConfusionNetwork::Slot &ref = refSlot(refId), &hyp = hypSlot(hypId);
		verify(!ref.empty() && !hyp.empty());
		const ConfusionNetwork::Arc &hypArc = hyp.front();
		Score cost = Infinity;
		if (hypArc.label == Fsa::Epsilon) {
		    cost = delCost(refId) - 0.001;
		} else {
		    Score tmpCost = 0.0;
		    Fsa::LabelId label = Fsa::InvalidLabelId;
		    for (ConfusionNetwork::Slot::const_iterator itRef = ref.begin(), endRef = ref.end(); itRef != endRef; ++itRef) {
			const ConfusionNetwork::Arc &refArc = *itRef;
			if (refArc.label != Fsa::Epsilon) {
			    Time
				refBegin = refArc.begin, refEnd = refArc.begin + refArc.duration,
				hypBegin = hypArc.begin, hypEnd = hypArc.begin + hypArc.duration;
			    tmpCost = Score(
				((refBegin < hypBegin) ? hypBegin - refBegin : refBegin - hypBegin) +
				((refEnd < hypEnd)     ? hypEnd - refEnd     : refEnd - hypEnd)) / 100.0;
			    verify_(tmpCost >= 0.0);
			    if (refArc.label != hypArc.label)
				tmpCost += 0.001;
			    if (tmpCost < cost) { label = refArc.label; cost = tmpCost; }
			}
		    }
		    if (cost == Infinity)
			cost = insCost(hypId) - 0.001;
		    // else  dbg("SUB: " << alphabet->symbol(label)  << ":" << alphabet->symbol(hypArc.label) << " " << cost);
		}
		return cost;
	    }

	    /*
	      virtual Score subCost(u32 refId, u32 hypId) const {
	      const ConfusionNetwork::Slot &ref = refSlot(refId), &hyp = hypSlot(hypId);
	      verify(!ref.empty() && (hyp.size() == 1));
	      if (ref.size() == 1) verify(ref.front().label != Fsa::Epsilon);
	      const ConfusionNetwork::Arc &hypArc = hyp.front();
	      verify(hypArc.label != Fsa::Epsilon);
	      Score cost = Infinity, tmpCost = 0.0;
	      Fsa::LabelId label = Fsa::InvalidLabelId;
	      for (ConfusionNetwork::Slot::const_iterator itRef = ref.begin(), endRef = ref.end(); itRef != endRef; ++itRef) {
	      const ConfusionNetwork::Arc &refArc = *itRef;
	      if (refArc.label != Fsa::Epsilon) {
	      Time
	      refBegin = refArc.begin, refEnd = refArc.begin + refArc.duration,
	      hypBegin = hypArc.begin, hypEnd = hypArc.begin + hypArc.duration;
	      tmpCost = Score(
	      ((refBegin < hypBegin) ? hypBegin - refBegin : refBegin - hypBegin) +
	      ((refEnd < hypEnd)     ? hypEnd - refEnd     : refEnd - hypEnd)) / 100.0;
	      verify_(tmpCost >= 0.0);
	      if (refArc.label != hypArc.label)
	      tmpCost += 0.001;
	      if (tmpCost < cost) { label = refArc.label; cost = tmpCost; }
	      }
	      }
	      verify(cost != Infinity);

	      // dbg("SUB: " << alphabet->symbol(label)  << ":" << alphabet->symbol(hypArc.label) << " " << cost);

	      return cost;
	      }
	    */

	    static NistScliteCostRef create(std::string null = "@") {
		return NistScliteCostRef(new NistScliteTimeMediatedCost(null));
	    }
	};

    private:
	WeightedPosteriorCombinationHelperRef weightedCombo_;
	NistScliteCostRef scliteCostFcn_;
	Score nullConfidence_;
	Score alpha_;
	bool removeEps_;

	mutable std::vector<ScoreList> confidences_;

    private:
	/**
	 * Build CN by merging the aligned slots
	 **/
	void init(ConfusionNetwork *combinedCn, std::vector<ScoreList> &confidences, ConstConfusionNetworkRef cn) const {
	    const SemiringCombinationHelper &semiringCombo = *weightedCombo_->semiringCombination();
	    const Semiring &combinedSemiring = *semiringCombo.semiring();
	    ScoresRef insOne = combinedSemiring.one();
	    combinedCn->alphabet = cn->alphabet;
	    combinedCn->semiring = semiringCombo.semiring();
	    combinedCn->nBestAlignmentProperties = ConfusionNetwork::ConstNBestAlignmentPropertiesRef(
		new ConfusionNetwork::NBestAlignmentProperties(weightedCombo_->size()));
	    combinedCn->resize(cn->size(), ConfusionNetwork::Slot(1));
	    ConfusionNetwork::iterator itCombinedSlot = combinedCn->begin();
	    for (ConfusionNetwork::const_iterator itSlot = cn->begin(), endSlot = cn->end(); itSlot != endSlot; ++itSlot, ++itCombinedSlot) {
		const ConfusionNetwork::Arc &arc = itSlot->front();
		ConfusionNetwork::Arc &combinedArc = itCombinedSlot->front();
		ScoresRef combinedScores = combinedSemiring.clone(insOne);
		semiringCombo.set(combinedScores, 0, arc.scores);
		combinedArc.label = arc.label;
		combinedArc.scores = combinedScores;
		combinedArc.begin = arc.begin;
		combinedArc.duration = arc.duration;

		// dbg
		verify(itCombinedSlot->size() == 1);

	    }
	    if (alpha_ != 1.0) {
		ScoreId posteriorId = weightedCombo_->posteriorIds().front();
		confidences.resize(cn->size(), ScoreList(1));
		std::vector<ScoreList>::iterator itConfidences = confidences.begin();
		for (ConfusionNetwork::const_iterator itSlot = cn->begin(), endSlot = cn->end(); itSlot != endSlot; ++itSlot, ++itConfidences) {
		    const ConfusionNetwork::Arc &arc = itSlot->front();
		    itConfidences->front() = arc.scores->get(posteriorId);
		}
	    }
	}

	void add(const ConfusionNetworkAlignment &alignment, ConfusionNetwork *combinedCn, std::vector<ScoreList> &confidences, ConstConfusionNetworkRef cn, u32 i) const {
	    const SemiringCombinationHelper &semiringCombo = *weightedCombo_->semiringCombination();
	    const Semiring &combinedSemiring = *semiringCombo.semiring();
	    ScoresRef subOne = combinedSemiring.one();
	    ScoresRef delOne = combinedSemiring.one();
	    ConfusionNetwork::Slot insSlot(
		i + 1,
		ConfusionNetwork::Arc(Fsa::Epsilon, combinedSemiring.one()));
	    if (alpha_ != 1.0) {
		const ScoreIdList &posteriorIds = weightedCombo_->posteriorIds();
		for (u32 j = 0; j < i; ++j) {
		    ConfusionNetwork::Arc &arc = insSlot[j];
		    arc.scores = combinedSemiring.clone(arc.scores);
		    semiringCombo.set(arc.scores, j, posteriorIds[j], nullConfidence_);
		}
		delOne = combinedSemiring.clone(delOne);
		semiringCombo.set(delOne, i, posteriorIds[i], nullConfidence_);
	    }
	    ConfusionNetwork::const_iterator itSlot = cn->begin();
	    ConfusionNetwork::iterator itCombinedSlot = combinedCn->begin();
	    for (const_iterator itPair = alignment.begin(), endPair = alignment.end(); itPair != endPair; ++itPair) {
		if (itPair->first == Core::Type<u32>::max) {
		    // insertion
		    // insert slot, fill with eps arcs
		    const ConfusionNetwork::Arc &arc = itSlot->front();

		    //dbg("INS " << combinedCn->alphabet->symbol(itSlot->front().label) << " -> " << combinedCn->alphabet->symbol(Fsa::Epsilon));

		    {
			itCombinedSlot = combinedCn->insert(itCombinedSlot, insSlot);
			Time insTime = arc.begin + arc.duration / 2;
			for (ConfusionNetwork::Slot::iterator itArc = itCombinedSlot->begin(), endArc = itCombinedSlot->begin() + i;
			     itArc != endArc; ++itArc) {
			    itArc->scores = combinedSemiring.clone(itArc->scores);
			    itArc->begin = insTime;
			    itArc->duration = 0;
			}
		    }
		    ConfusionNetwork::Arc &combinedArc = itCombinedSlot->back();
		    ScoresRef combinedScores = combinedSemiring.clone(combinedArc.scores);
		    semiringCombo.set(combinedScores, i, arc.scores);
		    combinedArc.label = arc.label;
		    combinedArc.scores = combinedScores;
		    combinedArc.begin = arc.begin;
		    combinedArc.duration = arc.duration;

		    //dbg("INS" << weightedCombo_->semirings()[i]->describe(arc.scores, Fsa::HintShowDetails | HintUnscaled)
		    //<< " -> " << combinedCn->semiring->describe(itCombinedSlot->back().scores, Fsa::HintShowDetails | HintUnscaled));

		    ++itSlot; ++itCombinedSlot;
		} else if (itPair->second == Core::Type<u32>::max) {
		    // deletion
		    // add eps arc

		    //dbg("DEL " << combinedCn->alphabet->symbol(Fsa::Epsilon) << " -> " << combinedCn->alphabet->symbol(itCombinedSlot->front().label));

		    const ConfusionNetwork::Arc &primaryArc = itCombinedSlot->front();
		    itCombinedSlot->push_back(
			ConfusionNetwork::Arc(
			    Fsa::Epsilon,
			    combinedSemiring.clone(delOne),
			    primaryArc.begin + primaryArc.duration / 2, 0));

		    //dbg("DEL" << combinedCn->semiring->describe(itCombinedSlot->back().scores, Fsa::HintShowDetails | HintUnscaled));

		    ++itCombinedSlot;
		} else {
		    // substitution
		    // add arc

		    //dbg("SUB " << combinedCn->alphabet->symbol(itSlot->front().label) << " -> " << combinedCn->alphabet->symbol(itCombinedSlot->front().label));

		    const ConfusionNetwork::Arc &arc = itSlot->front();
		    ScoresRef combinedScores = combinedSemiring.clone(subOne);
		    semiringCombo.set(combinedScores, i, arc.scores);
		    itCombinedSlot->push_back(
			ConfusionNetwork::Arc(
			    arc.label,
			    combinedScores,
			    arc.begin, arc.duration));

		    //dbg("SUB" << weightedCombo_->semirings()[i]->describe(arc.scores, Fsa::HintShowDetails | HintUnscaled)
		    //<< " -> " << combinedCn->semiring->describe(itCombinedSlot->back().scores, Fsa::HintShowDetails | HintUnscaled));

		    ++itSlot; ++itCombinedSlot;
		}

		// dbg
		verify((itCombinedSlot - 1)->size() == i + 1);

	    }
	    verify((itCombinedSlot == combinedCn->end()) && (itSlot == cn->end()));
	    if (alpha_ != 1.0) {
		ScoreId posteriorId = weightedCombo_->posteriorIds()[i];
		ConfusionNetwork::const_iterator itSlot = cn->begin();
		std::vector<ScoreList>::iterator itConfidences = confidences.begin();
		for (const_iterator itPair = alignment.begin(), endPair = alignment.end(); itPair != endPair; ++itPair) {
		    if (itPair->first == Core::Type<u32>::max) {
			const ConfusionNetwork::Arc &arc = itSlot->front();
			{
			    itConfidences = confidences.insert(
				itConfidences,
				ScoreList(i, nullConfidence_));
			}
			itConfidences->push_back(arc.scores->get(posteriorId));
			++itSlot; ++itConfidences;
		    } else if (itPair->second == Core::Type<u32>::max) {
			itConfidences->push_back(nullConfidence_);
			++itConfidences;
		    } else {
			const ConfusionNetwork::Arc &arc = itSlot->front();
			itConfidences->push_back(arc.scores->get(posteriorId));
			++itSlot; ++itConfidences;
		    }
		}
		verify((itConfidences == confidences.end()) && (itSlot == cn->end()));
	    }
	}

	/*
	  score[i] = alpha * w[i] + (1 - alpha) * (w[i] * c[i] / [ sum_{j=0}^{n} w[j] * c[j] ])
	*/
	void calcScores(ConfusionNetwork *combinedCn, const std::vector<ScoreList> &confidences) const {
	    ScoreId posteriorId = weightedCombo_->semiringCombination()->combinationId(0);
	    if (alpha_ == 1.0) {
		ScoreList::const_iterator beginWeight = weightedCombo_->weights().begin(), itWeight;
		for (ConfusionNetwork::iterator itSlot = combinedCn->begin(), endSlot = combinedCn->end(); itSlot != endSlot; ++itSlot) {
		    itWeight = beginWeight;
		    for (ConfusionNetwork::Slot::iterator itArc = itSlot->begin(), endArc = itSlot->end(); itArc != endArc; ++itArc, ++itWeight)
			itArc->scores->set(posteriorId, *itWeight);
		}
	    } else {
		std::vector<ScoreList>::const_iterator itConfidences = confidences.begin();
		for (ConfusionNetwork::iterator itSlot = combinedCn->begin(), endSlot = combinedCn->end();
		     itSlot != endSlot; ++itSlot, ++itConfidences) {
		    Score norm = 0.0;
		    ScoreList::const_iterator beginWeight = weightedCombo_->weights().begin(), itWeight;
		    ScoreList::const_iterator beginConfidence = itConfidences->begin(), endConfidence = itConfidences->end(), itConfidence;
		    itWeight = beginWeight;
		    itConfidence = beginConfidence;
		    for (; itConfidence != endConfidence; ++itConfidence, ++itWeight)
			norm += *itWeight * *itConfidence;
		    norm = (1.0 - alpha_) / norm;
		    itWeight = beginWeight;
		    itConfidence = beginConfidence;
		    for (ConfusionNetwork::Slot::iterator itArc = itSlot->begin(), endArc = itSlot->end();
			 itArc != endArc; ++itArc, ++itConfidence, ++itWeight)
			itArc->scores->set(posteriorId, (alpha_ * *itWeight) + (norm * *itWeight * *itConfidence));

		    /*
		      {
		      Score sum = 0.0;
		      for (ConfusionNetwork::Slot::iterator itArc = itSlot->begin(), endArc = itSlot->end(); itArc != endArc; ++itArc) {
		      //dbg(combinedCn->alphabet->symbol(itArc->label) << " "
		      //<< combinedCn->semiring->describe(itArc->scores, Fsa::HintShowDetails | HintUnscaled));
		      sum += itArc->scores->get(posteriorId);
		      }
		      verify(Core::isAlmostEqualUlp(sum, Score(1.0), 100));
		      }
		    */

		}
		verify(itConfidences == confidences.end());
	    }
	}

	ConstConfusionNetworkRef _combine(const ConstConfusionNetworkRefList &cns, ConfusionNetworkAlignmentPtrList *alignments = 0) const {
	    verify(!cns.empty());

	    if (alignments)
		if (alignments->size() < cns.size() - 1) alignments->resize(cns.size() - 1, 0);

	    if (alpha_ != 1.0)
		weightedCombo_->update(cns, true);
	    else
		weightedCombo_->update(cns, false);
	    ConfusionNetwork *combinedCnPtr = new ConfusionNetwork;
	    ConstConfusionNetworkRef combinedCn = ConstConfusionNetworkRef(combinedCnPtr);
	    init(combinedCnPtr, confidences_, cns.front());
	    scliteCostFcn_->setAlphabet(combinedCnPtr->alphabet);
	    for (u32 i = 1; i < cns.size(); ++i) {
		ConstConfusionNetworkRef cn = cns[i];

		/*
		  Looks not only a little hackish: it is!
		*/
		const ConfusionNetworkAlignment *alignment = this;
		if (alignments) {
		    if ((*alignments)[i - 1]) {

			dbg("ROVER: transfer alignment");

			alignment = (*alignments)[i - 1];
		    } else {
			align(*combinedCnPtr, *cn);
			(*alignments)[i - 1] = new ConfusionNetworkAlignment(*this);
		    }
		} else
		    align(*combinedCnPtr, *cn);

		add(*alignment, combinedCnPtr, confidences_, cn, i);
	    }
	    calcScores(combinedCnPtr, confidences_);
	    confidences_.clear();
	    return combinedCn;
	}

	RoverCombination(
	    WeightedPosteriorCombinationHelperRef weightedCombo,
	    NistScliteCostRef scliteCostFcn,
	    Score nullConfidence,
	    Score alpha,
	    bool removeEps,
	    u32 minBeamWidth) :
	    Precursor(scliteCostFcn, minBeamWidth),
	    weightedCombo_(weightedCombo),
	    scliteCostFcn_(scliteCostFcn),
	    nullConfidence_(nullConfidence), alpha_(alpha), removeEps_(removeEps) {}

    public:
	void dump(std::ostream &os) const {
	    os << "Rover Combination:" << std::endl;
	    os << "null-confidence=" << nullConfidence_ << std::endl;
	    os << "alpha=" << alpha_ << std::endl;
	    os << "remove epsilons=" << (removeEps_ ? "true" : "false") << std::endl;
	    weightedCombo_->dump(os);
	    Precursor::dump(os);
	}

	ConstConfusionNetworkRef combine(const ConstConfusionNetworkRefList &cns, ConfusionNetworkAlignmentPtrList *alignments = 0) const {
	    ConstConfusionNetworkRefList linearCns(cns.size(), ConstConfusionNetworkRef());
	    for (u32 i = 0; i < cns.size(); ++i) {
		const ConfusionNetwork &cn = *cns[i];
		verify(cn.isNormalized());
		ScoreId posteriorId = cn.normalizedProperties->posteriorId;
		ConfusionNetwork *linearCn = new ConfusionNetwork(cn.size(), ConfusionNetwork::Slot(1, ConfusionNetwork::Arc()));
		linearCn->alphabet = cn.alphabet;
		linearCn->semiring = cn.semiring;
		linearCn->normalizedProperties = cn.normalizedProperties;
		ConfusionNetwork::iterator itLinearSlot = linearCn->begin();
		for (ConfusionNetwork::const_iterator itSlot = cn.begin(), endSlot = cn.end(); itSlot != endSlot; ++itSlot, ++itLinearSlot) {
		    Score maxPosterior = 0.0;
		    ConfusionNetwork::Slot::const_iterator itBestPdf = itSlot->end();
		    for (ConfusionNetwork::Slot::const_iterator itPdf = itSlot->begin(), endPdf = itSlot->end(); itPdf != endPdf; ++itPdf) {
			Score posterior = itPdf->scores->get(posteriorId);
			if (posterior > maxPosterior) {
			    maxPosterior = posterior;
			    itBestPdf = itPdf;
			}
		    }
		    verify(itBestPdf != itSlot->end());
		    itLinearSlot->front() = *itBestPdf;
		}
		linearCns[i] = ConstConfusionNetworkRef(linearCn);
	    }
	    return _combine(linearCns, alignments);
	}

	// Todo: std::pair<ConstConfusionNetworkRef, ConstLatticeRef> combine(const ConstLatticeRefList &lats) const
	// -> output is CN(with normalized posterior scores) + best(with averaged [unnormalized] confidences)
	ConstConfusionNetworkRef combine(const ConstLatticeRefList &lats, ConfusionNetworkAlignmentPtrList *alignments = 0) const {
	    ConstConfusionNetworkRefList cns(lats.size(), ConstConfusionNetworkRef());
	    for (u32 i = 0; i < lats.size(); ++i) {
		ConstLatticeRef l = lats[i];
		if (!l->hasProperty(Fsa::PropertyLinear))
		    l = best(l);
		cns[i] = sausageLattice2cn(l, !removeEps_);
	    }
	    return _combine(cns, alignments);
	}

	static ConstRoverCombinationRef create(
	    WeightedPosteriorCombinationHelperRef weightedCombo,
	    NistScliteCostRef scliteCostFcn,
	    Score nullConfidence, Score alpha, bool removeEps,
	    u32 minBeamWidth) {
	    verify(weightedCombo);
	    verify(scliteCostFcn);
	    // verify(0.0 <= nullConfidence);
	    if ((nullConfidence < 0.0) && (1.0 < nullConfidence))
		Core::Application::us()->warning(
		    "Null-confidence %f is not in [0, 1]", nullConfidence);
	    // verify((0.0 <= alpha) && (alpha <= 1.0));
	    if ((alpha < 0.0) && (1.0 < alpha))
		Core::Application::us()->warning(
		    "Interpolation alpha %f is not in [0, 1]", alpha);
	    return ConstRoverCombinationRef(new RoverCombination(weightedCombo, scliteCostFcn, nullConfidence, alpha, removeEps, minBeamWidth));
	}
    };
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class RoverCombinationNode : public Node {
    public:
    public:
	typedef enum {
	    NistScliteWordCost,
	    NistScliteTimeMediatedCost
	} RoverCostFcn;
	static const Core::Choice choiceRoverCostFcn;
	static const Core::ParameterChoice paramRoverCostFcn;
	static const Core::ParameterString paramNullWord;
	static const Core::ParameterFloat paramNullConfidence;
	static const Core::ParameterFloat paramAlpha;
	static const Core::ParameterBool paramRemoveEpsilons;

    private:
	WeightedPosteriorCombinationHelperRef weightedCombo_;
	ConstRoverCombinationRef roverCombo_;
	ConstConfusionNetworkRef nBestCn_;
	ConstConfusionNetworkRef cn_;

    private:
	ConstConfusionNetworkRef getNBestCn() {
	    if (!nBestCn_) {
		const WeightedPosteriorCombinationHelper::IdList &indices = weightedCombo_->indices();
		ConstLatticeRefList lats(indices.size());
		for (u32 i = 0; i < lats.size(); ++i) {
		    lats[i] = requestLattice(indices[i]);
		    if (!lats[i]) {
			log("Empty lattice from port %d; skip combination.", indices[i]);
			lats.clear();
			break;
		    }
		}
		if (!lats.empty()) {
		    {
			Core::Component::Message msg = log("Combine 1-bests:\n");
			for (u32 i = 0; i < lats.size(); ++i)
			    msg << "  " << (i + 1) << ". " << lats[i]->describe() << "\n";
		    }
		    nBestCn_ = roverCombo_->combine(lats);
		}
	    }
	    return nBestCn_;
	}

	ConstConfusionNetworkRef getCn() {
	    if (!cn_) {
		ConstConfusionNetworkRef nBestCn = getNBestCn(); // Attention: getNBestCn() updates weightedCombo_
		cn_ = normalizeCn(nBestCn, weightedCombo_->semiringCombination()->combinationId(0));
	    }
	    return cn_;
	}

    public:
	RoverCombinationNode(const std::string &name, const Core::Configuration &config) :
	    Node(name, config) {}
	virtual ~RoverCombinationNode() {}

	virtual void init(const std::vector<std::string> &arguments) {
	    u32 n = 0;
	    for (n = 0; connected(n); ++n);
	    if (n == 0)
		criticalError("At least one incoming lattice at port 0 required.");
	    ScoreList weights(n);
	    KeyList keys(n);
	    for (u32 i = 0; i < n; ++i) {
		const Core::Configuration latConfig(config, Core::form("lattice-%d", i));
		weights[i] = WeightedPosteriorCombinationHelper::paramWeight(latConfig);
		keys[i] = WeightedPosteriorCombinationHelper::paramConfidenceKey(latConfig, "");
	    }
	    weightedCombo_ = WeightedPosteriorCombinationHelper::create(
		weights, keys,
		SemiringCombinationHelper::getType(SemiringCombinationHelper::paramType(select("score-combination"))),
		WeightedPosteriorCombinationHelper::paramPosteriorKey(config, "confidence"));
	    RoverCombination::NistScliteCostRef costFcn;
	    Core::Choice::Value costChoice = paramRoverCostFcn(config);
	    if (costChoice == Core::Choice::IllegalValue)
		criticalError("Unknwon cost fucntion.");
	    switch (RoverCostFcn(costChoice)) {
	    case NistScliteWordCost:
		costFcn = RoverCombination::NistScliteWordCost::create(paramNullWord(config));
		break;
	    case NistScliteTimeMediatedCost:
		costFcn = RoverCombination::NistScliteTimeMediatedCost::create(paramNullWord(config));
		break;
	    default:
		defect();
	    }
	    roverCombo_ = RoverCombination::create(
		weightedCombo_, costFcn, paramNullConfidence(config), paramAlpha(config), paramRemoveEpsilons(config), paramBeamWidth(config));
	    roverCombo_->dump(log());
	    if (weightedCombo_->indices().size() == 1)
		warning("ROVER combination has only a single CN input.");
	}

	virtual void finalize() {}

	virtual ConstLatticeRef sendLattice(Port to) {
	    switch (to) {
	    case 0:
		return decodeCn(getCn());
	    case 2:
		return cn2lattice(getCn());
	    case 4:
		return cn2lattice(getNBestCn());
	    default:
		defect();
		return ConstLatticeRef();
	    }
	}

	virtual ConstConfusionNetworkRef sendCn(Port to) {
	    switch (to) {
	    case 1:
		return getCn();
	    case 3:
		return getNBestCn();
	    default:
		defect();
		return ConstConfusionNetworkRef();
	    }
	}

	virtual void sync() {
	    nBestCn_.reset();
	    cn_.reset();
	}
    };
    const Core::Choice RoverCombinationNode::choiceRoverCostFcn(
	"sclite-word-cost",          RoverCombinationNode::NistScliteWordCost,
	"sclite-time-mediated-cost", RoverCombinationNode::NistScliteTimeMediatedCost,
	Core::Choice::endMark());
    const Core::ParameterChoice RoverCombinationNode::paramRoverCostFcn(
	"cost",
	&RoverCombinationNode::choiceRoverCostFcn,
	"cost function",
	RoverCombinationNode::NistScliteTimeMediatedCost);
    const Core::ParameterString RoverCombinationNode::paramNullWord(
	"null-word",
	"null word",
	"@");
    const Core::ParameterFloat RoverCombinationNode::paramNullConfidence(
	"null-confidence",
	"null confidence",
	0.7);
    const Core::ParameterFloat RoverCombinationNode::paramAlpha(
	"alpha",
	"alpha",
	0.0);
    const Core::ParameterBool RoverCombinationNode::paramRemoveEpsilons(
	"remove-eps",
	"remove epsilon arcs",
	false);

    NodeRef createRoverCombinationNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new RoverCombinationNode(name, config));
    }
    // -------------------------------------------------------------------------



    // -------------------------------------------------------------------------
    class OracleAlignment;
    typedef Core::Ref<const OracleAlignment> ConstOracleAlignmentRef;

    class OracleAlignment : public ConfusionNetworkAlignment, public Core::ReferenceCounted {
	typedef ConfusionNetworkAlignment Precursor;
    private:
	static ConstSemiringRef emptySemiring_;
    public:
	static ConstSemiringRef emptySemiring() {
	    if (!emptySemiring_)
		emptySemiring_ = Semiring::create(Fsa::SemiringTypeTropical, 0);
	    return emptySemiring_;
	}

    public:
	class PosteriorCost;
	typedef Core::Ref<PosteriorCost> PosteriorCostRef;

	class PosteriorCost : public ConfusionNetworkAlignment::Cost {
	protected:
	    ScoreId posteriorId;
	public:
	    PosteriorCost() : posteriorId(Semiring::InvalidId) {}
	    void setPosteriorId(ScoreId posteriorId) {
		verify(posteriorId != Semiring::InvalidId);
		this->posteriorId = posteriorId;
	    }
	    virtual bool requiresPosteriorId() const { return false; }
	};

	/*
	  0, if reference word is in slot,
	  1, else
	  slightly prefer correct alignments
	*/
	class OracleCost : public PosteriorCost {
	public:
	    virtual std::string describe() const { return "oracle-cost"; }

	    virtual Score insCost(u32 hypId) const {
		const ConfusionNetwork::Slot &hyp = hypSlot(hypId);
		for (ConfusionNetwork::Slot::const_iterator itHyp = hyp.begin(), endHyp = hyp.end(); itHyp != endHyp; ++itHyp)
		    if (itHyp->label == Fsa::Epsilon)
			return -0.001;
		return 1.0;
	    }

	    virtual Score delCost(u32 refId) const {
		const ConfusionNetwork::Slot &ref = refSlot(refId);
		verify(!ref.empty() && (ref.front().label != Fsa::Epsilon));
		return 1.0;
	    }

	    virtual Score subCost(u32 refId, u32 hypId) const {
		const ConfusionNetwork::Slot &ref = refSlot(refId), &hyp = hypSlot(hypId);
		verify(!ref.empty() && (ref.front().label != Fsa::Epsilon));
		Fsa::LabelId refLabel = ref.front().label;
		for (ConfusionNetwork::Slot::const_iterator itHyp = hyp.begin(), endHyp = hyp.end(); itHyp != endHyp; ++itHyp)
		    if (itHyp->label == refLabel)
			return -0.002;
		return 1.0;
	    }

	    static PosteriorCostRef create() {
		return PosteriorCostRef(new OracleCost);
	    }
	};

	/*
	  i**a, where
	  a is a constant and
	  i the position of the reference word in the slot,
	  resp. 100 if the reference word does not occur in the slot

	  alpha = 0.0 -> minimizes the "top" error rate
	*/
	class WeightedOracleCost : public PosteriorCost {
	private:
	    f64 alpha_;
	public:
	    WeightedOracleCost(f64 alpha) : PosteriorCost(), alpha_(alpha) {}

	    virtual std::string describe() const { return Core::form("weighted-oracle-cost(alpha=%.2f)", alpha_); }

	    virtual Score insCost(u32 hypId) const {
		const ConfusionNetwork::Slot &hyp = hypSlot(hypId);
		for (ConfusionNetwork::Slot::const_iterator itHyp = hyp.begin(), endHyp = hyp.end(); itHyp != endHyp; ++itHyp)
		    if (itHyp->label == Fsa::Epsilon)
			return ::pow(f64(itHyp - hyp.begin()) + 0.001, alpha_);
		return ::pow(100.0, alpha_);
	    }

	    virtual Score delCost(u32 refId) const {
		const ConfusionNetwork::Slot &ref = refSlot(refId);
		verify(!ref.empty() && (ref.front().label != Fsa::Epsilon));
		return ::pow(100.0, alpha_);
	    }

	    virtual Score subCost(u32 refId, u32 hypId) const {
		const ConfusionNetwork::Slot &ref = refSlot(refId), &hyp = hypSlot(hypId);
		verify(!ref.empty() && (ref.front().label != Fsa::Epsilon));
		Fsa::LabelId refLabel = ref.front().label;
		for (ConfusionNetwork::Slot::const_iterator itHyp = hyp.begin(), endHyp = hyp.end(); itHyp != endHyp; ++itHyp)
		    if (itHyp->label == refLabel)
			return ::pow(f64(itHyp - hyp.begin()), alpha_);
		return ::pow(100.0, alpha_);
	    }

	    static PosteriorCostRef create(f64 alpha = 1.0) {
		return PosteriorCostRef(new WeightedOracleCost(alpha));
	    }
	};

	/*
	  1.0 - p(reference word)
	*/
	class OracleLoss : public PosteriorCost {
	public:
	    static const Score ErrorCost;
	public:
	    virtual std::string describe() const { return "oracle-loss"; }

	    virtual bool requiresPosteriorId() const { return true; }

	    virtual Score insCost(u32 hypId) const {
		const ConfusionNetwork::Slot &hyp = hypSlot(hypId);
		verify(posteriorId != Semiring::InvalidId);
		Score cost = 1.0;
		for (ConfusionNetwork::Slot::const_iterator it = hyp.begin(), end = hyp.end(); it != end; ++it)
		    if (it->label == Fsa::Epsilon)
			cost -= it->scores->get(posteriorId);
		return (cost == 1.0) ? ErrorCost : cost;
	    }

	    virtual Score delCost(u32 refId) const {
		const ConfusionNetwork::Slot &ref = refSlot(refId);
		verify(!ref.empty() && (ref.front().label != Fsa::Epsilon));
		return ErrorCost;
	    }

	    virtual Score subCost(u32 refId, u32 hypId) const {
		const ConfusionNetwork::Slot &ref = refSlot(refId), &hyp = hypSlot(hypId);
		verify(!ref.empty() && (ref.front().label != Fsa::Epsilon));
		verify(posteriorId != Semiring::InvalidId);
		Fsa::LabelId refLabel = ref.front().label;
		Score cost = 1.0;
		for (ConfusionNetwork::Slot::const_iterator it = hyp.begin(), end = hyp.end(); it != end; ++it)
		    if (it->label == refLabel)
			cost -= it->scores->get(posteriorId);
		return (cost == 1.0) ? ErrorCost : cost;
	    }

	    static PosteriorCostRef create() {
		return PosteriorCostRef(new OracleLoss);
	    }
	};

    private:
	PosteriorCostRef posteriorCostFcn_;
	Key posteriorKey_;
	Lexicon::SymbolMap lemmaSymbolMap_;

    private:
	void build(ConfusionNetwork *alignedCn, ConstConfusionNetworkRef ref, ConstConfusionNetworkRef cn) const {
	    ConfusionNetwork::OracleAlignmentProperties *oracleProps = new ConfusionNetwork::OracleAlignmentProperties;
	    oracleProps->semiring = ref->semiring;
	    alignedCn->semiring = cn->semiring;
	    alignedCn->alphabet = cn->alphabet;
	    alignedCn->normalizedProperties = cn->normalizedProperties;
	    alignedCn->nBestAlignmentProperties = cn->nBestAlignmentProperties;
	    alignedCn->oracleAlignmentProperties = ConfusionNetwork::ConstOracleAlignmentPropertiesRef(oracleProps);
	    ConfusionNetwork::Arc epsRefArc(Fsa::Epsilon, ref->semiring->one());
	    u32 n = 1;
	    if (alignedCn->isNBestAlignment()) {
		n = alignedCn->nBestAlignmentProperties->n;
	    }
	    ScoresRef epsScore = alignedCn->semiring->one();
	    if (alignedCn->isNormalized()) {
		epsScore = alignedCn->semiring->clone(epsScore);
		epsScore->set(alignedCn->normalizedProperties->posteriorId, 1.0 / Score(n));
	    }
	    ConfusionNetwork::Slot epsSlot(n, ConfusionNetwork::Arc(Fsa::Epsilon, epsScore));
	    ConfusionNetwork::const_iterator itRefSlot = ref->begin();
	    ConfusionNetwork::const_iterator itSlot = cn->begin();
	    ConfusionNetwork::OracleAlignmentProperties::OracleAlignment &oracleAlignment = oracleProps->alignment;
	    for (const_iterator itPair = begin(), endPair = end(); itPair != endPair; ++itPair) {
		if (itPair->first != Core::Type<u32>::max) {
		    oracleAlignment.push_back(itRefSlot->front());
		    ++itRefSlot;
		} else
		    oracleAlignment.push_back(epsRefArc);
		if (itPair->second != Core::Type<u32>::max) {
		    alignedCn->push_back(*itSlot);
		    ++itSlot;
		} else
		    alignedCn->push_back(epsSlot);
	    }
	    verify((itRefSlot == ref->end()) && (itSlot == cn->end()));
	    verify(alignedCn->size() == oracleAlignment.size());
	    if (cn->hasMap()) {
		StateIdList slotMap(cn->size());
		StateIdList::iterator itSlotMap = slotMap.begin();
		Fsa::StateId toSlotId = 0;
		for (const_iterator itPair = begin(), endPair = end(); itPair != endPair; ++itPair, ++toSlotId)
		    if (itPair->second != Core::Type<u32>::max) {
			*itSlotMap = toSlotId;
			++itSlotMap;
		    }
		verify((itSlotMap == slotMap.end()) && (toSlotId == alignedCn->size()));

		ConfusionNetwork::MapProperties *mapProperties = new ConfusionNetwork::MapProperties;
		mapProperties->stateIndex = cn->mapProperties->stateIndex;
		mapProperties->lat2cn = cn->mapProperties->lat2cn;
		for (ConfusionNetwork::MapProperties::Map::iterator itMap = mapProperties->lat2cn.begin(), endMap = mapProperties->lat2cn.end();
		     itMap != endMap; ++itMap) if (itMap->sid != Fsa::InvalidStateId) {
			verify(itMap->sid < slotMap.size());
			verify(itMap->sid <= slotMap[itMap->sid]);
			itMap->sid = slotMap[itMap->sid];
		    }
		verify(mapProperties->stateIndex.back() == mapProperties->lat2cn.size());
		if (!cn->mapProperties->slotIndex.empty()) {
		    mapProperties->slotIndex.grow(alignedCn->size(), Fsa::InvalidStateId);
		    mapProperties->cn2lat = cn->mapProperties->cn2lat;
		    Fsa::StateId sid = 0;
		    for (StateIdList::const_iterator itSlotMap = slotMap.begin(); itSlotMap != slotMap.end(); ++itSlotMap, ++sid) {
			verify(sid <= *itSlotMap);
			mapProperties->slotIndex[*itSlotMap] = cn->mapProperties->slotIndex[sid];
		    }
		    u32 previousOffset = mapProperties->cn2lat.size();
		    for (Core::Vector<Fsa::StateId>::reverse_iterator itOffset = mapProperties->slotIndex.rbegin(), endOffset = mapProperties->slotIndex.rend();
			 itOffset != endOffset; ++itOffset)
			if (*itOffset == Fsa::InvalidStateId)
			    *itOffset = previousOffset;
			else
			    previousOffset = *itOffset;
		    verify(mapProperties->slotIndex.back() == mapProperties->cn2lat.size());

		    // dbg
		    /*
		    for (ConfusionNetwork::MapProperties::Map::iterator itMap = mapProperties->lat2cn.begin(), endMap = mapProperties->lat2cn.end();
			 itMap != endMap; ++itMap) if ((itMap->sid != Fsa::InvalidStateId) && (itMap->aid != Fsa::InvalidStateId)) {
			    const ConfusionNetwork::MapProperties::Mapping &toLatMap = mapProperties->latticeArc(itMap->sid, itMap->aid);
			    const ConfusionNetwork::MapProperties::Mapping &toCnMap = mapProperties->slotArc(toLatMap.sid, toLatMap.aid);
			    verify(itMap->sid == toCnMap.sid);
			    verify(itMap->aid == toCnMap.aid);
			}
		    */

		}
		alignedCn->mapProperties = ConfusionNetwork::ConstMapPropertiesRef(mapProperties);
	    }
	}

	void lat2cn(ConfusionNetwork *cn, ConstLatticeRef l) const {
	    ConstBoundariesRef b = l->getBoundaries();
	    if (!b->valid())
		b.reset();
	    ConfusionNetwork::Slot tmpSlot(1, ConfusionNetwork::Arc(
					       Fsa::Epsilon,
					       cn->semiring->one()));
	    for (ConstStateRef sr = l->getState(l->initialStateId()), nextSr;  sr->hasArcs(); sr = nextSr) {
		if (sr->nArcs() > 1)
		    Core::Application::us()->criticalError(
			"CnOracleAlignmentNode: Reference lattice \"%s\" is not linear.",
			l->describe().c_str());
		nextSr = l->getState(sr->begin()->target());
		const Arc &arc = *sr->begin();
		if (arc.input() != Fsa::Epsilon) {
		    cn->push_back(tmpSlot);
		    ConfusionNetwork::Arc &cnArc = cn->back().front();
		    cnArc.label = arc.input();
		    cnArc.scores = arc.weight();
		    if (b) {
			cnArc.begin = b->time(sr->id());
			cnArc.duration = b->time(nextSr->id()) - cnArc.begin;
		    }
		}
	    }
	}

	void str2cn(ConfusionNetwork *cn, const std::string &str) const {
	    ConfusionNetwork::Slot tmpSlot(1, ConfusionNetwork::Arc(
					       Fsa::Epsilon,
					       cn->semiring->one()));
	    const char *b = str.c_str(), *c;
	    for (; (*b != '\0') && ::isspace(*b); ++b);
	    while (*b != '\0') {
		for (c = b + 1; (*c != '\0') && !::isspace(*c); ++c);
		Fsa::LabelId label = lemmaSymbolMap_.index(std::string(b, c - b));
		if (label != Fsa::Epsilon) {
		    cn->push_back(tmpSlot);
		    cn->back().front().label = (label == Fsa::InvalidLabelId) ?
			Lexicon::us()->unkLemmaId() : label;
		}
		for (b = c; (*b != '\0') && ::isspace(*b); ++b);
	    }
	}

	OracleAlignment(PosteriorCostRef posteriorCostFcn, const Key &posteriorKey, u32 minBeamWidth) :
	    Precursor(posteriorCostFcn, minBeamWidth),
	    posteriorCostFcn_(posteriorCostFcn),
	    posteriorKey_(posteriorKey) {
	    lemmaSymbolMap_ = Lexicon::us()->symbolMap(Lexicon::LemmaAlphabetId);
	}

    public:
	void dump(std::ostream &os) const {
	    os << "Oracle alignment:" << std::endl;
	    if (!posteriorKey_.empty())
		os << "  posterior-key=" << posteriorKey_ << std::endl;
	    Precursor::dump(os);
	}

	std::pair<ConstConfusionNetworkRef, Score> align(ConstConfusionNetworkRef ref, ConstConfusionNetworkRef cn) const {
	    verify(ref && cn);
	    ConfusionNetwork *alignedCnPtr = new ConfusionNetwork;
	    if (posteriorCostFcn_->requiresPosteriorId()) {
		if (posteriorKey_.empty()) {
		    if (!cn->isNormalized())
			Core::Application::us()->criticalError(
			    "Failed to find posterior dimension: Cn is not normalized and no posterior dimension is given.");
		    posteriorCostFcn_->setPosteriorId(cn->normalizedProperties->posteriorId);
		} else {
		    ScoreId posteriorId = cn->semiring->id(posteriorKey_);
		    if (posteriorId == Semiring::InvalidId)
			Core::Application::us()->criticalError(
			    "Semiring \"%s\" has no dimension labeled \"%s\".",
			    cn->semiring->name().c_str(), posteriorKey_.c_str());
		    posteriorCostFcn_->setPosteriorId(posteriorId);
		}
	    }
	    Score cost = Precursor::align(*ref, *cn);
	    build(alignedCnPtr, ref, cn);
	    return std::make_pair(ConstConfusionNetworkRef(alignedCnPtr), cost);
	}

	std::pair<ConstConfusionNetworkRef, Score> align(ConstLatticeRef ref, ConstConfusionNetworkRef cn) const {
	    if (Lexicon::us()->alphabetId(ref->getInputAlphabet()) != Lexicon::us()->alphabetId(cn->alphabet))
		Core::Application::us()->criticalError(
		    "Reference and CN use different alphabet; \"%s\" != \"%s\".",
		    Lexicon::us()->alphabetName(Lexicon::us()->alphabetId(ref->getInputAlphabet())).c_str(),
		    Lexicon::us()->alphabetName(Lexicon::us()->alphabetId(cn->alphabet)).c_str());
	    ConfusionNetwork *cnRefPtr = new ConfusionNetwork;
	    cnRefPtr->alphabet = ref->getInputAlphabet();
	    cnRefPtr->semiring = ref->semiring();
	    lat2cn(cnRefPtr, ref);
	    ConstConfusionNetworkRef cnRef(cnRefPtr);
	    return align(cnRef, cn);
	}

	std::pair<ConstConfusionNetworkRef, Score> align(const std::string &ref, ConstConfusionNetworkRef cn) const {
	    if (Lexicon::us()->alphabetId(cn->alphabet) != Lexicon::LemmaAlphabetId)
		Core::Application::us()->criticalError(
		    "Reference is assumed to be a lemma string, but CN's alphabet is \"%s\".",
		    Lexicon::us()->alphabetName(Lexicon::us()->alphabetId(cn->alphabet)).c_str());
	    ConfusionNetwork *cnRefPtr = new ConfusionNetwork;
	    cnRefPtr->alphabet = cn->alphabet;
	    cnRefPtr->semiring = OracleAlignment::emptySemiring();
	    str2cn(cnRefPtr, ref);
	    ConstConfusionNetworkRef cnRef(cnRefPtr);
	    return align(cnRef, cn);
	}

	static ConstOracleAlignmentRef create(
	    PosteriorCostRef posteriorCostFcn,
	    const Key &posteriorKey,
	    u32 minBeamWidth) {
	    verify(posteriorCostFcn);
	    return ConstOracleAlignmentRef(new OracleAlignment(posteriorCostFcn, posteriorKey, minBeamWidth));
	}
    };
    const Score OracleAlignment::OracleLoss::ErrorCost = 100.0;
    ConstSemiringRef OracleAlignment::emptySemiring_ = Semiring::create(Fsa::SemiringTypeTropical, 0);
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class OracleAlignmentNode : public Node {
    public:
    public:
	typedef enum {
	    OracleCost,
	    WeightedOracleCost,
	    OracleLoss
	} CostFcn;
	static const Core::Choice choiceCostFcn;
	static const Core::ParameterChoice paramCostFcn;
	static const Core::ParameterFloat paramAlpha;

    private:
	ConstOracleAlignmentRef oracleAligner_;
	ConstConfusionNetworkRef cn_;

    private:
	ConstConfusionNetworkRef getCn() {
	    if (!cn_) {
		ConstConfusionNetworkRef cn = requestCn(0);
		if (cn) {
		    std::pair<ConstConfusionNetworkRef, Score> result;
		    if (connected(1)) {
			ConstLatticeRef ref = requestLattice(1);
			if (ref)
			    result = oracleAligner_->align(ref, cn);
			else
			    warning("Empty reference lattice from port 1.");
		    } else if (connected(2)) {
			std::string ref = requestString(2);
			result = oracleAligner_->align(ref, cn);
		    } else if (connected(3)) {
			ConstConfusionNetworkRef ref = requestCn(3);
			if (ref)
			    result = oracleAligner_->align(ref, cn);
			else
			    warning("Empty reference CN from port 3.");
		    } else if (connected(4)) {
			ConstSegmentRef segment = requestSegment(4);
			require(segment->hasOrthography());
			result = oracleAligner_->align(segment->orthography(), cn);
		    } else
			defect();
		    if (result.first) {
			cn_ = result.first;
			log() << Core::XmlFull("cost", result.second);
		    }
		} else
		    warning("Empty CN from port 0.");
	    }
	    return cn_;
	}

    public:
	OracleAlignmentNode(const std::string &name, const Core::Configuration &config) :
	    Node(name, config) {}
	virtual ~OracleAlignmentNode() {}

	virtual void init(const std::vector<std::string> &arguments) {
	    if (!connected(0))
		criticalError("CN at port 0 required.");
	    if (!connected(1) && !connected(2) && !connected(3) && !connected(4))
		criticalError(
		    "Either a linear lattice at port 1, a string at port 2, a linear CN at port 3, "
		    "or a segment at port 4 providing an orthography is required.");
	    OracleAlignment::PosteriorCostRef costFcn;
	    Core::Choice::Value costChoice = paramCostFcn(config);
	    if (costChoice == Core::Choice::IllegalValue)
		criticalError("Unknwon cost fucntion.");
	    switch (CostFcn(costChoice)) {
	    case OracleCost:
		costFcn = OracleAlignment::OracleCost::create();
		break;
	    case WeightedOracleCost:
		costFcn = OracleAlignment::WeightedOracleCost::create(paramAlpha(choiceCostFcn[costChoice]));
		break;
	    case OracleLoss:
		costFcn = OracleAlignment::OracleLoss::create();
		break;
	    default:
		defect();
	    }
	    oracleAligner_ = OracleAlignment::create(
		costFcn, WeightedPosteriorCombinationHelper::paramPosteriorKey(config, ""), paramBeamWidth(config));
	    oracleAligner_->dump(log());
	}

	virtual void finalize() {}

	virtual ConstConfusionNetworkRef sendCn(Port to) {
	    switch (to) {
	    case 0:
		return getCn();
	    default:
		defect();
		return ConstConfusionNetworkRef();
	    }
	}

	virtual void sync() {
	    cn_.reset();
	}
    };
    const Core::Choice OracleAlignmentNode::choiceCostFcn(
	"oracle-cost", OracleAlignmentNode::OracleCost,
	"weighted-oracle-cost", OracleAlignmentNode::OracleCost,
	"oracle-loss", OracleAlignmentNode::OracleLoss,
	Core::Choice::endMark());
    const Core::ParameterChoice OracleAlignmentNode::paramCostFcn(
	"cost",
	&OracleAlignmentNode::choiceCostFcn,
	"cost function",
	OracleAlignmentNode::OracleCost);
    const Core::ParameterFloat OracleAlignmentNode::paramAlpha(
	"alpha",
	"alpha",
	1.0);

    NodeRef createOracleAlignmentNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new OracleAlignmentNode(name, config));
    }
    // -------------------------------------------------------------------------

} // namespace
