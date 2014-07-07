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
#ifndef _FLF_CORE_LATTICE_HH
#define _FLF_CORE_LATTICE_HH

#include <Core/Assertions.hh>
#include <Core/ReferenceCounting.hh>
#include <Core/Vector.hh>
#include <Fsa/tAutomaton.hh>
//include <Fsa/tStorage.hh>
#include <Fsa/tStatic.hh>
//include <Fsa/Automaton.hh>

#include "Boundaries.hh"
#include "Semiring.hh"
#include "Types.hh"


namespace Flf {

    /**
     * Lattice arc
     *
     **/
    struct Arc {
	typedef ScoresRef Weight;

	Fsa::StateId target_;
	ScoresRef weight_;
	Fsa::LabelId input_;
	Fsa::LabelId output_;
	Arc() {}
	Arc(Fsa::StateId target, ScoresRef a, Fsa::LabelId input) :
	    target_(target), weight_(a), input_(input), output_(input) {}
	Arc(Fsa::StateId target, ScoresRef a, Fsa::LabelId input, Fsa::LabelId output) :
	    target_(target), weight_(a), input_(input), output_(output) {}
	~Arc() {}
	Fsa::StateId target() const { return target_; }
	ScoresRef    weight() const { return weight_; }
	Fsa::LabelId input()  const { return input_; }
	Fsa::LabelId output() const { return output_; }
	Score score(ScoreId i) const { return weight_->get(i); }
	void setTarget(Fsa::StateId target) { target_ = target; }
	void setWeight(const Weight &weight) { weight_ = weight; }
	void setInput(Fsa::LabelId input)  { input_ = input;  }
	void setOutput(Fsa::LabelId output) { output_ = output; }
	void setScore(ScoreId i, Score a) const { weight_->set(i, a); }
    };


    /**
     * Lattice state
     *
     **/
    typedef Ftl::State<Arc> State;
    typedef State::Ref StateRef;
    typedef Core::Vector<StateRef> StateRefList;
    typedef State::ConstRef ConstStateRef;
    typedef Core::Vector<ConstStateRef> ConstStateRefList;
    typedef Core::Vector<Fsa::StateId> StateIdList;

    /**
     * Mapping from (or to) state id to (or from) state id;
     * the direction is task dependent.
     *
     * If maxSid is not Fsa::InvalidStateId then it is the maximal state id in use.
     *
     **/
    class StateMap : public Core::Vector<Fsa::StateId>, public Core::ReferenceCounted {
	typedef Core::Vector<Fsa::StateId> Precursor;
    public:
	Fsa::StateId maxSid;
    public:
	StateMap() : Precursor(), maxSid(Fsa::InvalidStateId) {}
	StateMap(size_t n) : Precursor(n), maxSid(Fsa::InvalidStateId) {}
	StateMap(size_t n, Fsa::StateId def) : Precursor(n, def), maxSid(Fsa::InvalidStateId) {}
    };
    typedef Core::Ref<StateMap> StateMapRef;
    typedef Core::Ref<const StateMap> ConstStateMapRef;


    /**
     * Lattice
     *
     **/
    class Lattice : public Ftl::Automaton<Semiring,  State> {
	typedef Lattice Self;
	typedef Ftl::Automaton<Semiring, State> Precursor;
    public:
	typedef Core::Ref<Self>       Ref;
	typedef Core::Ref<const Self> ConstRef;

    private:
	mutable ConstBoundariesRef boundaries_;
	mutable ConstStateMapRef topologicalSort_;

    public:
#ifdef MEM_DBG
	static u32 nLattices;
#endif
    public:
	Lattice();
	virtual ~Lattice();

	ConstBoundariesRef getBoundaries() const { return boundaries_; }
	void setBoundaries(ConstBoundariesRef boundaries) const;
	const Boundary& boundary(Fsa::StateId id) const
	    { return boundaries_->get(id); }

	ConstStateMapRef getTopologicalSort() const { return topologicalSort_; }
	void setTopologicalSort(ConstStateMapRef topologicalSort) const;

	void dumpState(Fsa::StateId s, std::ostream &o) const;
    };
    typedef Lattice::ConstRef ConstLatticeRef;
    typedef Core::Vector<ConstLatticeRef> ConstLatticeRefList;

    // typedef Ftl::StorageAutomaton<Lattice> StorageLattice;
    // typedef Core::Ref<StorageLattice> StorageLatticeRef;

    typedef Ftl::StaticAutomaton<Lattice> StaticLattice;
    typedef Core::Ref<StaticLattice> StaticLatticeRef;


    /**
     * Abstract Confusion Networks;
     * arc type is abstract.
     *
     **/
    namespace Cn {
	typedef Fsa::LabelId Label;

	struct Arc {
	    Label label;
	    ScoresRef scores;
	    Time begin;
	    Time duration;
	    u32 from, to;
	    Arc() {
		begin = Speech::InvalidTimeframeIndex; duration = 0;
		from = to = Core::Type<u32>::max;
	    }
	    Arc(Label label, ScoresRef scores) :
		label(label), scores(scores),
		begin(Speech::InvalidTimeframeIndex), duration(0),
		from(Core::Type<u32>::max), to(Core::Type<u32>::max) {}
	    Arc(Label label, ScoresRef scores, Time begin, Time duration) :
		label(label), scores(scores), begin(begin), duration(duration),
		from(Core::Type<u32>::max), to(Core::Type<u32>::max) {}
	    Arc(Label label, ScoresRef scores, Time begin, Time duration, u32 from, u32 to) :
		label(label), scores(scores), begin(begin), duration(duration), from(from), to(to) {}
	    inline bool operator< (const Arc &a) const
		{ return label < a.label; }
	};

	struct PosteriorArc {
	    Label label;
	    Probability score;
	    PosteriorArc() {}
	    PosteriorArc(Label label, Probability score) : label(label), score(score) {}
	    inline bool operator< (const PosteriorArc &a) const
		{ return label < a.label; }
	};

	template<typename Arc_>
	class ConfusionNetwork :
	    public Core::Vector<Core::Vector<Arc_> >,
	    public Core::ReferenceCounted {
	    typedef Core::Vector<Core::Vector<Arc_> > Precursor;
	public:
	    typedef Arc_ Arc;
	    typedef Core::Vector<Arc> Slot;
	public:
	    Fsa::ConstAlphabetRef alphabet;
	public:
	    ConfusionNetwork() : Precursor(0) {}
	    ConfusionNetwork(size_t n) : Precursor(n) {}
	    ConfusionNetwork(size_t n, const Slot &slot) : Precursor(n, slot) {}
	};
    };


    /**
     * (General) Confusion Network;
     * an arc stores basically all available information.
     *
     **/
    class ConfusionNetwork: public Cn::ConfusionNetwork<Cn::Arc> {
	typedef Cn::ConfusionNetwork<Cn::Arc> Precursor;
    public:
	/**
	 * Each slot has the following properties
	 * 1) each word occurs at most once
	 * 2) the slot arcs are sorted by label ids, starting with the lowest label id
	 * 3) if posteriorId is valid, then
	 *    sum over all scores[posteriorId] per slot equals 1.0,
	 *    i.e. each slot describes a probability distribution over all words
	 * 4) If dumped to text (plain text or XML), then slots are sorted
	 *    in descending order by scores[posteriorId].
	 * -> see "oracle alignment"
	 **/
	struct NormalizedProperties : public Core::ReferenceCounted {
	    ScoreId posteriorId;
	    NormalizedProperties(ScoreId posteriorId) : posteriorId(posteriorId) {}
	    Score posteriorScore(const ConfusionNetwork::Slot &slot, Fsa::LabelId label) const;
	};
	typedef Core::Ref<const NormalizedProperties> ConstNormalizedPropertiesRef;

	/**
	 * Each slot has exactly N entries.
	 * -> see "oracle alignment"
	 **/
	struct NBestAlignmentProperties : public Core::ReferenceCounted {
	    u32 n;
	    NBestAlignmentProperties(u32 n) : n(n) {}
	};
	typedef Core::Ref<const NBestAlignmentProperties> ConstNBestAlignmentPropertiesRef;

	/**
	 * Additive to "normalized": probabiliy is normalized over all arcs in slot but the first (aligned) arc
	 * Additive to "n-best alignment": each slot has n+1 arcs, where the first arc is the aligned arc
	 **/
	struct OracleAlignmentProperties : public Core::ReferenceCounted {
	    typedef std::vector<ConfusionNetwork::Arc> OracleAlignment;
	    OracleAlignment alignment;
	    ConstSemiringRef semiring;
	    OracleAlignmentProperties() {}
	};
	typedef Core::Ref<const OracleAlignmentProperties> ConstOracleAlignmentPropertiesRef;


	struct MapProperties : public Core::ReferenceCounted {
	    // Mapping might be invalid:
	    // - invalid aid means there is no corresponding arc
	    // - invalid sid means there is no corresponding state/slot; this implies invalid aid
	    struct Mapping {
		Fsa::StateId sid;
		Fsa::StateId aid;
		Mapping(Fsa::StateId sid, Fsa::StateId aid) : sid(sid), aid(aid) {}
	    };
	    static const Mapping InvalidMapping;
	    typedef std::vector<Mapping> Map;
	    mutable Map lat2cn;
	    mutable Map cn2lat;
	    mutable Core::Vector<Fsa::StateId> stateIndex;
	    mutable Core::Vector<Fsa::StateId> slotIndex;
	    // lattice-state+arc -> slot+arc
	    const Mapping & slotArc(Fsa::StateId stateId, Fsa::StateId aid) const;
	    // slot+arc -> lattice-state+arc
	    const Mapping & latticeArc(Fsa::StateId slotId, Fsa::StateId aid) const;
	    // iterator over all arcs in lattice state; lattice-state+arc -> slot+arc
	    Map::const_iterator state(Fsa::StateId stateId) const;
	    // iterator over all arcs in slot; slot+arc -> lattice-state+arc
	    Map::const_iterator slot(Fsa::StateId slotId) const;
	    // reduce to "lattice-state+arc -> slot+(invalid id)" mapping,
	    // i.e. discard any information about the slot arc
	    void reduce() const;
	    bool isReduced() const;
	};
	typedef Core::Ref<const MapProperties> ConstMapPropertiesRef;

    public:
	ConstSemiringRef semiring;
	ConstNormalizedPropertiesRef normalizedProperties;
	ConstNBestAlignmentPropertiesRef nBestAlignmentProperties;
	ConstOracleAlignmentPropertiesRef oracleAlignmentProperties;
	ConstMapPropertiesRef mapProperties;
    public:
	ConfusionNetwork() : Precursor() {}
	ConfusionNetwork(size_t n) : Precursor(n) {}
	ConfusionNetwork(size_t n, const Slot &slot) : Precursor(n, slot) {}

	bool isNormalized() const { return normalizedProperties; }
	bool isNBestAlignment() const { return (nBestAlignmentProperties); }
	bool isOracleAlignment() const { return (oracleAlignmentProperties); }
	bool hasMap() const { return (mapProperties); }
    };
    typedef Core::Vector<ConfusionNetwork> ConfusionNetworkList;
    typedef Core::Ref<ConfusionNetwork> ConfusionNetworkRef;
    typedef Core::Vector<ConfusionNetworkRef> ConfusionNetworkRefList;
    typedef Core::Ref<const ConfusionNetwork> ConstConfusionNetworkRef;
    typedef Core::Vector<ConstConfusionNetworkRef> ConstConfusionNetworkRefList;


    /**
     * Posterior Probability Confusion Network;
     * an arc stores only the label and the posterior probability of the label.
     *
     **/
    class PosteriorCn: public Cn::ConfusionNetwork<Cn::PosteriorArc> {
	typedef Cn::ConfusionNetwork<Cn::PosteriorArc> Precursor;
    public:
	PosteriorCn() : Precursor() {}
	PosteriorCn(size_t n) : Precursor(n) {}
	PosteriorCn(size_t n, const Slot &slot) : Precursor(n, slot) {}
	Probability score(size_t t, Fsa::LabelId label) const;
	void scores(ProbabilityList::iterator begin, ProbabilityList::iterator end, size_t t, Fsa::LabelId label) const;
    };
    typedef Core::Vector<PosteriorCn> PosteriorCnList;
    typedef Core::Ref<PosteriorCn> PosteriorCnRef;
    typedef Core::Vector<PosteriorCnRef> PosteriorCnRefList;
    typedef Core::Ref<const PosteriorCn> ConstPosteriorCnRef;
    typedef Core::Vector<ConstPosteriorCnRef> ConstPosteriorCnRefList;

} // namespace Flf

#endif // _FLF_CORE_LATTICE_HH
