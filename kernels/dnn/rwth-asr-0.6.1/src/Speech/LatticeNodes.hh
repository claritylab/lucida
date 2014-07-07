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
#ifndef _SPEECH_LATTICE_NODES_HH
#define _SPEECH_LATTICE_NODES_HH

#include <Core/Archive.hh>
#include <Flow/DataAdaptor.hh>
#include <Flow/Node.hh>
#include <Lattice/Lattice.hh>
#include <Flf/FlfCore/Lattice.hh>
#include "ModelCombination.hh"
#include <Core/Parameter.hh>

namespace Core {

    template <>
    class NameHelper<Flf::ConstLatticeRef> : public std::string {
    public:
	NameHelper() : std::string("flow-const-lattice-ref") {}
    };

} // namespace Core

namespace Lattice {

    class ArchiveWriter;
    class ArchiveReader;

}

namespace Speech {

    Lattice::ConstWordLatticeRef toWordLattice(Flf::ConstLatticeRef);
    Flf::ConstLatticeRef fromWordLattice(Lattice::ConstWordLatticeRef);

    /** ModelCombinationNode */
    class ModelCombinationNode : public Flow::SourceNode {
	typedef Flow::SourceNode Precursor;
    public:
	static Core::Choice choiceMode;
	static Core::ParameterChoice paramMode;
	static Core::Choice choiceAcousticModelMode;
	static Core::ParameterChoice paramAcousticModelMode;
    private:
	ModelCombination::Mode mode_;
	Am::AcousticModel::Mode acousticModelMode_;
	ModelCombinationRef modelCombination_;
	Flow::DataAdaptor<ModelCombinationRef> *out_;
	bool needInit_;
	void initialize();
    public:
	static std::string filterName() { return "model-combination"; }
	ModelCombinationNode(const Core::Configuration&);

	virtual bool setParameter(const std::string &name, const std::string &value) { return false;}
	virtual bool configure();
	virtual bool work(Flow::PortId);
    };

    /** LatticeIoNode */
    class LatticeIoNode : public Flow::SleeveNode {
	typedef Flow::SleeveNode Precursor;
    private:
	static const Core::ParameterString paramSegmentId;
    protected:
	bool needInit_;
	std::string segmentId_;
    protected:
	virtual void initialize(ModelCombinationRef) {}
    public:
	LatticeIoNode(const Core::Configuration&);

	virtual bool setParameter(const std::string &name, const std::string &value);
	virtual bool configure();
	virtual bool work(Flow::PortId);
    };

    /** LatticeReadNode */
    class LatticeReadNode : public LatticeIoNode {
	typedef LatticeIoNode Precursor;
	typedef std::vector<std::string> LatticeReaders;
    public:
	static const Core::ParameterStringVector paramReaders;
    private:
	LatticeReaders readers_;
	Lattice::ArchiveReader *latticeArchiveReader_;
    private:
	virtual void initialize(ModelCombinationRef);
    public:
	static std::string filterName() { return "lattice-read"; }
	LatticeReadNode(const Core::Configuration&);
	virtual ~LatticeReadNode();

	virtual bool work(Flow::PortId);
    };

    /** LatticeWriteNode */
    class LatticeWriteNode : public LatticeIoNode {
	typedef LatticeIoNode Precursor;
    private:
	Lattice::ArchiveWriter *latticeArchiveWriter_;
    private:
	virtual void initialize(ModelCombinationRef);
    public:
	static std::string filterName() { return "lattice-write"; }
	LatticeWriteNode(const Core::Configuration&);
	virtual ~LatticeWriteNode();
	virtual Flow::PortId getInput(const std::string &name) {
	    return name == "model-combination" ? 0 : 1;
	}
	virtual bool configure();
	virtual bool work(Flow::PortId);
    };

    /** LatticeSemiringNode */
    class LatticeSemiringNode : public Flow::SleeveNode {
	typedef Flow::SleeveNode Precursor;
    private:
	Flf::ConstSemiringRef semiring_;
    public:
	static std::string filterName() { return "lattice-semiring"; }
	LatticeSemiringNode(const Core::Configuration&);
	virtual ~LatticeSemiringNode() {}

	virtual bool setParameter(const std::string &name, const std::string &value) { return false; }
	virtual bool configure();
	virtual bool work(Flow::PortId);
    };

    /** LatticeSimpleModifyNode */
    class LatticeSimpleModifyNode : public Flow::SleeveNode {
	typedef Flow::SleeveNode Precursor;
    public:
	static const Core::ParameterFloatVector paramScales;
    private:
	Flf::ScoreList scales_;
    public:
	static std::string filterName() { return "lattice-simple-modification"; }
	LatticeSimpleModifyNode(const Core::Configuration&);
	virtual ~LatticeSimpleModifyNode() {}

	virtual bool setParameter(const std::string &name, const std::string &value);
	virtual bool configure();
	virtual bool work(Flow::PortId);
    };

    /** LatticeTransformNode
     *  Base node for unary operations on lattices,
     *  preserving the topology.
     */
    class LatticeTransformNode : public Flow::SleeveNode {
	typedef Flow::SleeveNode Precursor;
    private:
	static const Core::ParameterBool paramAppendScore;
    protected:
	bool appendScore_;
    protected:
	virtual Fsa::ConstAutomatonRef transform(Fsa::ConstAutomatonRef fsa) const { return fsa; }
    public:
	LatticeTransformNode(const Core::Configuration &);
	virtual bool configure();
	virtual bool work(Flow::PortId);
    };

    /** LatticeTransform2Node
     *  Base node for binary operations on lattices,
     *  preserving the topology.
     */
    class LatticeTransform2Node : public LatticeTransformNode {
	typedef LatticeTransformNode Precursor;
    protected:
	virtual Fsa::ConstAutomatonRef transform(Fsa::ConstAutomatonRef fsa1, Fsa::ConstAutomatonRef) const { return fsa1; }
    public:
	LatticeTransform2Node(const Core::Configuration &);
	virtual Flow::PortId getInput(const std::string &name) {
	    return name == "lattice-2" ? 1 : 0; }
	virtual bool configure();
	virtual bool work(Flow::PortId);
    };

    /** LatticeWordPosteriorNode */
    class LatticeWordPosteriorNode : public LatticeTransformNode {
	typedef LatticeTransformNode Precursor;
    protected:
	virtual Fsa::ConstAutomatonRef transform(Fsa::ConstAutomatonRef) const;
    public:
	static std::string filterName() { return "lattice-word-posterior"; }
	LatticeWordPosteriorNode(const Core::Configuration&);
    };

    /** LatticeCopyNode */
    class LatticeCopyNode : public LatticeTransformNode {
	typedef LatticeTransformNode Precursor;
    protected:
	virtual Fsa::ConstAutomatonRef transform(Fsa::ConstAutomatonRef) const;
    public:
	static std::string filterName() { return "lattice-copy"; }
	LatticeCopyNode(const Core::Configuration&);
    };

    /** LatticeCacheNode */
    class LatticeCacheNode : public LatticeTransformNode {
	typedef LatticeTransformNode Precursor;
    protected:
	virtual Fsa::ConstAutomatonRef transform(Fsa::ConstAutomatonRef) const;
    public:
	static std::string filterName() { return "lattice-cache"; }
	LatticeCacheNode(const Core::Configuration&);
    };

    /** LatticeExpmNode */
    class LatticeExpmNode : public LatticeTransformNode {
	typedef LatticeTransformNode Precursor;
    protected:
	virtual Fsa::ConstAutomatonRef transform(Fsa::ConstAutomatonRef) const;
    public:
	static std::string filterName() { return "lattice-expm"; }
	LatticeExpmNode(const Core::Configuration&);
    };

    /** LatticeExpectationPosteriorNode */
    class LatticeExpectationPosteriorNode : public LatticeTransform2Node {
	typedef LatticeTransform2Node Precursor;
    private:
	bool normalize_;
    protected:
	virtual Fsa::ConstAutomatonRef transform(Fsa::ConstAutomatonRef, Fsa::ConstAutomatonRef) const;
    public:
	static const Core::ParameterBool paramNormalize;
	static std::string filterName() { return "lattice-expectation-posterior"; }
	LatticeExpectationPosteriorNode(const Core::Configuration&);
    };

     /** LatticeNBestNode */
     class LatticeNBestNode : public Flow::SleeveNode {
	typedef Flow::SleeveNode Precursor;
     public:
	static std::string filterName() { return "lattice-nbest"; }
	LatticeNBestNode(const Core::Configuration&);
	virtual ~LatticeNBestNode() {}

	virtual bool setParameter(const std::string &name, const std::string &value) { return false; }
	virtual bool configure();
	virtual bool work(Flow::PortId);
     };

    /** LatticeDumpCtmNode */
    class LatticeDumpCtmNode : public Flow::SleeveNode {
	typedef Flow::SleeveNode Precursor;
    private:
	Core::Channel ctmChannel_;
	static const Core::ParameterString paramSegmentId;
	static const Core::ParameterString paramTrackId;
    protected:
	std::string segmentId_;
	std::string trackId_;
    public:
	static std::string filterName() { return "lattice-dump-ctm"; }
	LatticeDumpCtmNode(const Core::Configuration&);
	virtual ~LatticeDumpCtmNode() {}

	virtual Flow::PortId getInput(const std::string &name) { return name == "features" ? 1 : 0; }
	virtual bool setParameter(const std::string &name, const std::string &value);
	virtual bool configure();
	virtual bool work(Flow::PortId);
    };

} // namespace Speech

#endif // _SPEECH_LATTICE_NODES_HH
