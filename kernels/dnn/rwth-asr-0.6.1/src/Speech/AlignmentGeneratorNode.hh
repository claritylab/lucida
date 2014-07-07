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
#ifndef _SPEECH_ALIGNMENT_GENERATOR_NODE_HH
#define _SPEECH_ALIGNMENT_GENERATOR_NODE_HH

#include "SegmentNode.hh"
#include "PhonemeSequenceAlignmentGenerator.hh"

namespace Speech
{

    /** AlignmentGeneratorNode */
    class AlignmentGeneratorNode : public SegmentNode
    {
	typedef SegmentNode Precursor;
    private:
	AlignmentGeneratorRef alignmentGenerator_;
    protected:
	virtual void initialize(ModelCombinationRef);
    public:
	static std::string filterName() { return "speech-lattice-alignment-generator"; }
	AlignmentGeneratorNode(const Core::Configuration &);
	virtual Flow::PortId getInput(const std::string &name) {
	    return name == "model-combination" ? 0 : 1; }
	virtual bool configure();
	virtual bool work(Flow::PortId);
    };


}

#endif // _SPEECH_ALIGNMENT_GENERATOR_NODE_HH
