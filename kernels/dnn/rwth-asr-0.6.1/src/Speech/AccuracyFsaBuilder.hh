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
#ifndef _SPEECH_ACCURACY_FSA_BUILDER_HH
#define _SPEECH_ACCURACY_FSA_BUILDER_HH

#include <Core/Component.hh>
#include <Fsa/Automaton.hh>
#include <Lattice/Accuracy.hh>
#include <Lattice/Lattice.hh>
#include "SegmentwiseAlignmentGenerator.hh"
#include "PhonemeSequenceAlignmentGenerator.hh"

namespace Bliss {
    class Evaluator;
    class Lexicon;
    class OrthographicParser;
}

namespace Lattice {
    class ArchiveReader;
}

namespace Speech {

    template <class BuilderInput>
    class MetricFsaBuilder : public Core::Component
    {
    public:
	class Functor
	{
	private:
	    MetricFsaBuilder &builder_;
	    const std::string &id_;
	    BuilderInput builderInput_;
	public:
	    Functor(MetricFsaBuilder &builder,
		    const std::string &id,
		    BuilderInput builderInput) :
		builder_(builder), id_(id), builderInput_(builderInput) {}

	    const std::string &id() const { return id_; }
	    Fsa::ConstAutomatonRef build() {
		return builder_.build(builderInput_);
	    }
	};
    public:
	MetricFsaBuilder(const Core::Configuration &c) :
	    Core::Component(c) {}

	virtual Fsa::ConstAutomatonRef build(BuilderInput) = 0;
    };


    /**
     * Base class for metrics which are based on the time alignment,
     * e.g. approximate accuracy.
     */
    class TimeAlignmentBasedMetricLatticeBuilder :
	public MetricFsaBuilder<Lattice::ConstWordLatticeRef>
    {
	typedef MetricFsaBuilder<Lattice::ConstWordLatticeRef> Precursor;
    protected:
	static const Core::ParameterStringVector paramShortPausesLemmata;
	enum TokenType {
	    noneType,
	    lemmaPronunciationType,
	    lemmaType,
	    phoneType,
	    stateType };
	static Core::Choice choiceTokenType;
	static Core::ParameterChoice paramTokenType;
    protected:
	Lattice::ShortPauses shortPauses_;
	TokenType tokenType_;
	Lattice::ConstWordLatticeRef reference_;
    protected:
	void setReference(Lattice::ConstWordLatticeRef reference);
	void initializeShortPauses(Core::Ref<const Bliss::Lexicon>);
    public:
	TimeAlignmentBasedMetricLatticeBuilder(
	    const Core::Configuration &, Core::Ref<const Bliss::Lexicon>);
	virtual ~TimeAlignmentBasedMetricLatticeBuilder();
    };

    /**
     * Numerator lattice is extracted from $param lattice using $param orth.
     */
    class OrthographyTimeAlignmentBasedMetricLatticeBuilder :
	public TimeAlignmentBasedMetricLatticeBuilder
    {
	typedef TimeAlignmentBasedMetricLatticeBuilder Precursor;
    protected:
	Bliss::OrthographicParser *orthToLemma_;
	Fsa::ConstAutomatonRef lemmaPronToLemma_;
	Fsa::ConstAutomatonRef lemmaToLemmaConfusion_;
    public:
	OrthographyTimeAlignmentBasedMetricLatticeBuilder(
	    const Core::Configuration &, Core::Ref<const Bliss::Lexicon>);
	virtual ~OrthographyTimeAlignmentBasedMetricLatticeBuilder();

	Functor createFunctor(const std::string &id,
			      const std::string &orth,
			      Lattice::ConstWordLatticeRef lattice);
    };

    /**
     * Numerator lattice is read from archive.
     */
    class ArchiveTimeAlignmentBasedMetricLatticeBuilder :
	public TimeAlignmentBasedMetricLatticeBuilder
    {
	typedef TimeAlignmentBasedMetricLatticeBuilder Precursor;
    private:
	static const Core::ParameterString paramName;
    private:
	std::string name_;
	Lattice::ArchiveReader *numeratorArchiveReader_;
    private:
	const std::string& name() const { return name_; }
    public:
	ArchiveTimeAlignmentBasedMetricLatticeBuilder(
	    const Core::Configuration &, Core::Ref<const Bliss::Lexicon>);
	virtual ~ArchiveTimeAlignmentBasedMetricLatticeBuilder();

	Functor createFunctor(const std::string &id,
			      const std::string &segmentId,
			      Lattice::ConstWordLatticeRef lattice);
    };

    /**
     * Calculate approximate accuracies (cf. Povey). For this purpose numerator
     * lattices containing the spoken hypotheses are used.
     * Two versions are supported:
     *     1) Numerator lattice is extracted from the passed (denominator) lattice
     *        using the orthography.
     *     2) Numerator lattice is read from the archive. They may be generated
     *        with a forced alignment, for instance.
     */
    class OrthographyApproximateWordAccuracyLatticeBuilder :
	public OrthographyTimeAlignmentBasedMetricLatticeBuilder
    {
	typedef OrthographyTimeAlignmentBasedMetricLatticeBuilder Precursor;
    public:
	OrthographyApproximateWordAccuracyLatticeBuilder(
	    const Core::Configuration &, Core::Ref<const Bliss::Lexicon>);
	virtual ~OrthographyApproximateWordAccuracyLatticeBuilder() {}

	virtual Fsa::ConstAutomatonRef build(Lattice::ConstWordLatticeRef);
    };

    class ArchiveApproximateWordAccuracyLatticeBuilder :
	public ArchiveTimeAlignmentBasedMetricLatticeBuilder
    {
	typedef ArchiveTimeAlignmentBasedMetricLatticeBuilder Precursor;
    public:
	ArchiveApproximateWordAccuracyLatticeBuilder(
	    const Core::Configuration &, Core::Ref<const Bliss::Lexicon>);
	virtual ~ArchiveApproximateWordAccuracyLatticeBuilder() {}

	virtual Fsa::ConstAutomatonRef build(Lattice::ConstWordLatticeRef);
    };

    class OrthographyApproximatePhoneAccuracyLatticeBuilder :
	public OrthographyTimeAlignmentBasedMetricLatticeBuilder
    {
	typedef OrthographyTimeAlignmentBasedMetricLatticeBuilder Precursor;
    private:
	Core::Ref<PhonemeSequenceAlignmentGenerator> alignmentGenerator_;
    public:
	OrthographyApproximatePhoneAccuracyLatticeBuilder(
	    const Core::Configuration &, Core::Ref<const Bliss::Lexicon>);
	virtual ~OrthographyApproximatePhoneAccuracyLatticeBuilder() {}

	Functor createFunctor(const std::string &id,
			      const std::string &segmentId,
			      Lattice::ConstWordLatticeRef lattice,
			      Core::Ref<PhonemeSequenceAlignmentGenerator> alignmentGenerator);
	virtual Fsa::ConstAutomatonRef build(Lattice::ConstWordLatticeRef);
    };

    class ArchiveApproximatePhoneAccuracyLatticeBuilder :
	public ArchiveTimeAlignmentBasedMetricLatticeBuilder
    {
	typedef ArchiveTimeAlignmentBasedMetricLatticeBuilder Precursor;
    private:
	Core::Ref<PhonemeSequenceAlignmentGenerator> alignmentGenerator_;
    public:
	ArchiveApproximatePhoneAccuracyLatticeBuilder(
	    const Core::Configuration &, Core::Ref<const Bliss::Lexicon>);
	virtual ~ArchiveApproximatePhoneAccuracyLatticeBuilder() {}

	Functor createFunctor(const std::string &id,
			      const std::string &segmentId,
			      Lattice::ConstWordLatticeRef lattice,
			      Core::Ref<PhonemeSequenceAlignmentGenerator> alignmentGenerator);
	virtual Fsa::ConstAutomatonRef build(Lattice::ConstWordLatticeRef);
    };


    /**
     * approximate phone accuracy */
    class ApproximatePhoneAccuracyLatticeBuilder :
	public TimeAlignmentBasedMetricLatticeBuilder
    {
	typedef TimeAlignmentBasedMetricLatticeBuilder Precursor;
    private:
	AlignmentGeneratorRef alignmentGenerator_;
    public:
	ApproximatePhoneAccuracyLatticeBuilder(
	    const Core::Configuration &, Bliss::LexiconRef);

	Functor createFunctor(const std::string &id,
			      Lattice::ConstWordLatticeRef reference,
			      Lattice::ConstWordLatticeRef lattice,
			      AlignmentGeneratorRef alignmentGenerator);
	virtual Fsa::ConstAutomatonRef build(Lattice::ConstWordLatticeRef);
    };


} //namespace Speech

#endif // _SPEECH_ACCURACY_FSA_BUILDER_HH
