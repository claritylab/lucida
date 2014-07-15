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
#ifndef _SPEECH_SEGMENT_NODE_HH
#define _SPEECH_SEGMENT_NODE_HH

#include "ModelCombination.hh"
#include <Flow/Node.hh>
#include <Am/AcousticModel.hh>

namespace Speech
{
    /** SegmentNode */
    class SegmentNode : public Flow::SleeveNode
    {
	typedef Flow::SleeveNode Precursor;
    private:
	static const Core::ParameterString paramSegmentId;
    protected:
	bool needInit_;
	std::string segmentId_;
    protected:
	const std::string& segmentId() const { return segmentId_; }
	virtual void initialize(ModelCombinationRef) {}
    public:
	SegmentNode(const Core::Configuration &);

	virtual bool setParameter(const std::string &name, const std::string &value);
	virtual bool configure();
	virtual bool work(Flow::PortId);
    };

    /** SegmentwiseFeaturesNode */
    class SegmentwiseFeaturesNode : public SegmentNode
    {
	typedef SegmentNode Precursor;
    private:
	Core::Ref<const Am::AcousticModel> acousticModel_;
	static const Core::ParameterBool paramNoDependencyCheck;
	const bool noDependencyCheck_;
    private:
	void checkFeatureDependencies(const Mm::Feature &) const;
    protected:
	virtual void initialize(ModelCombinationRef);
    public:
	static std::string filterName() { return "speech-segmentwise-features"; }
	SegmentwiseFeaturesNode(const Core::Configuration &);
	virtual Flow::PortId getInput(const std::string &name) {
	    return name == "model-combination" ? 0 : 1; }
	virtual bool configure();
	virtual bool work(Flow::PortId);
    };
}

#endif // _SPEECH_SEGMENT_NODE_HH
