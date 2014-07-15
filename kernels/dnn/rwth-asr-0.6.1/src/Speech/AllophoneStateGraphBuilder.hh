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
#ifndef _SPEECH_ALLOPHONE_STATE_GRAPH_BUILDER_HH
#define _SPEECH_ALLOPHONE_STATE_GRAPH_BUILDER_HH

#include <Core/Component.hh>
#include <Bliss/Lexicon.hh>
#include <Bliss/CorpusDescription.hh>
#include <Fsa/Automaton.hh>

namespace Am {
    class AcousticModel;
}

namespace Bliss {
    class PhonemeToLemmaTransducer;
    class OrthographicParser;
}

namespace Speech {
    class ModelCombination;
    class Alignment;
}

namespace Speech {

    typedef Fsa::Automaton::ConstRef AllophoneStateGraphRef;

    /**
     *  AllophoneStateGraphBuilder
     */
    class AllophoneStateGraphBuilder : public Core::Component
    {
	typedef Core::Component Precursor;
    public:
	/**
	 *   Base class for build-functor classes.
	 */
	template<class BuilderInput>
	class FunctorBase {
	protected:
	    AllophoneStateGraphBuilder &builder_;
	    const std::string id_;
	    const BuilderInput &builderInput_;
	public:
	    FunctorBase(AllophoneStateGraphBuilder &builder,
			const std::string &id,
			const BuilderInput &builderInput) :
		builder_(builder), id_(id), builderInput_(builderInput) {}

	    const std::string &id() const { return id_; }
	};

	/**
	 *   Converts a call to one of the build functions into a functor.
	 *
	 *   Underlying function: build
	 *   Input: template BuilderInput
	 *   Ouptut: AllophoneStateGraphRef
	 */
	template<class BuilderInput>
	struct Functor : public FunctorBase<BuilderInput> {
	private:
	    typedef FunctorBase<BuilderInput> Precursor;
	public:
	    Functor(AllophoneStateGraphBuilder &builder,
		    const std::string &id,
		    const BuilderInput &builderInput) :
		FunctorBase<BuilderInput>(builder, id, builderInput) {}

	    AllophoneStateGraphRef build() {
		return Precursor::builder_.build(Precursor::builderInput_);
	    }
	};

	/**
	 *   Converts a call to one of the finalizeTransducer functions into a functor.
	 *
	 *   Underlying function: finalizeTransducer
	 *   Input: Fsa::ConstAutomatonRef
	 *   Ouptut: AllophoneStateGraphRef
	 */
	struct FinalizationFunctor : public FunctorBase<Fsa::ConstAutomatonRef> {
	private:
	    typedef FunctorBase<Fsa::ConstAutomatonRef> Precursor;
	public:
	    FinalizationFunctor(AllophoneStateGraphBuilder &builder,
				const std::string &id,
				const Fsa::ConstAutomatonRef &builderInput) :
		FunctorBase<Fsa::ConstAutomatonRef>(builder, id, builderInput) {}

	    AllophoneStateGraphRef build() {
		return Precursor::builder_.finalizeTransducer(Precursor::builderInput_);
	    }
	};

	/**
	 *   Converts a call to one of the buildTransducer functions into a functor.
	 *
	 *   Underlying function: buildTransducer
	 *   Input: BuilderInput
	 *   Ouptut: Fsa::ConstAutomatonRef
	 */
	template<class BuilderInput>
	struct TranducerFunctor : public FunctorBase<BuilderInput> {
	private:
	    typedef FunctorBase<BuilderInput> Precursor;
	public:
	    TranducerFunctor(AllophoneStateGraphBuilder &builder,
			     const std::string &id,
			     const BuilderInput &builderInput) :
		FunctorBase<BuilderInput>(builder, id, builderInput) {}

	    Fsa::ConstAutomatonRef build() {
		return Precursor::builder_.buildTransducer(Precursor::builderInput_);
	    }
	};
    private:
	Bliss::LexiconRef lexicon_;
	Core::Ref<const Am::AcousticModel> acousticModel_;
	Bliss::OrthographicParser *orthographicParser_;
	Fsa::ConstAutomatonRef lemmaPronunciationToLemmaTransducer_;
	Fsa::ConstAutomatonRef phonemeToLemmaPronunciationTransducer_;
	Fsa::ConstAutomatonRef allophoneStateToPhonemeTransducer_;
	Fsa::ConstAutomatonRef singlePronunciationAllophoneStateToPhonemeTransducer_;
	Core::XmlChannel modelChannel_;
	bool flatModelAcceptor_;
	std::vector<const Bliss::Pronunciation*> silencesAndNoises_;
    private:
	Bliss::OrthographicParser &orthographicParser();
	Fsa::ConstAutomatonRef lemmaPronunciationToLemmaTransducer();
	Fsa::ConstAutomatonRef phonemeToLemmaPronunciationTransducer();
	Fsa::ConstAutomatonRef allophoneStateToPhonemeTransducer();
	Fsa::ConstAutomatonRef singlePronunciationAllophoneStateToPhonemeTransducer();

	Fsa::ConstAutomatonRef createAlignmentGraph(const Alignment &);
	AllophoneStateGraphRef build(
	    const Alignment &alignment, AllophoneStateGraphRef);
    public:
	AllophoneStateGraphBuilder(
	    const Core::Configuration&,
	    Core::Ref<const Bliss::Lexicon> lexicon,
	    Core::Ref<const Am::AcousticModel> acousticModel,
	    bool flatModelAcceptor = false);

	virtual ~AllophoneStateGraphBuilder();

	void addSilenceOrNoise(const Bliss::Pronunciation*);
	void addSilenceOrNoise(const Bliss::Lemma *lemma);
	void setSilencesAndNoises(const std::vector<std::string> &);

	/** Builds allophone state acceptor from an orthography. */
	AllophoneStateGraphRef build(const std::string &orth);
	Functor<std::string> createFunctor(const std::string &id, const std::string &orth) {
	    return Functor<std::string>(*this, id, orth);
	}
	Functor<std::string> createFunctor(const Bliss::SpeechSegment &s) {
	    return Functor<std::string>(*this, s.fullName(), s.orth());
	}
	/** Builds allophone state acceptor for phoneme loops, cf. phoneme recognition. */
	enum InputLevel { lemma, phone };
	AllophoneStateGraphRef build(const InputLevel &level);
	Functor<InputLevel> createFunctor(const std::string &id, const InputLevel &level) {
	    return Functor<InputLevel>(*this, id, level);
	}
	/** Builds allophone state acceptor from a (non-coarticulated) pronunciation. */
	AllophoneStateGraphRef build(const Bliss::Pronunciation &p) {
	    return build(Bliss::Coarticulated<Bliss::Pronunciation>(p));
	}
	Functor<Bliss::Pronunciation> createFunctor(const Bliss::Pronunciation &p) {
	    Bliss::Coarticulated<Bliss::Pronunciation> cp(p);
	    return Functor<Bliss::Pronunciation>(
		*this, cp.format(lexicon_->phonemeInventory()), p);
	}

	/** Builds allophone state acceptor from a coarticulated pronunciation. */
	AllophoneStateGraphRef build(const Bliss::Coarticulated<Bliss::Pronunciation> &);
	Functor<Bliss::Coarticulated<Bliss::Pronunciation> > createFunctor(
	    const Bliss::Coarticulated<Bliss::Pronunciation> &p) {
	    return Functor<Bliss::Coarticulated<Bliss::Pronunciation> >(
		*this, p.format(lexicon_->phonemeInventory()), p);
	}

	AllophoneStateGraphRef build(const Alignment &);
	Functor<Alignment> createFunctor(const std::string &id, const Alignment &a) {
	    return Functor<Alignment>(*this, id, a);
	}
	/**
	 *  Accelerated way of creating an alignment allophone state graph.
	 *  Pronuniciation restricts the allophone state graph with which the
	 *  alignment graph is composed.
	 */
	AllophoneStateGraphRef build(
	    const Alignment &, const Bliss::Coarticulated<Bliss::Pronunciation> &);

	/** Builds a allophone state to lemma pronunciation transducer from orthography. */
	Fsa::ConstAutomatonRef buildTransducer(const std::string &orth);
	TranducerFunctor<const std::string> createTransducerFunctor(
	    const std::string &id, const std::string &orth) {
	    return TranducerFunctor<const std::string>(*this, id, orth);
	}

	/** Creates a static epsilon free acceptor from the input transducer. */
	AllophoneStateGraphRef finalizeTransducer(Fsa::ConstAutomatonRef);
	FinalizationFunctor createFinalizationFunctor(const std::string &id, Fsa::ConstAutomatonRef t) {
	    return FinalizationFunctor(*this, id, t);
	}

	/** Builds allophone state acceptor from a lemma accertor. */
	AllophoneStateGraphRef build(Fsa::ConstAutomatonRef);
	/** Builds a allophone state to lemma pronunciation transducer from lemma acceptor. */
	Fsa::ConstAutomatonRef buildTransducer(Fsa::ConstAutomatonRef);
    };

} // namespace Speech

#endif // _SPEECH_ALLOPHONE_STATE_GRAPH_BUILDER_HH
