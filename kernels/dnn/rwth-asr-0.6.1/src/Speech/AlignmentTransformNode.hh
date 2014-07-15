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
#ifndef _SPEECH_ALIGNMENT_TRANSFORM_NODE_HH
#define _SPEECH_ALIGNMENT_TRANSFORM_NODE_HH

#include <Flow/Node.hh>
#include <Am/AcousticModel.hh>

namespace Speech
{

    class Alignment;

    /** AlignmentTransformNode.
     *  Derive from this class to encapsulate unary function computations on alignments in Flow.
     */
    class AlignmentTransformNode : public Flow::SleeveNode {
	typedef Flow::SleeveNode Precursor;
    protected:
	virtual void transform(Alignment &a) = 0;
    public:
	AlignmentTransformNode(const Core::Configuration &);
	virtual ~AlignmentTransformNode();
	virtual bool configure();
	virtual bool work(Flow::PortId);
    };

    /** Map the allophone alphabet from the stored alignment alphabet into the current one */
    class AlignmentMapAlphabet : public AlignmentTransformNode {
	    typedef AlignmentTransformNode Precursor;
    private:
	Core::Ref<const Am::AcousticModel> acousticModel_;
	bool forceMap_;
	bool skipMismatch_;

	virtual void transform(Alignment &a);
    public:
	static const Core::ParameterBool paramForceMap;
	static const Core::ParameterBool paramSkipMismatch;
	AlignmentMapAlphabet(const Core::Configuration &);
	static std::string filterName() { return "speech-alignment-map-alphabet"; }
    };

    /** AlignmentGammaCorrectionNode. @see Alignment::gammaCorrection() */
    class AlignmentGammaCorrectionNode : public AlignmentTransformNode {
	typedef AlignmentTransformNode Precursor;
    protected:
	static const Core::ParameterFloat paramExponent;
	static const Core::ParameterBool  paramNormalize;
	Mc::Scale exponent_;
	bool normalize_;
	virtual void transform(Alignment &a);
    public:
	AlignmentGammaCorrectionNode(const Core::Configuration &);
	static std::string filterName() { return "speech-alignment-gamma-correction"; }
	virtual bool setParameter(const std::string &name, const std::string &value);
    };

    /** AlignmentCombineItemsNode. @see Alignment::combineItems() */
    class AlignmentCombineItemsNode : public AlignmentTransformNode {
	typedef AlignmentTransformNode Precursor;
    private:
	static const Core::ParameterChoice paramSemiringType;
    private:
	Fsa::ConstSemiringRef semiring_;
    protected:
	virtual void transform(Alignment &a);
    public:
	AlignmentCombineItemsNode(const Core::Configuration &);
	static std::string filterName() { return "speech-alignment-combine-items"; }
    };

    /** AlignmentFilterWeightsNode. @see Alignment::filterWeights() */
    class AlignmentFilterWeightsNode : public AlignmentTransformNode {
	typedef AlignmentTransformNode Precursor;
    protected:
	static const Core::ParameterFloat paramMinWeight;
	static const Core::ParameterFloat paramMaxWeight;
	Mm::Weight minWeight_, maxWeight_;
	virtual void transform(Alignment &a);
    public:
	AlignmentFilterWeightsNode(const Core::Configuration &);
	static std::string filterName() { return "speech-alignment-clip-weights"; }
	virtual bool setParameter(const std::string &name, const std::string &value);
    };

    /** AlignmentResetWeightsNode */
    class AlignmentResetWeightsNode : public AlignmentTransformNode {
	    typedef AlignmentTransformNode Precursor;
    protected:
	enum Mode {
	    modeNone,
	    modeLargerThan,
	    modeSmallerThan,
	    modeTiedLargerThan,
	    modeTiedSmallerThan
	};
	static const Core::Choice choiceMode;
	static const Core::ParameterChoice paramMode;
	static const Core::ParameterFloat paramPreviousWeight;
	static const Core::ParameterFloat paramNewWeight;
	static const Core::ParameterBool paramRestrict;
	static const Core::ParameterStringVector paramSelectPhones;
	Core::Ref<const Am::AcousticModel> acousticModel_;

	Mm::Weight previousWeight_, newWeight_;
	s32 mode_;
	bool restrict_;
	std::set<Bliss::Phoneme::Id> selectPhones_;
	virtual void transform(Alignment &a);
    public:
	AlignmentResetWeightsNode(const Core::Configuration &);
	static std::string filterName() { return "speech-alignment-reset-weights"; }
	virtual bool setParameter(const std::string &name, const std::string &value);
    };

    /** BinaryAlignmentTransformNode.
     *  Derive from this class to encapsulate binary function computations on alignments in Flow.
     */
    class BinaryAlignmentTransformNode : public Flow::SleeveNode {
	typedef Flow::SleeveNode Precursor;
    protected:
	virtual void transform(Alignment &a1, Alignment &a2) = 0;
    public:
	BinaryAlignmentTransformNode(const Core::Configuration &);
	virtual ~BinaryAlignmentTransformNode();
	virtual bool configure();
	virtual bool work(Flow::PortId);
    };

    /** AlignmentMultiplyAlignmentsNode.
     *  Component-wise multiplication of weights of two alignments.
     */
    class AlignmentMultiplyAlignmentsNode : public BinaryAlignmentTransformNode {
	typedef BinaryAlignmentTransformNode Precursor;
    protected:
	virtual void transform(Alignment &a1, Alignment &a2);
    public:
	AlignmentMultiplyAlignmentsNode(const Core::Configuration &);
	virtual ~AlignmentMultiplyAlignmentsNode();
	static std::string filterName() { return "speech-alignment-multiply-alignments"; }
	virtual Flow::PortId getInput(const std::string &name) {
	    return name == "left" ? 0 : 1; }
    };

    /** AlignmentMultiplyWeightsNode. @see Alignment::multiplyWeights() */
    class AlignmentMultiplyWeightsNode : public AlignmentTransformNode {
	typedef AlignmentTransformNode Precursor;
    protected:
	static const Core::ParameterFloat paramFactor;
	Mm::Weight factor_;
	virtual void transform(Alignment &a);
    public:
	AlignmentMultiplyWeightsNode(const Core::Configuration &);
	static std::string filterName() { return "speech-alignment-multiply-weights"; }
	virtual bool setParameter(const std::string &name, const std::string &value);
    };

    /** AlignmentAddWeightNode. @see Alignment::addWeight() */
    class AlignmentAddWeightNode : public AlignmentTransformNode {
	typedef AlignmentTransformNode Precursor;
    protected:
	static const Core::ParameterFloat paramOffset;
	Mm::Weight offset_;
	virtual void transform(Alignment &a);
    public:
	AlignmentAddWeightNode(const Core::Configuration &);
	static std::string filterName() { return "speech-alignment-add-weight"; }
	virtual bool setParameter(const std::string &name, const std::string &value);
    };

    /** AlignmentExpmNode. @see Alignment::expm() */
    class AlignmentExpmNode : public AlignmentTransformNode {
	typedef AlignmentTransformNode Precursor;
    protected:
	virtual void transform(Alignment &a);
    public:
	AlignmentExpmNode(const Core::Configuration &);
	static std::string filterName() { return "speech-alignment-expm"; }
    };


    /** AlignmentRemoveEmissionScoreNode. */
    class AlignmentRemoveEmissionScoreNode : public AlignmentTransformNode {
	typedef AlignmentTransformNode Precursor;
    private:
	bool needInit_;
	Core::Ref<Am::AcousticModel> acousticModel_;
    private:
	void checkFeatureDependencies(const Mm::Feature &) const;
    protected:
	virtual void transform(Alignment &a);
    public:
	AlignmentRemoveEmissionScoreNode(const Core::Configuration &);
	static std::string filterName() { return "speech-alignment-remove-emission-score"; }
	virtual Flow::PortId getInput(const std::string &name) {
	    return (name == "features") ? 1 : ((name == "model-combination") ? 2 : 0); }
	virtual bool configure();
	virtual bool work(Flow::PortId p);
    };

    /** SetAlignmentWeightsByTiedStateAlignmentWeightsNode */
    class SetAlignmentWeightsByTiedStateAlignmentWeightsNode : public Flow::SleeveNode {
	typedef Flow::SleeveNode Precursor;
    protected:
	Core::Ref<const Am::AcousticModel> acousticModel_;
    public:
	static std::string filterName() { return "alignment-weights-by-tied-state-alignment-weights"; }
	SetAlignmentWeightsByTiedStateAlignmentWeightsNode(const Core::Configuration&);
	virtual ~SetAlignmentWeightsByTiedStateAlignmentWeightsNode() {}

	virtual Flow::PortId getInput(const std::string &name) { return name == "target" ? 1 : 0; }
	virtual bool setParameter(const std::string &name, const std::string &value) { return false; }
		virtual bool configure();
	virtual bool work(Flow::PortId);
    };

}

#endif // _SPEECH_ALIGNMENT_TRANSFORM_NODE_HH
