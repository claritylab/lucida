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
#ifndef _SPEECH_PRUNING_LATTICE_SET_NODE
#define _SPEECH_PRUNING_LATTICE_SET_NODE

#include "LatticeSetProcessor.hh"

namespace Speech {

    /**
     * pruning node
     */
    class PruningLatticeSetNode : public LatticeSetProcessor
    {
	typedef LatticeSetProcessor Precursor;
    private:
	static const Core::ParameterFloat paramThreshold;
	static const Core::ParameterBool paramThresholdIsRelative;
	static const Core::ParameterBool paramHasFailArcs;
	Fsa::Weight threshold_;
	bool thresholdIsRelative_;
	bool hasFailArcs_;
	enum PruningType { forwardBackward, forward };
	static Core::Choice choicePruningType;
	static const Core::ParameterChoice paramPruningType;
	PruningType pruningType_;
    public:
	PruningLatticeSetNode(const Core::Configuration &);
	virtual ~PruningLatticeSetNode();

	virtual void processWordLattice(Lattice::ConstWordLatticeRef, Bliss::SpeechSegment *);
    };

}

#endif // _SPEECH_PRUNING_LATTICE_SET_NODE
