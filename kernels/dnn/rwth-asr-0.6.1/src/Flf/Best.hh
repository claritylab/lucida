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
#ifndef _FLF_BEST_HH
#define _FLF_BEST_HH

#include "FlfCore/Lattice.hh"
#include "Network.hh"

namespace Flf {

    /**
     * Dijkstra:
     *   - aka Ftl::firstBest
     *   - extend path scores, find path with minimum w.r.t. to projection of extended path scores
     *   - uses priority queue to follow currently best path
     * Bellman-Ford:
     *   - aka Ftl::best
     *   - extend path scores, collect extended path scores
     *   - general single source shortest path (SSSP) algorithm
     * Projecting-Bellman-Ford:
     *   - project scores, sum projected scores, find path with minimum sum of projected path scores
     * For the tropical semiring and if the projection of each score is positive,
     * then all three algorithms yield the same result.
     **/
    typedef enum {
	Dijkstra,
	BellmanFord,
	ProjectingBellmanFord,
    } SingleSourceShortestPathAlgorithm;
    ConstLatticeRef best(ConstLatticeRef l, SingleSourceShortestPathAlgorithm = Dijkstra);

    void dumpBest(ConstLatticeRef l, std::ostream &os);

    NodeRef createBestNode(const std::string &name, const Core::Configuration &config);


    // equal to best(l, ProjectedSumBellmanFord)
    std::pair<ConstLatticeRef, Score> bestProjection(ConstLatticeRef l);


    /**
     * Minimal score (sum of projected weights) for all state pairs in lattice.
     * Invalid score, if no connection between two states.
     **/
    class AllPairsShortestDistance;
    typedef Core::Ref<const AllPairsShortestDistance> ConstAllPairsShortestDistanceRef;
    class AllPairsShortestDistance : public Core::ReferenceCounted {
    public:
	struct Distance {
	    Fsa::StateId from;
	    Fsa::StateId to;
	    Score score;
	};
	typedef Distance const * const_iterator;
    private:
	class Internal;
	const Internal *internal_;
	AllPairsShortestDistance(const Internal *internal);
    public:
	~AllPairsShortestDistance();
	// distance
	Score get(Fsa::StateId from, Fsa::StateId to) const;
	// all distances sorted by state id
	const_iterator begin() const;
	const_iterator end() const;
	static ConstAllPairsShortestDistanceRef create(ConstLatticeRef l, Time timeThreshold = Core::Type<Time>::max);
    };

    /**
     * Format:
     * # from     to        score
     * <from-id>  <to-id>   <score>
     * ...
     * Output is sorted by from/to in increasing order
     **/
    NodeRef createDumpAllPairsShortestDistanceNode(const std::string &name, const Core::Configuration &config);

} // namespace Flf

#endif // _FLF_BEST_HH
