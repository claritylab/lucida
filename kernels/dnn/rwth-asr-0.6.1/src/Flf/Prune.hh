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
#ifndef _FLF_PRUNE_HH
#define _FLF_PRUNE_HH

#include "FlfCore/Lattice.hh"
#include "FwdBwd.hh"
#include "Network.hh"
#include "TimeframeConfusionNetwork.hh"

namespace Flf {

    /*
      Prune arc, if fwd/bwd-score is greater than the given threshold;
      in the "relative" mode the threshold is interpreted relative to min. fb-score
    */
    ConstLatticeRef pruneByFwdBwdScores(ConstLatticeRef l, ConstFwdBwdRef fb, Score threshold, f32 minArcsPerSecond = 0.0, f32 maxArcsPerSecond = Core::Type<f32>::max, s32 maxArcsPerSegment = Core::Type<s32>::max);

    /*
      Prune lattice by fwd./bwd. score

      [<selection>]
      statistics.channel  = nil
      relative            = true
      as-probability      = false
      threshold           = inf
      [<selection>.fb]
      ... (see FwdBwd.hh for single lattice)
    */
    class FwdBwdPruner;
    typedef Core::Ref<FwdBwdPruner> FwdBwdPrunerRef;
    class FwdBwdPruner : public Core::ReferenceCounted {
    private:
	class Internal;
	Internal *internal_;
    private:
	FwdBwdPruner();
	~FwdBwdPruner();
    public:
	ConstLatticeRef prune(ConstLatticeRef l, bool trim = false);
	ConstLatticeRef prune(ConstLatticeRef l, ConstFwdBwdRef fb, bool trim = false);

	/*
	  Factory method:
	*/
	static FwdBwdPrunerRef create(const Core::Configuration &config, FwdBwdBuilderRef fbBuilder = FwdBwdBuilderRef());
    };
    NodeRef createFwdBwdPruningNode(const std::string &name, const Core::Configuration &config);



    /*
      Prune CN or fCN slot-wise.
      Pruning is done in situ.

      Only normalized CNs can be pruned.
      Arcs with scores[posteriorId](resp. probability) >= threshold are kept.
      The first maxSlotSize arcs are kept, where arcs are sorted by scores[posteriorId](resp. probability).
      Re-normalize the posterior probability distribution on request.
    */
    void prune(ConstConfusionNetworkRef cn, Score threshold, u32 maxSlotSize = Core::Type<u32>::max, bool normalize = true);
    void prune(ConstPosteriorCnRef cn, Score threshold, u32 maxSlotSize = Core::Type<u32>::max, bool normalize = true);

    /*
      Remove slots from CN or fCN, if the slot is dominated by the epsilon arc.
      Removal is done in situ.

      Only for normalized CNs the epsilon slot removal works.
      If the slot contains only a single, epsilon arc or if the posterior probability of the epsilon arc exceeds
      threshold, then the slot is removed.
    */
    void removeEpsSlots(ConstConfusionNetworkRef cnRef, Score threshold = Core::Type<Score>::max);
    void removeEpsSlots(ConstPosteriorCnRef cnRef, Score threshold = Core::Type<Score>::max);

    NodeRef createNormalizedCnPruningNode(const std::string &name, const Core::Configuration &config);
    NodeRef createPosteriorCnPruningNode(const std::string &name, const Core::Configuration &config);

} // namespace Flf

#endif // _FLF_PRUNE_HH
