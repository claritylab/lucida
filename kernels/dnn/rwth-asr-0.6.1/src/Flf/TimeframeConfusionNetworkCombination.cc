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
#include "TimeframeConfusionNetwork.hh"
#include "TimeframeConfusionNetworkCombination.hh"

namespace Flf {

    typedef std::vector<ScoreList> WeightsList;

    // -------------------------------------------------------------------------
    namespace {
	typedef std::pair<PosteriorCn::Slot::const_iterator, PosteriorCn::Slot::const_iterator> SlotRange;
	typedef Core::Vector<SlotRange> SlotRangeList;

	const PosteriorCn::Slot EpsSlot = PosteriorCn::Slot(1, PosteriorCn::Arc(Fsa::Epsilon, 1.0));
    } // namespace

    /**
     * entropy
     **/
    f64 slotEntropy(const PosteriorCn::Slot &slot) {
	if (slot.size() == 1)
	    return 0.0;
	f64 entropy = 0.0;
	for (PosteriorCn::Slot::const_iterator itArc = slot.begin(), endArc = slot.end(); itArc != endArc; ++itArc)
	    entropy += itArc->score * ::log2(itArc->score);
	return -entropy;
    }

    /**
     * minimum entropy weighting
     **/
    void minEntropyWeights(
	WeightsList &weightsList,
	const ConstPosteriorCnRefList &cns,
	const ScoreList &_weights) {
	const std::vector<f64> weights(_weights.begin(), _weights.end());
	for (u32 i = 0; i < weightsList.size(); ++i) {
	    ScoreList &slotWeights = weightsList[i];
	    std::fill(slotWeights.begin(), slotWeights.end(), 0.0);
	    f64 minEntropy = Core::Type<f64>::max;
	    u32 minEntropyIdx = Core::Type<u32>::max;
	    std::vector<f64>::const_iterator itWeight = weights.begin();
	    for (u32 j = 0; j < cns.size(); ++j, ++itWeight) {
		const PosteriorCn::Slot &slot = (i < cns[j]->size()) ? (*cns[j])[i] : EpsSlot;
		const f64 entropy = *itWeight * slotEntropy(slot);
		if (entropy < minEntropy) {
		    minEntropy = entropy;
		    minEntropyIdx = j;
		}
	    }
	    slotWeights[minEntropyIdx] = 1.0;

	    //dbg
	    /*
	    for (u32 j = 0; j < cns.size(); ++j)
		dbg(j << ". " << weights[j] << " -> " << weightsList[i][j]);
	    dbg("");
	    */

	}
    }

    /**
     * inverse, thresholded entropy weighting
     **/
    f64 inverseSlotEntropy(const PosteriorCn::Slot &slot) {
	f64 entropy = slotEntropy(slot);
	if (entropy < 0.008) return 125.0;
	//if (entropy > 1.5)   return 0.0001;
	return 1.0/entropy;
    }

    void inverseEntropyWeights(
	WeightsList &weightsList,
	const ConstPosteriorCnRefList &cns,
	const ScoreList &_weights) {
	const std::vector<f64> weights(_weights.begin(), _weights.end());
	std::vector<f64> weightedInverseEntropies(_weights.size(), 0.0);
	for (u32 i = 0; i < weightsList.size(); ++i) {
	    f64 sumWeightedInverseEntropy = 0.0;
	    std::vector<f64>::const_iterator itWeight = weights.begin();
	    for (u32 j = 0; j < cns.size(); ++j, ++itWeight) {
		const PosteriorCn::Slot &slot = (i < cns[j]->size()) ? (*cns[j])[i] : EpsSlot;
		f64 &weightedInverseEntropy = weightedInverseEntropies[j];
		weightedInverseEntropy = *itWeight * inverseSlotEntropy(slot);
		sumWeightedInverseEntropy += weightedInverseEntropy;
	    }
	    const f64 normWeightedInverseEntropy = 1.0 / sumWeightedInverseEntropy;
	    ScoreList &slotWeights = weightsList[i];
	    ScoreList::iterator itSlotWeight = slotWeights.begin();
	    for (std::vector<f64>::const_iterator itWeightedInverseEntropy = weightedInverseEntropies.begin();
		 itWeightedInverseEntropy != weightedInverseEntropies.end(); ++itWeightedInverseEntropy, ++itSlotWeight)
		*itSlotWeight = normWeightedInverseEntropy * *itWeightedInverseEntropy;

	    //dbg
	    /*
	    for (u32 j = 0; j < cns.size(); ++j)
		dbg(j << ". " << weights[j] << " -> " << weightsList[i][j]);
	    dbg("");
	    */

	}
    }

    void estimateJointPosteriorCn(
	PosteriorCn &jointCn,
	const ConstPosteriorCnRefList &cns,
	const WeightsList &weightsList) {
	ensure(jointCn.size() == weightsList.size());
	for (u32 i = 0; i < jointCn.size(); ++i) {
	    SlotRangeList slotRanges(cns.size());
	    Fsa::LabelId nextLabel = Core::Type<Fsa::LabelId>::max;
	    for (u32 j = 0; j < cns.size(); ++j) {
		const PosteriorCn::Slot &pdf = (i < cns[j]->size()) ? (*cns[j])[i] : EpsSlot;
		slotRanges[j] = std::make_pair(pdf.begin(), pdf.end());
		verify_(slotRanges[j].first != slotRanges[j].second);
		verify_(slotRanges[j].first->label < Core::Type<Fsa::LabelId>::max);
		nextLabel = std::min(nextLabel, slotRanges[j].first->label);
	    }
	    const ScoreList &weights = weightsList[i];
	    PosteriorCn::Slot &jointPdf = jointCn[i];
	    ensure_(jointPdf.empty());
	    do {
		jointPdf.push_back(PosteriorCn::Arc(nextLabel, 0.0));
		nextLabel = Core::Type<Fsa::LabelId>::max;
		for (u32 j = 0; j < slotRanges.size(); ++j) {
		    SlotRange &slotRange(slotRanges[j]);
		    if (slotRange.first != slotRange.second) {
			if (slotRange.first->label == jointPdf.back().label) {
			    jointPdf.back().score += weights[j] * slotRange.first->score;
			    if (++slotRange.first != slotRange.second)
				nextLabel = std::min(nextLabel, slotRange.first->label);
			} else {
			    verify_(jointPdf.back().label < slotRange.first->label);
			    verify(slotRange.first->label < Core::Type<Fsa::LabelId>::max);
			    nextLabel = std::min(nextLabel, slotRange.first->label);
			}
		    }
		}
	    } while (nextLabel != Core::Type<Fsa::LabelId>::max);
	}
    }

    void estimateMaxPosteriorCn(
	PosteriorCn &maxCn,
	const ConstPosteriorCnRefList &cns,
	const WeightsList &weightsList) {
	ensure(cns.size() == weightsList.size());
	for (u32 i = 0; i < maxCn.size(); ++i) {
	    SlotRangeList slotRanges(cns.size());
	    Fsa::LabelId nextLabel = Core::Type<Fsa::LabelId>::max;
	    for (u32 j = 0; j < cns.size(); ++j) {
		const PosteriorCn::Slot &pdf = (i < cns[j]->size()) ? (*cns[j])[i] : EpsSlot;
		slotRanges[j] = std::make_pair(pdf.begin(), pdf.end());
		verify_(slotRanges[j].first != slotRanges[j].second);
		verify_(slotRanges[j].first->label < Core::Type<Fsa::LabelId>::max);
		nextLabel = std::min(nextLabel, slotRanges[j].first->label);
	    }
	    const ScoreList &weights = weightsList[i];
	    PosteriorCn::Slot &maxPdf = maxCn[i];
	    ensure_(maxPdf.empty());
	    do {
		maxPdf.push_back(PosteriorCn::Arc(nextLabel, 0.0));
		nextLabel = Core::Type<Fsa::LabelId>::max;
		for (u32 j = 0; j < slotRanges.size(); ++j) {
		    SlotRange &slotRange = slotRanges[j];
		    if (slotRange.first != slotRange.second) {
			if (slotRange.first->label == maxPdf.back().label) {
			    f64 score = weights[j] * slotRange.first->score;
			    if (score > maxPdf.back().score)
				maxPdf.back().score = score;
			    if (++slotRange.first != slotRange.second)
				nextLabel = std::min(nextLabel, slotRange.first->label);
			} else {
			    verify_(maxPdf.back().label < slotRange.first->label);
			    verify(slotRange.first->label < Core::Type<Fsa::LabelId>::max);
			    nextLabel = std::min(nextLabel, slotRange.first->label);
			}
		    }
		}
	    } while (nextLabel != Core::Type<Fsa::LabelId>::max);
	}
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class FramePosteriorCnCombinationNode : public Node {
    public:
	static const Core::ParameterFloat paramWeight;
	typedef enum {
	    WeightingSchemeStatic,
	    WeightingSchemeMinEntropy,
	    WeightingSchemeInverseEntropy
	} WeightingScheme;
	static const Core::Choice choiceWeightingScheme;
	static const Core::ParameterChoice paramWeightingScheme;
    private:
	WeightingScheme weightingScheme_;
	std::vector<u32> ids_;
	ScoreList weights_;
	ConstPosteriorCnRef cn_;
	bool hasNext_;

    private:
	ConstPosteriorCnRef getPosteriorCn() {
	    if (!hasNext_) {
		if (ids_.size() == 1)
		    cn_ = requestPosteriorCn(ids_.front());
		else {
		    ConstPosteriorCnRefList cns(ids_.size());
		    u32 maxT = 0;
		    for (u32 i = 0; i < ids_.size(); ++i) {
			cns[i] = requestPosteriorCn(ids_[i]);
			if (!cns[i]) {
			    warning("No fCN provided at port %d, discard", i);
			    cns.clear();
			    break;
			}
			maxT = std::max(maxT, u32(cns[i]->size()));
		    }
		    if (!cns.empty()) {
			PosteriorCn *cnPtr = new PosteriorCn(maxT);
			cnPtr->alphabet = cns.front()->alphabet;
			WeightsList weightsList(maxT, weights_);
			switch (weightingScheme_) {
			case WeightingSchemeStatic:
			    break;
			case WeightingSchemeMinEntropy:
			    minEntropyWeights(weightsList, cns, weights_);
			    break;
			case WeightingSchemeInverseEntropy:
			    inverseEntropyWeights(weightsList, cns, weights_);
			    break;
			default:
			    defect();
			}
			estimateJointPosteriorCn(*cnPtr, cns, weightsList);
			cn_ = ConstPosteriorCnRef(cnPtr);
		    }
		}
		hasNext_ = true;
	    }
	    return cn_;
	}

    public:
	FramePosteriorCnCombinationNode(const std::string &name, const Core::Configuration &config) :
	    Node(name, config), weightingScheme_(WeightingSchemeStatic) {}
	virtual ~FramePosteriorCnCombinationNode() {}

	virtual void init(const std::vector<std::string> &arguments) {
	    u32 n = 0;
	    for (; connected(n); ++n);
	    if (n == 0)
		criticalError("At least one incoming fCN at port 0 required.");
	    Core::Component::Message msg = log();
	    Score sum = 0.0;
	    for (u32 i = 0; i < n; ++i) {
		Score weight = paramWeight(select(Core::form("lattice-%d", i)));
		if (weight != 0.0) {
		    ids_.push_back(i);
		    weights_.push_back(weight);
		    sum += weight;
		}
	    }
	    verify(sum != 0.0);
	    for (ScoreList::iterator it = weights_.begin(); it != weights_.end(); ++it)
		*it /= sum;
	    msg << "Combine " << ids_.size() << " fCNs.\n";
	    for (u32 i = 0; i < ids_.size(); ++i)
		msg << "fCN at port " << ids_[i] << " has weight " << weights_[i] << ".\n";
	    Core::Choice::Value weightingScheme = paramWeightingScheme(config);
	    if (weightingScheme ==  Core::Choice::IllegalValue)
		criticalError("Unknown weighting scheme");
	    msg << "Use weighting scheme: " << choiceWeightingScheme[weightingScheme] << "\n";
	    weightingScheme_ = WeightingScheme(weightingScheme);
	    hasNext_ = false;
	}

	virtual ConstLatticeRef sendLattice(Port to) {
	    verify(to == 0);
	    return posteriorCn2lattice(getPosteriorCn());
	}

	virtual ConstPosteriorCnRef sendPosteriorCn(Port to) {
	    verify(to == 1);
	    return getPosteriorCn();
	}

	virtual void sync() {
	    cn_.reset();
	    hasNext_ = false;
	}
    };
    const Core::ParameterFloat FramePosteriorCnCombinationNode::paramWeight(
	"weight",
	"weight",
	1.0);
    const Core::Choice FramePosteriorCnCombinationNode::choiceWeightingScheme(
	"static",          FramePosteriorCnCombinationNode::WeightingSchemeStatic,
	"min-entropy",     FramePosteriorCnCombinationNode::WeightingSchemeMinEntropy,
	"inverse-entropy", FramePosteriorCnCombinationNode::WeightingSchemeInverseEntropy,
	Core::Choice::endMark());
    const Core::ParameterChoice FramePosteriorCnCombinationNode::paramWeightingScheme(
	"weighting",
	&FramePosteriorCnCombinationNode::choiceWeightingScheme,
	"weighting scheme",
	FramePosteriorCnCombinationNode::WeightingSchemeStatic);
    NodeRef createFramePosteriorCnCombinationNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new FramePosteriorCnCombinationNode(name, config));
    }
    // -------------------------------------------------------------------------

} // namespace Flf
