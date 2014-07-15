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
#ifndef _SPEECH_LATTICE_EXTRACTOR_HH
#define _SPEECH_LATTICE_EXTRACTOR_HH

#include "DataExtractor.hh"
#include <Bliss/Lexicon.hh>
#include <Lattice/Lattice.hh>
#include "PhonemeSequenceAlignmentGenerator.hh"
#include <Lm/ScaledLanguageModel.hh>

namespace Speech {
    class SegmentwiseFeatureExtractor;
    class AllophoneStateGraphBuilder;
    class ArchiveApproximateWordAccuracyLatticeBuilder;
    class OrthographyApproximateWordAccuracyLatticeBuilder;
    class ArchiveApproximatePhoneAccuracyLatticeBuilder;
    class OrthographyApproximatePhoneAccuracyLatticeBuilder;
}

namespace Lattice {
    class ArchiveReader;
}

namespace Speech {

    /*
     * LatticeExtractor
     */
    class LatticeExtractor : public Core::Component
    {
    protected:
	static const Core::ParameterString paramLevel;
    private:
	std::string level_;
    public:
	LatticeExtractor(const Core::Configuration &);
	virtual ~LatticeExtractor() {}

	const std::string& level() const { return level_; }

	virtual Lattice::ConstWordLatticeRef extract(
	    Lattice::ConstWordLatticeRef, Bliss::SpeechSegment *);

	/** Override this function to add triggers to the corpus visitor */
	virtual void signOn(Speech::CorpusVisitor &corpusVisitor) {};

	/** override this function to perform something when all data has been processed */
	virtual void finalize() {}
    };

    /*
     * LatticeRescorer
     */
    class LatticeRescorer : public LatticeExtractor
    {
	typedef LatticeExtractor Precursor;
    protected:
	virtual Lattice::ConstWordLatticeRef work(
	    Lattice::ConstWordLatticeRef, Bliss::SpeechSegment *) = 0;
    public:
	LatticeRescorer(const Core::Configuration &);
	virtual ~LatticeRescorer() {}

	virtual Lattice::ConstWordLatticeRef extract(
	    Lattice::ConstWordLatticeRef, Bliss::SpeechSegment *);
    };

    /*
     * LatticeRescorer: acoustic base class
     */
    class AcousticLatticeRescorerBase : public LatticeRescorer
    {
	typedef LatticeRescorer Precursor;
    protected:
	Core::Ref<Am::AcousticModel> acousticModel_;
    public:
	AcousticLatticeRescorerBase(const Core::Configuration &);
	virtual ~AcousticLatticeRescorerBase() {}

	Core::Ref<Am::AcousticModel> acousticModel() const {
	    return acousticModel_;
	}
    };

    /*
     * LatticeRescorer: acoustic base class for exact match
     */
    class AcousticLatticeRescorer : public AcousticLatticeRescorerBase
    {
	typedef AcousticLatticeRescorerBase Precursor;
	typedef Core::Ref<PhonemeSequenceAlignmentGenerator> AlignmentGeneratorRef;
    protected:
	AlignmentGeneratorRef alignmentGenerator_;
    protected:
	virtual Lattice::ConstWordLatticeRef work(
	    Lattice::ConstWordLatticeRef, Bliss::SpeechSegment *);
    public:
	AcousticLatticeRescorer(const Core::Configuration &);
	virtual ~AcousticLatticeRescorer() {}

	virtual void setAlignmentGenerator(AlignmentGeneratorRef alignmentGenerator) {
	    alignmentGenerator_ = alignmentGenerator;
	}
	virtual void signOn(Speech::CorpusVisitor &corpusVisitor);
    };


    /*
     * LatticeRescorer: "alignment model" (aka acoustic score).
     * In contrast to CombinedAcousticLatticeRescorer, this
     * rescorer uses the scores from the alignment,
     * which makes rescoring more efficient.
     * However, it is less general than CombinedAcousticLatticeRescorer
     * because the acoustic model for the alignment and scoring are the same.
     */
    class AlignmentLatticeRescorer : public virtual AcousticLatticeRescorer
    {
	typedef AcousticLatticeRescorer Precursor;
	typedef Core::Ref<PhonemeSequenceAlignmentGenerator> AlignmentGeneratorRef;
    protected:
	virtual Lattice::ConstWordLatticeRef work(
	    Lattice::ConstWordLatticeRef, Bliss::SpeechSegment *);
    public:
	AlignmentLatticeRescorer(const Core::Configuration &);
	AlignmentLatticeRescorer(const Core::Configuration &, Core::Ref<const Am::AcousticModel>);
	virtual ~AlignmentLatticeRescorer() {}

	virtual void setAlignmentGenerator(AlignmentGeneratorRef alignmentGenerator) {
	    alignmentGenerator_ = alignmentGenerator;
	    acousticModel_ = alignmentGenerator_->acousticModel();
	}
    };


    /*
     * LatticeGenerator: lm rescoring
     */
    class LmLatticeRescorer : public LatticeRescorer
    {
	typedef LatticeRescorer Precursor;
    protected:
	Core::Ref<const Lm::ScaledLanguageModel> languageModel_;
    protected:
	virtual Lattice::ConstWordLatticeRef work(
	    Lattice::ConstWordLatticeRef, Bliss::SpeechSegment *);
    public:
	LmLatticeRescorer(const Core::Configuration &,
			  bool initialize = true);
	virtual ~LmLatticeRescorer() {}
    };

    /*
     * LatticeRescorer: combined lm = lm + pronunciation
     */
    class CombinedLmLatticeRescorer : public LmLatticeRescorer
    {
	typedef LmLatticeRescorer Precursor;
    private:
	f32 pronunciationScale_;
    protected:
	virtual Lattice::ConstWordLatticeRef work(
	    Lattice::ConstWordLatticeRef, Bliss::SpeechSegment *);
    public:
	CombinedLmLatticeRescorer(const Core::Configuration &);
	virtual ~CombinedLmLatticeRescorer() {}
    };


    /*
     * LatticeReader
     */
    class LatticeReader : public LatticeExtractor
    {
	typedef LatticeExtractor Precursor;
    private:
	static const Core::ParameterString paramFsaPrefix;
    private:
	Lattice::ArchiveReader *archiveReader_;
    private:
	/**
	 *  Prefix distinguishing different lattices in one lattice archive.
	 *  If paramFsaPrefix is not given, configuration name of this object
	 *  is used.
	 */
	std::string fsaPrefix_;
    public:
	LatticeReader(const Core::Configuration &, Bliss::LexiconRef);
	virtual ~LatticeReader() {}

	virtual Lattice::ConstWordLatticeRef extract(
	    Lattice::ConstWordLatticeRef, Bliss::SpeechSegment *);
    };

    /*
     * LatticeRescorer: distance
     */
    class DistanceLatticeRescorer : public LatticeRescorer
    {
	typedef LatticeRescorer Precursor;
    public:
	enum DistanceType {
	    approximateWordAccuracy,
	    approximatePhoneAccuracy,
	    approximatePhoneAccuracyMask,
	    frameStateAccuracy,
	    levenshteinOnList,
	    wordAccuracy,
	    phonemeAccuracy,
	    frameWordAccuracy,
	    framePhoneAccuracy,
	    smoothedFrameStateAccuracy,
	    softFramePhoneAccuracy };
	static Core::Choice choiceDistanceType;
	static Core::ParameterChoice paramDistanceType;

	enum SpokenSource {
	    orthography,
	    archive};
	static Core::Choice choiceSpokenSource;
	static Core::ParameterChoice paramSpokenSource;
    public:
	DistanceLatticeRescorer(const Core::Configuration &);
	virtual ~DistanceLatticeRescorer() {}

	static LatticeRescorer* createDistanceLatticeRescorer(
	    const Core::Configuration &, Bliss::LexiconRef);
    };

    /*
     * LatticeRescorer: approximate
     */
    class ApproximateDistanceLatticeRescorer : public DistanceLatticeRescorer
    {
	typedef DistanceLatticeRescorer Precursor;
    protected:
	virtual Lattice::ConstWordLatticeRef work(
	    Lattice::ConstWordLatticeRef, Bliss::SpeechSegment *);
	virtual Fsa::ConstAutomatonRef getDistanceFsa(
	    Lattice::ConstWordLatticeRef, Bliss::SpeechSegment *) {
	    return Fsa::ConstAutomatonRef();
	}
    public:
	ApproximateDistanceLatticeRescorer(const Core::Configuration &, Bliss::LexiconRef);
	virtual ~ApproximateDistanceLatticeRescorer() {}
    };

    /*
     * LatticeRescorer: approximate word accuracy
     */
    class ArchiveApproximateWordAccuracyLatticeRescorer :
	public ApproximateDistanceLatticeRescorer
    {
	typedef ApproximateDistanceLatticeRescorer Precursor;
    private:
	ArchiveApproximateWordAccuracyLatticeBuilder *builder_;
    protected:
	virtual Fsa::ConstAutomatonRef getDistanceFsa(
	    Lattice::ConstWordLatticeRef, Bliss::SpeechSegment *);
    public:
	ArchiveApproximateWordAccuracyLatticeRescorer(const Core::Configuration &, Bliss::LexiconRef);
	virtual ~ArchiveApproximateWordAccuracyLatticeRescorer();
    };

    class OrthographyApproximateWordAccuracyLatticeRescorer :
	public ApproximateDistanceLatticeRescorer
    {
	typedef ApproximateDistanceLatticeRescorer Precursor;
    private:
	OrthographyApproximateWordAccuracyLatticeBuilder *builder_;
    protected:
	virtual Fsa::ConstAutomatonRef getDistanceFsa(
	    Lattice::ConstWordLatticeRef, Bliss::SpeechSegment *);
    public:
	OrthographyApproximateWordAccuracyLatticeRescorer(const Core::Configuration &, Bliss::LexiconRef);
	virtual ~OrthographyApproximateWordAccuracyLatticeRescorer();
    };

    /*
     * LatticeRescorer: approximate phone accuracy
     */
    class ApproximatePhoneAccuracyLatticeRescorer :
	public ApproximateDistanceLatticeRescorer
    {
	typedef ApproximateDistanceLatticeRescorer Precursor;
	typedef Core::Ref<PhonemeSequenceAlignmentGenerator> AlignmentGeneratorRef;
    protected:
	AlignmentGeneratorRef alignmentGenerator_;
    public:
	ApproximatePhoneAccuracyLatticeRescorer(const Core::Configuration &, Bliss::LexiconRef);
	virtual ~ApproximatePhoneAccuracyLatticeRescorer();

	void setAlignmentGenerator(AlignmentGeneratorRef alignmentGenerator) {
	    alignmentGenerator_ = alignmentGenerator;
	}
    };

    class ArchiveApproximatePhoneAccuracyLatticeRescorer :
	public ApproximatePhoneAccuracyLatticeRescorer
    {
	typedef ApproximatePhoneAccuracyLatticeRescorer Precursor;
    private:
	ArchiveApproximatePhoneAccuracyLatticeBuilder *builder_;
    protected:
	virtual Fsa::ConstAutomatonRef getDistanceFsa(
	    Lattice::ConstWordLatticeRef, Bliss::SpeechSegment *);
    public:
	ArchiveApproximatePhoneAccuracyLatticeRescorer(const Core::Configuration &, Bliss::LexiconRef);
	virtual ~ArchiveApproximatePhoneAccuracyLatticeRescorer();
    };

    class OrthographyApproximatePhoneAccuracyLatticeRescorer :
	public ApproximatePhoneAccuracyLatticeRescorer
    {
	typedef ApproximatePhoneAccuracyLatticeRescorer Precursor;
    private:
	OrthographyApproximatePhoneAccuracyLatticeBuilder *builder_;
    protected:
	virtual Fsa::ConstAutomatonRef getDistanceFsa(
	    Lattice::ConstWordLatticeRef, Bliss::SpeechSegment *);
    public:
	OrthographyApproximatePhoneAccuracyLatticeRescorer(const Core::Configuration &, Bliss::LexiconRef);
	virtual ~OrthographyApproximatePhoneAccuracyLatticeRescorer();
    };



} // namespace Speech

#endif // _SPEECH_LATTICE_EXTRACTOR_HH
