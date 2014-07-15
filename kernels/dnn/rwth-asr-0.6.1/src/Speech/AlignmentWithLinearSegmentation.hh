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
#ifndef _SPEECH_ALIGNMENT_WITH_LINEAR_SEGMENTATION_HH
#define _SPEECH_ALIGNMENT_WITH_LINEAR_SEGMENTATION_HH

#include "AlignmentNode.hh"

namespace Speech {

    class LinearSegmenter : public Core::Component
    {
    public:
	static const Core::ParameterInt paramMinimumSegmentLength;
	static const Core::ParameterInt paramMaximumSegmentLength;
	static const Core::ParameterBool paramShouldUseSietill;
    public:
	class Delimiter;
    private:
	Delimiter *delimiter_;
	Am::AllophoneStateIndex silence_;
	std::vector<Am::AllophoneStateIndex> states_;
	u32 minimumSegmentLength_, maximumSegmentLength_;
	const u32 maximumJump_;
    public:
	LinearSegmenter(const Core::Configuration&);
	virtual ~LinearSegmenter();

	void setSilence(Am::AllophoneStateIndex silence);
	void setModel(Fsa::ConstAutomatonRef);
	void feed(f32);
	bool isAlignmentValid() const;
	bool getAlignment(Alignment&) const;
	u32 nFeatures() const;
    };

    class AlignmentWithLinearSegmentationNode : public AlignmentBaseNode {
	typedef AlignmentBaseNode Precursor;
    private:
	LinearSegmenter segmenter_;
    protected:
	virtual void initialize();
	virtual void createModel();
    public:
	static std::string filterName() { return "speech-linear-segmentation"; }
	AlignmentWithLinearSegmentationNode(const Core::Configuration&);
	virtual ~AlignmentWithLinearSegmentationNode() {}

	virtual bool configure();
	virtual bool work(Flow::PortId);
    };

}

#endif // _SPEECH_ALIGNMENT_WITH_LINEAR_SEGMENTATION_HH
