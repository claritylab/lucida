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
#ifndef _SPEECH_LATTICE_SET_EXTRACTOR_HH
#define _SPEECH_LATTICE_SET_EXTRACTOR_HH

#include <vector>

#include <Core/Component.hh>
#include <Core/ReferenceCounting.hh>
#include "CorpusProcessor.hh"
#include "LatticeSetProcessor.hh"
#include "LatticeExtractor.hh"
#include <Lattice/Lattice.hh>
#include <Lattice/Archive.hh>

namespace Nn {
    template<typename T>
    class NeuralNetwork;
    class ClassLabelWrapper;
}

namespace Speech {

    class CorpusVisitor;

    /**
     *  LatticeSetExtractor
     */
    class LatticeSetExtractor :
	public virtual Core::Component
    {
	typedef Core::Component Precursor;
    protected:
	static const Core::ParameterStringVector paramAcousticExtractors;
	static const Core::ParameterStringVector paramEmissionExtractors;
	static const Core::ParameterStringVector paramNnEmissionExtractors;
	static const Core::ParameterStringVector paramTdpExtractors;
	static const Core::ParameterStringVector paramPronunciationExtractors;
	static const Core::ParameterStringVector paramLmExtractors;
	static const Core::ParameterStringVector paramCombinedLmExtractors;
	static const Core::ParameterStringVector paramRestorers;
	static const Core::ParameterStringVector paramReaders;
	static const Core::ParameterStringVector paramDistanceExtractors;
	static const Core::ParameterStringVector paramPosteriorExtractors;
	static const Core::ParameterStringVector paramPassExtractors;
    public:
	LatticeSetExtractor(const Core::Configuration &);
	virtual ~LatticeSetExtractor() {}
    };

    /**
     *  LatticeSetGenerator
     */
    class LatticeSetGenerator :
	public LatticeSetProcessor,
	public LatticeSetExtractor
    {
	typedef LatticeSetProcessor Precursor;
	typedef std::vector<LatticeExtractor*> LatticeExtractors;
	typedef Core::Ref<PhonemeSequenceAlignmentGenerator> AlignmentGeneratorRef;
    private:
	enum SearchType { exactMatch, fullSearch };
	static Core::Choice choiceSearchType;
	static const Core::ParameterChoice paramSearchType;
	static const Core::ParameterBool paramShareAcousticModel;
	static const Core::ParameterBool paramLoadAcoustics;
    private:
	Bliss::LexiconRef lexicon_;
	LatticeExtractors extractors_;
	Core::Ref<SegmentwiseFeatureExtractor> segmentwiseFeatureExtractor_;
	AlignmentGeneratorRef alignmentGenerator_;
	std::vector<f64> timeRescorers_;
    private:
	void initializeExtractors();

	/**
	 *  Returns segmentwiseFeatureExtractor_ object.
	 *  Object is created only on-demand thus the first time this function is called.
	 */
	Core::Ref<SegmentwiseFeatureExtractor> segmentwiseFeatureExtractor();
	/**
	 *  Returns alignmentGenerator_ object.
	 *  Object is created only on-demand thus the first time this function is called.
	 */
	AlignmentGeneratorRef alignmentGenerator();

	void appendAcousticRescorers();
	void appendEmissionRescorers();
	void appendNnEmissionRescorers();
	void appendTdpRescorers();
	void appendPronunciationRescorers();
	void appendLmRescorers();
	void appendCombinedLmRescorers();
	void appendRestorers();
	void appendReaders();
	void appendDistanceRescorers();
	void appendPosteriorRescorers();
	void appendPassRescorers();
    public:
	LatticeSetGenerator(const Core::Configuration &);
	virtual ~LatticeSetGenerator();

	virtual void signOn(CorpusVisitor &);
	virtual void processWordLattice(Lattice::ConstWordLatticeRef, Bliss::SpeechSegment *);
	virtual void initialize(Bliss::LexiconRef);
	virtual void setSegmentwiseFeatureExtractor(Core::Ref<SegmentwiseFeatureExtractor>);
	virtual void setAlignmentGenerator(Core::Ref<PhonemeSequenceAlignmentGenerator>);
	virtual bool hasNnEmissionRescorer() const { return paramNnEmissionExtractors(config).size() > 0 ; }
	virtual void leaveCorpus(Bliss::Corpus *corpus);
	virtual std::string name() const { return "rescorer"; }
	virtual void logComputationTime() const;
    };

    /**
     *  LatticeSetReader
     */
    class LatticeSetReader :
	public LatticeSetProcessorRoot,
	public LatticeSetExtractor
    {
	typedef LatticeSetProcessorRoot Precursor;
	typedef std::vector<std::string> LatticeReaders;
    private:
	LatticeReaders readers_;
	Lattice::ArchiveReader *archiveReader_;
    private:
	void initializeReaders();
	void appendAcousticReaders();
	void appendEmissionReaders();
	void appendTdpReaders();
	void appendPronunciationReaders();
	void appendLmReaders();
	void appendCombinedLmReaders();
	void appendReaders();
	void appendPassReaders();
    public:
	LatticeSetReader(const Core::Configuration &);
	virtual ~LatticeSetReader();

	virtual void leaveSpeechSegment(Bliss::SpeechSegment *);
	virtual void initialize(Bliss::LexiconRef);
	virtual std::string name() const { return "reader"; }
    };

    /**
     *  LatticeSetWriter
     */
    class LatticeSetWriter :
	public LatticeSetProcessor
    {
	typedef LatticeSetProcessor Precursor;
    private:
	Lattice::ArchiveWriter *archiveWriter_;
    public:
	LatticeSetWriter(const Core::Configuration &);
	virtual ~LatticeSetWriter();

	virtual void processWordLattice(Lattice::ConstWordLatticeRef, Bliss::SpeechSegment *);
	virtual void initialize(Bliss::LexiconRef);
    };

} // namespace Speech

#endif // _SPEECH_LATTICE_SET_EXTRACTOR_HH
