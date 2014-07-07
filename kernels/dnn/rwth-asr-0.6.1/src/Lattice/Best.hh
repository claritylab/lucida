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
#ifndef _LATTICE_BEST_HH
#define _LATTICE_BEST_HH

#include <Core/Component.hh>
#include <Core/Parameter.hh>
#include <Fsa/Automaton.hh>
#include "Lattice.hh"

namespace Lattice {

    /**
     *  NBestListExtractor
     *  Produces an n-best list with word boundaries. All hypotheses in
     *  the n-best list are distinct w.r.t. the evaluation tokens.
     */
    class NBestListExtractor
    {
    private:
	Fsa::ConstAutomatonRef lemmaPronToLemma_;
	Fsa::ConstAutomatonRef lemmaToEval_;
	u32 targetNHypotheses_;
	f32 minThreshold_;
	f32 maxThreshold_;
	f32 thresholdIncrement_;
	bool workOnOutput_;
	bool latticeIsDeterministic_;
	bool hasFailArcs_;
	bool normalize_;
    private:
	Fsa::ConstAutomatonRef mapLemmaPronToEval(Fsa::ConstAutomatonRef) const;
	Fsa::ConstAutomatonRef mapEvalToLemmaPron(Fsa::ConstAutomatonRef) const;
	Fsa::ConstAutomatonRef getNBestListWithoutWordBoundaries(Fsa::ConstAutomatonRef, f32 &threshold);
    public:
	NBestListExtractor();
	virtual ~NBestListExtractor();

	ConstWordLatticeRef getNBestList(ConstWordLatticeRef);
	void initialize(Bliss::LexiconRef);
	void setNumberOfHypotheses(u32 nHypotheses) {
	    targetNHypotheses_ = nHypotheses;
	}
	void setMinPruningThreshold(f32 minThreshold) {
	    minThreshold_ = minThreshold;
	}
	void setMaxPruningThreshold(f32 maxThreshold) {
	    maxThreshold_ = maxThreshold;
	}
	void setPruningIncrement(f32 thresholdIncrement) {
	    thresholdIncrement_ = thresholdIncrement;
	}
	void setWorkOnOutput(bool workOnOutput) {
	    workOnOutput_ = workOnOutput;
	}
	void setLatticeIsDeterministic(bool isDeterministic) {
	    latticeIsDeterministic_ = isDeterministic;
	}
	void setHasFailArcs(bool hasFailArcs) {
		hasFailArcs_ = hasFailArcs;
	}
	void setNormalize(bool normalize) {
		normalize_ = normalize;
	}
    };

    ConstWordLatticeRef nbest(ConstWordLatticeRef l, u32 n);
    ConstWordLatticeRef best(ConstWordLatticeRef l, bool listFormat = false);

} // namespace Lattice

#endif // _LATTICE_BEST_HH
