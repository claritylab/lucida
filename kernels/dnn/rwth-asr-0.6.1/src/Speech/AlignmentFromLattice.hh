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
#ifndef _SPEECH_ALIGNMENT_FROM_LATTICE_HH
#define _SPEECH_ALIGNMENT_FROM_LATTICE_HH

#include <Flow/Node.hh>
#include "ModelCombination.hh"
#include "PhonemeSequenceAlignmentGenerator.hh"

namespace Speech
{
    /** AlignmentFromLatticeNode */
    class AlignmentFromLatticeNode : public Flow::SleeveNode {
	typedef Flow::SleeveNode Precursor;
    public:
	static const Core::ParameterString paramSegmentId;
	static const Core::ParameterBool paramWriteAlphabet;

    private:
	std::string segmentId_;
	bool writeAlphabet_;
	bool needInit_;
	ModelCombinationRef modelCombination_;
	Core::Ref<PhonemeSequenceAlignmentGenerator> alignmentGenerator_;
    private:
	void initialize();
    public:
	static std::string filterName() { return "speech-alignment-from-lattice"; }
	AlignmentFromLatticeNode(const Core::Configuration&);
	virtual ~AlignmentFromLatticeNode() {}

	virtual Flow::PortId getInput(const std::string &name) {
	    return (name == "features") ? 1 : ((name == "model-combination") ? 2 : 0);
	}
	virtual bool setParameter(const std::string &name, const std::string &value);
	virtual bool configure();
	virtual bool work(Flow::PortId);
    };
}

#endif
