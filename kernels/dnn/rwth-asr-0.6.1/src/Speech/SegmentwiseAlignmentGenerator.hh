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
#ifndef _SPEECH_SEGMENTWISE_ALIGNMENT_GENERATOR_HH
#define _SPEECH_SEGMENTWISE_ALIGNMENT_GENERATOR_HH

#include "Alignment.hh"
#include "ModelCombination.hh"
#include <Search/Aligner.hh>
#include "SegmentwiseFeatureExtractor.hh"
#include <Core/Archive.hh>
#include <Core/Component.hh>
#include <Core/Dependency.hh>
#include <Core/ReferenceCounting.hh>
#include <Core/Hash.hh>
#include <Fsa/Automaton.hh>
#include <Lattice/Lattice.hh>

namespace Am {
    class AcousticModel;
}

namespace Bliss {
    class SpeechSegment;
}

namespace Core {
    class Archive;
}

namespace Lattice {
    class ArchiveWriter;
}

namespace Speech {
    class FsaCache;
}

namespace Speech {

    /** SegmentwiseAlignmentGenerator */
    class SegmentwiseAlignmentGenerator : public Core::Component,
					  public Core::ReferenceCounted
    {
    private:
	static const Core::ParameterString paramPortName;
	static const Core::ParameterStringVector paramSilencesAndNoises;
	static const Core::ParameterBool paramCheckCompatibility;
    protected:
	Flow::PortId portId_;
	Core::Ref<SegmentwiseFeatureExtractor> segmentwiseFeatureExtractor_;
	ConstSegmentwiseFeaturesRef  features_;
	Core::Ref<const ModelCombination> modelCombination_;
	AllophoneStateGraphBuilder *allophoneStateGraphBuilder_;
	Search::Aligner aligner_;
	Core::DependencySet dependencySet_;
	bool checkCompatibility_;
    protected:
	FsaCache * createFsaCache(const Core::Configuration &config);
	void getScorers(std::vector<Mm::FeatureScorer::Scorer> &) const;
	void getScorers(TimeframeIndex tbeg, TimeframeIndex tend,
			std::vector<Mm::FeatureScorer::Scorer> &) const;
	void update(const std::string &segmentId);
    public:
	SegmentwiseAlignmentGenerator(const Core::Configuration &, Core::Ref<const ModelCombination> = Core::Ref<const ModelCombination>());

	virtual ~SegmentwiseAlignmentGenerator();

	/**
	 * This segmentwise feature extractor provides the features, if set.
	 * Otherwise the features are set by setFeatures, see below.
	 */
	virtual void setSegmentwiseFeatureExtractor(Core::Ref<SegmentwiseFeatureExtractor>);
	/**
	 * The features can be also set directly. This, however, is only
	 * possible if @param segmentwiseFeatureExtractor_ is not set.
	 */
	void setFeatures(ConstSegmentwiseFeaturesRef features)
	    { require(features); features_ = features; }
	virtual const ConstSegmentwiseFeaturesRef features() const
	    { return segmentwiseFeatureExtractor_ ? segmentwiseFeatureExtractor_->features(portId_) : features_; }

	// call either or
	virtual void setSpeechSegmentId(const std::string &);
	virtual void setSpeechSegment(Bliss::SpeechSegment *);

	virtual void signOn(Speech::CorpusVisitor &corpusVisitor);

	Core::Ref<const ModelCombination> modelCombination() const { return modelCombination_; }
	Core::Ref<Am::AcousticModel> acousticModel() const { return modelCombination_->acousticModel(); }
	Fsa::ConstAlphabetRef allophoneStateAlphabet() const { return modelCombination_->acousticModel()->allophoneStateAlphabet(); }

	const Core::DependencySet & dependencies() const { return dependencySet_; }

	AllophoneStateGraphBuilder *allophoneStateGraphBuilder() { return allophoneStateGraphBuilder_; }
    };


    /** OrthographyAlignmentGenerator */
    class OrthographyAlignmentGenerator : public SegmentwiseAlignmentGenerator
    {
	typedef SegmentwiseAlignmentGenerator Precursor;
    protected:
	Core::Archive * createAlignmentCache(const Core::Configuration &config);
	Lattice::ArchiveWriter * createLatticeArchive(const Core::Configuration &config);

	Fsa::ConstAutomatonRef getAlignmentFsa();

	void update(const std::string &segmentId, const std::string &orthography);
	void setOrthography(const std::string &orthography);

    protected:
	FsaCache *transducerCache_;
	FsaCache *modelAcceptorCache_;
	Search::Aligner::WordLatticeBuilder *wordLatticeBuilder_;
	Fsa::ConstAutomatonRef lemmaPronunciationToLemma_;

	Core::Archive *alignmentCache_;

	std::string segmentId_;
	std::string orthography_;
	Fsa::ConstAutomatonRef alignmentFsa_;
	bool isValid_;

    public:
	OrthographyAlignmentGenerator(const Core::Configuration &, Core::Ref<const ModelCombination> = Core::Ref<const ModelCombination>());
	virtual ~OrthographyAlignmentGenerator();

	// call either or
	virtual void setSpeechSegmentId(const std::string &);
	virtual void setSpeechSegment(Bliss::SpeechSegment *);

	// if no orthography is given, the orthography form Bliss::SpeechSegment * is used
	const Alignment* getAlignment(const std::string &orthography = "");
	Lattice::ConstWordLatticeRef getWordLattice(const std::string &orthography = "");
    };

} // namespace Speech

#endif // _SPEECH_SEGMENTWISE_ALIGNMENT_GENERATOR_HH
