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
#ifndef _SPEECH_ALIGNMENT_NODE_HH
#define _SPEECH_ALIGNMENT_NODE_HH

#include "Alignment.hh"
#include <Core/Archive.hh>
#include <Search/Aligner.hh>
#include <Lattice/Lattice.hh>
#include "ModelCombination.hh"

namespace Lattice {
    class ArchiveWriter;
    class ArchiveReader;
}

namespace Speech {

    class CombinedAcousticLatticeRescorer;
    class CombinedLmLatticeRescorer;
    class FsaCache;

    /** AlignmentBaseNode */
    class AlignmentBaseNode : public Flow::SleeveNode {
	typedef Flow::SleeveNode Precursor;
    public:
	static const Core::ParameterString paramSegmentId;
	static const Core::ParameterString paramOrthography;
    protected:
	std::string segmentId_;
	std::string orthography_;
	AllophoneStateGraphBuilder *allophoneStateGraphBuilder_;
	Fsa::ConstAutomatonRef lemmaPronunciationToLemma_;
	FsaCache *modelCache_;
	bool needInit_;
    protected:
	virtual void initialize() {}
	virtual void createModel() = 0;
	bool configure(const Flow::Datatype *);
    public:
	AlignmentBaseNode(const Core::Configuration&);
	virtual ~AlignmentBaseNode();

	virtual bool setParameter(const std::string &name, const std::string &value);
	virtual bool configure();
    };

    /** AlignmentNode */
    class AlignmentNode : public AlignmentBaseNode {
	typedef AlignmentBaseNode Precursor;
    public:
	static const Core::ParameterBool paramStoreLattices;
	static const Core::ParameterBool paramUseTracebacks;
	static const Core::ParameterBool paramWriteAlphabet;
	enum PhonemeSequenceSet {
	    lemmaLoop,
	    phoneLoop,
	    orthography
	};
	static const Core::Choice choicePhonemeSequenceSet;
	static const Core::ParameterChoice paramPhonemeSequenceSet;
	static const Core::ParameterBool paramNoDependencyCheck;
    private:
	Search::Aligner::WordLatticeBuilder *wordLatticeBuilder_;
	PhonemeSequenceSet phonemeSequenceSet_;
	const bool noDependencyCheck_;
    protected:
	Core::Ref<Am::AcousticModel> acousticModel_;
	mutable Core::XmlChannel tracebackChannel_;
	std::vector<Flow::Timestamp> featureTimes_;
	Lattice::ArchiveWriter *latticeArchiveWriter_;
	Lattice::ArchiveReader *tracebackArchiveReader_;
	FsaCache *transducerCache_;
	Search::Aligner aligner_;
	bool writeAlphabet_;
    protected:
	virtual void initialize();
	virtual void createModel();
	void checkFeatureDependencies(const Mm::Feature&) const;
	void logTraceback(Lattice::ConstWordLatticeRef) const;
	void createWordLattice(Fsa::ConstAutomatonRef alignmentFsa) const;
    public:
	static std::string filterName() { return "speech-alignment"; }
	AlignmentNode(const Core::Configuration&);
	virtual ~AlignmentNode();

	virtual bool configure();
	virtual bool work(Flow::PortId);
    };

    /** Dumps alignments in a plain text format */
    class AlignmentDumpNode : public Flow::Node {
	typedef Flow::Node Precursor;
    public:
	static const Core::ParameterString paramFilename;
	static const Core::ParameterString paramSegmentId;
    private:
	typedef std::pair<Flow::Time, Flow::Time> FeatureTime;
	typedef std::vector<FeatureTime> FeatureTimes;
	FeatureTimes featureTimes_;
	Core::Archive *archive_;
	//Core::Ref<Flow::Attributes> attributes_;
	Core::ArchiveWriter *writer_;
	Core::ArchiveReader *reader_;
	bool archiveExists_;
    protected:
	std::string filename_;
	std::string segmentId_;
	Core::Ref<const Am::AcousticModel> acousticModel_;
	enum AlignmentType { standard, plainText };
	AlignmentType alignmentType_;
	Core::StringHashMap<std::string> parameters_;
	//f64 sampleRate_;
	Flow::Attributes::Parser attributesParser_;
    private:
	bool createContext(const std::string &id);
    protected:
	bool hasParameters(const std::string &s);
    public:
	AlignmentDumpNode(const Core::Configuration &);
	virtual ~AlignmentDumpNode();
	static std::string filterName() { return "speech-alignment-dump"; }
	virtual Flow::PortId getInput(const std::string &name) { return name == "features" ? 1 : 0; }
	virtual Flow::PortId getOutput(const std::string &name) { return 0; }
	Core::ArchiveWriter* newWriter(const std::string &name);
	Core::ArchiveReader* newReader(const std::string &name);
	bool hasAccess(Core::Archive::AccessMode a) const {
	    return (archive_) ? archive_->hasAccess(a) : false;
	}
	bool open(Core::Archive::AccessMode access);
	void close();
	bool isOpen() const { return (archive_ != 0); }
	virtual bool configure();
	virtual bool setParameter(const std::string &name, const std::string &value);
	virtual bool work(Flow::PortId);
    };

} // namespace Speech

#endif // _SPEECH_ALIGNMENT_NODE_HH
