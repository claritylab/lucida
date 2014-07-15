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
#ifndef _SPEECH_WORD_LATTICE_EXTRACTOR_HH
#define _SPEECH_WORD_LATTICE_EXTRACTOR_HH

#include "CorpusProcessor.hh"
#include "LatticeSetProcessor.hh"
#include <Lattice/Archive.hh>
#include <Lattice/Lattice.hh>
#include <Fsa/Automaton.hh>
#include <Core/ReferenceCounting.hh>
#include <Modules.hh>

namespace Bliss {
    class SpeechSegment;
    class OrthographicParser;
}

namespace Lm {
    class LanguageModel;
}

namespace Speech
{

    /**
     * WordLatticeUnion
     */
    class WordLatticeUnion : public LatticeSetProcessor
    {
	typedef LatticeSetProcessor Precursor;
    private:
	static const Core::ParameterString paramFsaPrefix;
    protected:
	std::string prefix_;
	Lattice::ArchiveReader *numeratorArchiveReader_;
    protected:
	const std::string &prefix() const { return prefix_; }
    public:
	WordLatticeUnion(const Core::Configuration &);
	virtual ~WordLatticeUnion();

	virtual void processWordLattice(Lattice::ConstWordLatticeRef, Bliss::SpeechSegment *);
	virtual void initialize(Bliss::LexiconRef);
    };

    /**
     * BaseWordLatticeMerger
     */
    class BaseWordLatticeMerger : public WordLatticeUnion
    {
	typedef WordLatticeUnion Precursor;
    private:
	static const Core::ParameterBool paramMergeOnlyIfSpokenNotInLattice;
    protected:
	bool mergeOnlyIfSpokenNotInLattice_;
	Bliss::OrthographicParser *orthToLemma_;
	Fsa::ConstAutomatonRef lemmaPronToLemma_;
	Fsa::ConstAutomatonRef lemmaToLemmaConfusion_;
    protected:
	bool needsMerging(Lattice::ConstWordLatticeRef, Bliss::SpeechSegment *) const;
    public:
	BaseWordLatticeMerger(const Core::Configuration &);
	virtual ~BaseWordLatticeMerger();

	virtual void processWordLattice(Lattice::ConstWordLatticeRef, Bliss::SpeechSegment *) {}
	virtual void initialize(Bliss::LexiconRef);
    };

    /**
     * WordLatticeMerger
     */
    class WordLatticeMerger : public BaseWordLatticeMerger
    {
	typedef BaseWordLatticeMerger Precursor;
    private:
	Core::Ref<const Lm::LanguageModel> languageModel_;
    public:
	WordLatticeMerger(const Core::Configuration &);
	virtual ~WordLatticeMerger() {}

	virtual void processWordLattice(Lattice::ConstWordLatticeRef, Bliss::SpeechSegment *);
	virtual void initialize(Bliss::LexiconRef);
    };

    class SpokenAndCompetingListProcessor : public BaseWordLatticeMerger
    {
	typedef BaseWordLatticeMerger Precursor;
    private:
	static const Core::ParameterInt paramNumberOfHypotheses;
    private:
	u32 numberOfHypotheses_;
    public:
	SpokenAndCompetingListProcessor(const Core::Configuration &);
	virtual ~SpokenAndCompetingListProcessor() {}

	virtual void processWordLattice(Lattice::ConstWordLatticeRef, Bliss::SpeechSegment *);
    };

    /**
     * NumeratorFromDenominatorExtractor
     */
    class NumeratorFromDenominatorExtractor :
	public LatticeSetProcessor
    {
	typedef LatticeSetProcessor Precursor;
    private:
	Bliss::OrthographicParser *orthToLemma_;
	Fsa::ConstAutomatonRef lemmaPronToLemma_;
	Fsa::ConstAutomatonRef lemmaToLemmaConfusion_;
    public:
	NumeratorFromDenominatorExtractor(const Core::Configuration &);
	virtual ~NumeratorFromDenominatorExtractor();

	virtual void processWordLattice(Lattice::ConstWordLatticeRef, Bliss::SpeechSegment *);
	virtual void initialize(Bliss::LexiconRef);
    };

} // namespace Speech

#endif // _SPEECH_WORD_LATTICE_EXTRACTOR_HH
