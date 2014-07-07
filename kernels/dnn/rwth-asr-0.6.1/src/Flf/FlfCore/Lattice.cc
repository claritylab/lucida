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
#include "Lattice.hh"

namespace Flf {

    // -------------------------------------------------------------------------
#ifdef MEM_DBG
    Lattice::nLattices = 0;

    Lattice::Lattice() :
	Precursor(), boundaries_(InvalidBoundaries) {
	++nLattices;
    }

    Lattice::~Lattice() {
	--nLattices;
    }
#else
    Lattice::Lattice() :
	Precursor(), boundaries_(InvalidBoundaries) {}

    Lattice::~Lattice() {}
#endif

    void Lattice::setBoundaries(ConstBoundariesRef boundaries) const {
	boundaries_ = boundaries;
	verify(boundaries_);
    }

    void Lattice::setTopologicalSort(ConstStateMapRef topologicalSort) const {
	topologicalSort_ = topologicalSort;
	if (topologicalSort_) {
	    verify(!knowsProperty(Fsa::PropertyAcyclic) || hasProperty(Fsa::PropertyAcyclic));
	    addProperties(Fsa::PropertyAcyclic);
	}
    }

    void Lattice::dumpState(Fsa::StateId sid, std::ostream &os) const {
	os << sid;
	ConstStateMapRef topologicalSort = getTopologicalSort();
	if (topologicalSort) {
	    os << "\\nT=" << (*topologicalSort)[sid];
	}
	const Boundary &b = boundary(sid);
	if (b.valid()) {
	    os << "\\nt=" << b.time();
	    if (b.transit().boundary == AcrossWordBoundary)
		os << ",xw";
	    else
		os << ",ww";
	    if ((b.transit().initial != Bliss::Phoneme::term) || (b.transit().final != Bliss::Phoneme::term))
		os << "\\ncoart.";
	}
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    namespace {
	template<class Cn>
	typename Cn::Slot::const_iterator binary_search(const typename Cn::Slot &slot, Fsa::LabelId label) {
	    /*
	    if (slot.size() <= 8) {
		Cn::Slot::const_iterator it = slot.begin(), end = slot.end();
		for (; (it != end) && (it->label < label); ++it);
		return ((it == end) || (it->label != label)) ? slot.end() : it;
	    } else {
	    */
		Fsa::LabelId tmp;
		s32 l = 0, r = slot.end() - slot.begin() - 1, m;
		while (l <= r) {
		    // m = (l + r) / 2;
		    m = (s32)((u32)(l + r) >> 1);
		    verify_((l <= r) && (0 <= m) && (m < slot.size()));
		    tmp = (slot.begin() + m)->label;
		    if (label > tmp)
			l = m + 1;
		    else if (label < tmp)
			r = m - 1;
		    else
			return (slot.begin() + m);
		}
		return slot.end();
	    /*
	    }
	    */
	}

	inline Score binary_score(const ConfusionNetwork::Slot &slot, Fsa::LabelId label, ScoreId posteriorId) {
	    ConfusionNetwork::Slot::const_iterator a = binary_search<ConfusionNetwork>(slot, label);
	    return (a == slot.end()) ? 0.0 : a->scores->get(posteriorId);
	}

	inline Score binary_score(const PosteriorCn::Slot &slot, Fsa::LabelId label) {
	    PosteriorCn::Slot::const_iterator a = binary_search<PosteriorCn>(slot, label);
	    return (a == slot.end()) ? 0.0 : a->score;
	}
    } // namespace
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    Score ConfusionNetwork::NormalizedProperties::posteriorScore(const ConfusionNetwork::Slot &slot, Fsa::LabelId label) const {
	return binary_score(slot, label, posteriorId);
    }

    const ConfusionNetwork::MapProperties::Mapping ConfusionNetwork::MapProperties::InvalidMapping =
		     ConfusionNetwork::MapProperties::Mapping(Fsa::InvalidStateId, Fsa::InvalidStateId);

    const ConfusionNetwork::MapProperties::Mapping & ConfusionNetwork::MapProperties::slotArc(Fsa::StateId stateId, Fsa::StateId aid) const {
	if ((stateId == Fsa::InvalidStateId) || (aid == Fsa::InvalidStateId))
	    return InvalidMapping;
	verify((stateId < stateIndex.size()) && (stateIndex[stateId] + aid < lat2cn.size()));
	return lat2cn[stateIndex[stateId] + aid];
    }

    const ConfusionNetwork::MapProperties::Mapping & ConfusionNetwork::MapProperties::latticeArc(Fsa::StateId slotId, Fsa::StateId aid) const {
	if ((slotId == Fsa::InvalidStateId) || (aid == Fsa::InvalidStateId))
	    return InvalidMapping;
	verify((slotId < slotIndex.size()) && (slotIndex[slotId] + aid < cn2lat.size()));
	return cn2lat[slotIndex[slotId] + aid];
    }

    ConfusionNetwork::MapProperties::Map::const_iterator ConfusionNetwork::MapProperties::state(Fsa::StateId stateId) const {
	if (stateId < stateIndex.size())
	    return lat2cn.begin() + stateIndex[stateId];
	return lat2cn.end();
    }

    ConfusionNetwork::MapProperties::Map::const_iterator ConfusionNetwork::MapProperties::slot(Fsa::StateId slotId) const {
	if (slotId < slotIndex.size())
	    return cn2lat.begin() + slotIndex[slotId];
	return cn2lat.end();
    }

    void ConfusionNetwork::MapProperties::reduce() const {
	for (Map::iterator itMap = lat2cn.begin(), endMap = lat2cn.end(); itMap != endMap; ++itMap)
	    itMap->aid = Fsa::InvalidStateId;
	stateIndex.clear();
	cn2lat.clear();
    }

    bool ConfusionNetwork::MapProperties::isReduced() const {
	return !stateIndex.empty() && slotIndex.empty();
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    Probability PosteriorCn::score(size_t t, Fsa::LabelId label) const {
	if (t < size()) return binary_score(operator[](t), label);
	else return (label == Fsa::Epsilon) ? 1.0 : 0.0;
    }

    void PosteriorCn::scores(ProbabilityList::iterator itScore, ProbabilityList::iterator endScore, size_t t, Fsa::LabelId label) const {
	if (t < size()) {
	    for (const_iterator itSlot = begin() + t, endSlot = end(); (itScore != endScore) && (itSlot != endSlot); ++itScore, ++itSlot)
		*itScore = binary_score(*itSlot, label);
	}
	if (itScore != endScore) {
	    if (label == Fsa::Epsilon)
		for (; itScore != endScore; ++itScore) *itScore = 1.0;
	    else
		for (; itScore != endScore; ++itScore) *itScore = 0.0;
	}
    }
    // -------------------------------------------------------------------------


} // namespace Flf
