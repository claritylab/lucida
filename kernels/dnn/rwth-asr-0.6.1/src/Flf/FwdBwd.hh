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
#ifndef _FLF_FWD_BWD_HH
#define _FLF_FWD_BWD_HH

#include <Core/ReferenceCounting.hh>

#include "FlfCore/Lattice.hh"
#include "Combination.hh"
#include "Network.hh"


namespace Flf {

    // -------------------------------------------------------------------------
    /*
      Build fwd./bwd. scores from lattice or union of lattices.
    */
    class FwdBwd;
    typedef Core::Ref<FwdBwd> FwdBwdRef;
    typedef Core::Ref<const FwdBwd> ConstFwdBwdRef;
    class FwdBwd : public Core::ReferenceCounted {
    private:
	class Builder;
	friend class Builder;
    public:
	class Arc {
	    friend class FwdBwd::Builder;
	public:
	    f64 arcScore;   // projected arc score
	    f64 fbScore;    // unormalized fwd/bwd score; from-state-fwd-score + arc-score + to-state-bwd-score
	    f64 normScore;  // normalization constant
	    f64 score() const { return fbScore - normScore; }
	    f64 probability() const { return ::exp(normScore - fbScore); }
	    Arc();
	};

	class State {
	    friend class FwdBwd::Builder;
	private:
	    Arc *begin_, *end_;
	public:
	    f64 fwdScore;   // unormalized fwd score
	    f64 bwdScore;   // unormalized bwd score
	    f64 normScore;  // normalization constant
	    f64 score() const { return fwdScore + bwdScore - normScore; }
	    f64 probability() const { return ::exp(normScore - fwdScore + bwdScore); }

	    typedef const Arc * const_iterator;
	    const_iterator begin() const { return begin_; }
	    const_iterator end()   const { return end_; }

	    State();
	};
    private:
	struct Internal;
	Internal *internal_;
    public:
	FwdBwd();
	~FwdBwd();
	// number of combined lattices and the used semirings
	s32 n() const;
	const ConstSemiringRefList & semirings() const;

	// Minimum fwd/bwd score found at a single arc
	f64 min() const;
	// Maximum fwd/bwd score found at a single arc
	f64 max() const;
	// Fwd. resp. bwd score over complete lattice
	f64 sum() const;

	const State & state(Fsa::StateId sid) const;
	const Arc & arc(ConstStateRef sr, Flf::State::const_iterator a) const
	    { return *(state(sr->id()).begin() + (a - sr->begin())); }

	/*
	  f64 score(Fsa::StateId sid) const
	  { return state(sid).score(); }
	  f64 probability(Fsa::StateId sid) const
	  { return state(sid).probability(); }

	  f64 score(ConstStateRef sr, State::const_iterator a) const
	  { return arc(sr, a).score(); }
	  f64 probability(ConstStateRef sr, State::const_iterator a) const
	  { return arc(sr, a).probability(); }
	*/

	// State::const_iterator begin(Fsa::StateId sid) const; // DEPR
	// State::const_iterator end(Fsa::StateId sid) const;

	/*
	  Factory methods:
	  - weights will be normalized; at least one lattice must have weight != 0.0
	  - alpha is ignored, if posteriorSemiring is defined
	*/
	/*
	  The resulting lattice is guaranteed to be equal (inclusive state numbering)
	  to the input lattice; the output is technically a static copy.
	*/
	struct Parameters {
	    Score alpha;
	    ConstSemiringRef posteriorSemiring;
	    ScoreId costId;
	    ScoreId scoreId;
	    ScoreId riskId;
	    bool normRisk;
	    Parameters();
	    void verifyConsistency() const;
	};
	static std::pair<ConstLatticeRef, ConstFwdBwdRef> build(
	    ConstLatticeRef l,
	    const FwdBwd::Parameters &params);
	static std::pair<ConstLatticeRef, ConstFwdBwdRef> build(
	    ConstLatticeRef l,
	    ConstSemiringRef posteriorSemiring = ConstSemiringRef());

	/*
	  The resulting lattice is the union of the input lattices;
	  a single lattice is not equal to the input lattice
	  as it gets a super-initial and a single super-final state
	*/
	struct CombinationParameters {
	    ScoreList weights;
	    ScoreList alphas;
	    ConstSemiringRefList posteriorSemirings;
	    Fsa::ConstAlphabetRef systemAlphabet;
	    LabelIdList systemLabels;
	    CombinationHelperRef combination;
	    ScoreId scoreId;
	    ScoreIdList normIds;
	    std::vector<bool> fsaNorms;
	    ScoreIdList weightIds;
	    bool setPosteriorSemiring;
	    CombinationParameters();
	    void verifyConsistency(u32 n) const;
	};
	static std::pair<ConstLatticeRef, ConstFwdBwdRef> build(
	    const ConstLatticeRefList &lats,
	    const FwdBwd::CombinationParameters &params);
	static std::pair<ConstLatticeRef, ConstFwdBwdRef> build(
	    const ConstLatticeRefList &lats,
	    const ScoreList &weights = ScoreList(),
	    const ConstSemiringRefList &posteriorSemirings = ConstSemiringRefList());
    };
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    /*
      Fwd./Bwd. score builder pre-configured by configuration

      [<selection>]
      configuration.channel  = nil
      statistics.channel     = nil

      # single system FB
      score.key              = <unset>
      risk.key               = <unset> # requires cost.key
      cost.key               = <unset>
      alpha                  = <1/max-scale>
      semiring               = <unset>

      # multiple system FB
      score-combination.type = discard|*concatenate\n"
      score.key              = <unset>
      system-labels          = false
      lattice-0.weight       = 1.0
      lattice-0.alpha        = <1/max-scale>
      lattice-0.semiring     = <unset>
      lattice-0.symbol       = <unset>
      lattice-0.norm.key     = <unset>
      lattice-1.weight       = 1.0
      lattice-1.alpha        = <1/max-scale>
      lattice-1.semiring     = <unset>
      lattice-1.symbol       = <unset>
      lattice-1.norm.key     = <unset>
      ...
    */
    class FwdBwdBuilder;
    typedef Core::Ref<FwdBwdBuilder> FwdBwdBuilderRef;
    class FwdBwdBuilder : public Core::ReferenceCounted {
    private:
	class Internal;
	Internal *internal_;
    private:
	FwdBwdBuilder();
	~FwdBwdBuilder();
    public:
	/*
	  See the build methods of the FwdBwd class.
	*/
	std::pair<ConstLatticeRef, ConstFwdBwdRef> build(ConstLatticeRef l);
	std::pair<ConstLatticeRef, ConstFwdBwdRef> build(const ConstLatticeRefList &lats);

	/*
	  Factory method:
	*/
	static FwdBwdBuilderRef create(const Core::Configuration &config,
				       const KeyList &extensionKeys = KeyList(), const ScoreList &extensionScales = ScoreList());
    };
    NodeRef createFwdBwdBuilderNode(const std::string &name, const Core::Configuration &config);
    // -------------------------------------------------------------------------

} // namespace Flf

#endif // _FLF_FWD_BWD_HH
