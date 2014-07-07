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
#define FAST_MODEL

#include "AllophoneStateGraphBuilder.hh"
#include "ModelCombination.hh"
#include "Alignment.hh"
#include <Fsa/Basic.hh>
#include <Fsa/Best.hh>
#include <Fsa/Compose.hh>
#include <Fsa/Determinize.hh>
#include <Fsa/Project.hh>
#include <Fsa/Storage.hh>
#include <Fsa/Output.hh>
#include <Fsa/RemoveEpsilons.hh>
#include <Bliss/Orthography.hh>
#include <Bliss/Fsa.hh>
#include <Fsa/Cache.hh>
#include <Fsa/Sort.hh>

using namespace Speech;

AllophoneStateGraphBuilder::AllophoneStateGraphBuilder(
	const Core::Configuration &c,
	Core::Ref<const Bliss::Lexicon> lexicon,
	Core::Ref<const Am::AcousticModel> acousticModel,
	bool flatModelAcceptor)
	:
	Precursor(c),
	lexicon_(lexicon),
	acousticModel_(acousticModel),
	orthographicParser_(0),
	modelChannel_(config, "model-automaton"),
	flatModelAcceptor_(flatModelAcceptor)
{}

AllophoneStateGraphBuilder::~AllophoneStateGraphBuilder()
{
	delete orthographicParser_;
}


void AllophoneStateGraphBuilder::addSilenceOrNoise(const Bliss::Pronunciation *pron)
{
	silencesAndNoises_.push_back(pron);
}

void AllophoneStateGraphBuilder::addSilenceOrNoise(const Bliss::Lemma *lemma)
{
	Bliss::Lemma::PronunciationIterator p, p_end;
	for (Core::tie(p, p_end) = lemma->pronunciations(); p != p_end; ++ p) {
		addSilenceOrNoise(p->pronunciation());
	}
}

void AllophoneStateGraphBuilder::setSilencesAndNoises(const std::vector<std::string> &silencesAndNoises)
{
	verify(silencesAndNoises_.empty());
	std::vector <std::string>::const_iterator noiseIt = silencesAndNoises.begin();
	for (; noiseIt != silencesAndNoises.end(); ++ noiseIt) {
		std::string noise(*noiseIt);
		Core::normalizeWhitespace(noise);
		const Bliss::Lemma *lemma = lexicon_->lemma(noise);
		if (lemma) {
			if (lemma->nPronunciations() != 0) {
				addSilenceOrNoise(lemma);
			} else {
				warning("did not find a pronunciation");
			}
		} else {
			warning("did not find lemma");
		}
	}
}

Bliss::OrthographicParser &AllophoneStateGraphBuilder::orthographicParser()
{
	if (!orthographicParser_)
		orthographicParser_ = new Bliss::OrthographicParser(select("orthographic-parser"), lexicon_);
	return *orthographicParser_;
}

Fsa::ConstAutomatonRef AllophoneStateGraphBuilder::lemmaPronunciationToLemmaTransducer()
{
	if (!lemmaPronunciationToLemmaTransducer_) {
		lemmaPronunciationToLemmaTransducer_ = lexicon_->createLemmaPronunciationToLemmaTransducer();
		// sort transducer by output symbols to accelerate composition operations
		lemmaPronunciationToLemmaTransducer_ =
		Fsa::ConstAutomatonRef(Fsa::staticCompactCopy(Fsa::sort(lemmaPronunciationToLemmaTransducer_, Fsa::SortTypeByOutput)));

		Fsa::info(lemmaPronunciationToLemmaTransducer_, log("lemma-pronuncation-to-lemma transducer"));
	}
	return lemmaPronunciationToLemmaTransducer_;
}

Fsa::ConstAutomatonRef AllophoneStateGraphBuilder::phonemeToLemmaPronunciationTransducer()
{
	if (!phonemeToLemmaPronunciationTransducer_) {
		phonemeToLemmaPronunciationTransducer_ = lexicon_->createPhonemeToLemmaPronunciationTransducer(false);
		// sort transducer by output symbols to accelerate composition operations
		phonemeToLemmaPronunciationTransducer_ =
		Fsa::ConstAutomatonRef(Fsa::staticCompactCopy(Fsa::sort(phonemeToLemmaPronunciationTransducer_, Fsa::SortTypeByOutput)));
		Fsa::info(phonemeToLemmaPronunciationTransducer_, log("phoneme-to-lemma-pronuncation transducer"));
	}
	return phonemeToLemmaPronunciationTransducer_;
}

Fsa::ConstAutomatonRef
AllophoneStateGraphBuilder::allophoneStateToPhonemeTransducer()
{
	if (!allophoneStateToPhonemeTransducer_) {
		Core::Ref<const Bliss::PhonemeAlphabet> phonemeAlphabet(
		dynamic_cast<const Bliss::PhonemeAlphabet*>(
		phonemeToLemmaPronunciationTransducer()->getInputAlphabet().get()));

		Core::Ref<Am::TransducerBuilder> tb = acousticModel_->createTransducerBuilder();

		tb->setDisambiguators(phonemeAlphabet->nDisambiguators());
		tb->selectAllophonesFromLexicon();

		// for efficiency reasons, precompute flat allophoneStateToPhoneme transducer
		// without loop and skip transitions and apply transition model (i.e. loops
		// and skips) afterwards on the final transducer
		tb->selectFlatModel();

		tb->selectAllophoneStatesAsInput();
		allophoneStateToPhonemeTransducer_ = tb->createPhonemeLoopTransducer();
		allophoneStateToPhonemeTransducer_ =
		Fsa::ConstAutomatonRef(Fsa::staticCompactCopy(Fsa::sort(allophoneStateToPhonemeTransducer_, Fsa::SortTypeByOutput)));

		// To accelerate the application of context dependency, it would be nice, if allophoneStateToPhonemeTransducer
		// was deterministic wrt. its output symbols:
		// allophoneStateToPhonemeTransducer_ =
		//      Fsa::ConstAutomatonRef(Fsa::staticCompactCopy(Fsa::invert(Fsa::determinize(Fsa::invert(allophoneStateToPhonemeTransducer_)))));
		// Unfortunately, this is currently not possible due to ambiguities at word boundaries
	}
	return allophoneStateToPhonemeTransducer_;
}

Fsa::ConstAutomatonRef
AllophoneStateGraphBuilder::singlePronunciationAllophoneStateToPhonemeTransducer()
{
	if (!singlePronunciationAllophoneStateToPhonemeTransducer_) {
		Core::Ref<const Bliss::PhonemeAlphabet> phonemeAlphabet(
		dynamic_cast<const Bliss::PhonemeAlphabet*>(
		phonemeToLemmaPronunciationTransducer()->getInputAlphabet().get()));

		Core::Ref<Am::TransducerBuilder> tb = acousticModel_->createTransducerBuilder();
		tb->selectAllophonesFromLexicon();
		tb->selectCoarticulatedSinglePronunciation();
		if (flatModelAcceptor_)
			tb->selectFlatModel();
		else
			tb->selectTransitionModel();
		tb->selectAllophoneStatesAsInput();
		singlePronunciationAllophoneStateToPhonemeTransducer_ =
		tb->createPhonemeLoopTransducer();
	}
	return singlePronunciationAllophoneStateToPhonemeTransducer_;
}

AllophoneStateGraphRef AllophoneStateGraphBuilder::build(const std::string &orth)
{
	return finalizeTransducer(buildTransducer(orth));
}

AllophoneStateGraphRef AllophoneStateGraphBuilder::build(const InputLevel &level)
{
	if (level == lemma) {
		return build(Fsa::projectOutput(lemmaPronunciationToLemmaTransducer()));
	} else if (level == phone) {
		Core::Ref<Am::TransducerBuilder> tb = acousticModel_->createTransducerBuilder();
		if (flatModelAcceptor_)
			tb->selectFlatModel();
		else
			tb->selectTransitionModel();
		tb->selectAllophoneStatesAsInput();
		return finalizeTransducer(tb->createPhonemeLoopTransducer());
	} else {
		criticalError("unknown input level");
		return AllophoneStateGraphRef();
	}
}

Fsa::ConstAutomatonRef AllophoneStateGraphBuilder::buildTransducer(const std::string &orth)
{
	return buildTransducer(orthographicParser().createLemmaAcceptor(orth));
}

AllophoneStateGraphRef AllophoneStateGraphBuilder::build(
Fsa::ConstAutomatonRef lemmaAcceptor)
{
	return finalizeTransducer(buildTransducer(lemmaAcceptor));
}

Fsa::ConstAutomatonRef AllophoneStateGraphBuilder::buildTransducer(
Fsa::ConstAutomatonRef lemmaAcceptor)
{
	require(lemmaAcceptor->type() == Fsa::TypeAcceptor);
	require(lemmaAcceptor->getInputAlphabet() == lexicon_->lemmaAlphabet());

	Fsa::AutomatonCounts counts;

	// remove silence and phrases by choosing the shortest path for a flat model acceptor
	// we trim in order to check for not empty but incomplete graphs without final states
	Fsa::ConstAutomatonRef lemmaPronunciationAcceptor =
	Fsa::projectOutput(
	Fsa::trim(
	Fsa::composeMatching(
	flatModelAcceptor_ ? Fsa::best(Fsa::extend(lemmaAcceptor, Fsa::Weight(1.0))) : Fsa::ConstAutomatonRef(lemmaAcceptor),
	Fsa::invert(lemmaPronunciationToLemmaTransducer()))));

	if (modelChannel_.isOpen()) {
		Fsa::info(lemmaPronunciationAcceptor, modelChannel_);
		Fsa::drawDot(lemmaPronunciationAcceptor, "/tmp/lemma-pronunciation.dot");
		Fsa::write(lemmaPronunciationAcceptor, "bin:/tmp/lemma-pronunciation.binfsa.gz");
	}

	if (lemmaPronunciationAcceptor->initialStateId() == Fsa::InvalidStateId)
		criticalError("lemma-pronuncation graph is empty. Probably the current sentence contains a word that has no pronunciation.");

	Fsa::ConstAutomatonRef phon = Fsa::trim(
	Fsa::composeMatching(phonemeToLemmaPronunciationTransducer(), lemmaPronunciationAcceptor));
	if (modelChannel_.isOpen()) {
		Fsa::info(phon, modelChannel_);
		Fsa::drawDot(phon, "/tmp/phon.dot");
		Fsa::write(phon, "bin:/tmp/phon.binfsa.gz");
	}

	if (phon->initialStateId() == Fsa::InvalidStateId)
		criticalError("phoneme graph is empty.  Probably the current sentence contains a word that has no pronunciation.");

	// remove pronunciation variants
	if (flatModelAcceptor_) phon = Fsa::best(phon);

	Fsa::ConstAutomatonRef model = Fsa::trim(
	Fsa::composeMatching(allophoneStateToPhonemeTransducer(), phon));

	if (!flatModelAcceptor_) {
		model = Fsa::cache(model);
		Core::Ref<Am::TransducerBuilder> tb = acousticModel_->createTransducerBuilder();
		tb->selectAllophoneStatesAsInput();
		tb->selectTransitionModel();
		tb->setDisambiguators(1); // word end disambiguators
		model = tb->applyTransitionModel(model);
	}

	if (modelChannel_.isOpen()) {
		Fsa::info(model, log());
		Fsa::drawDot(model, "/tmp/states.dot");
		Fsa::write(model, "bin:/tmp/states.binfsa.gz", Fsa::storeStates);
	}
	if (model->initialStateId() == Fsa::InvalidStateId)
		criticalError("allophone-state graph is empty.");

	return model;
}

AllophoneStateGraphRef AllophoneStateGraphBuilder::finalizeTransducer(
Fsa::ConstAutomatonRef allophoneStateToLemmaPronuncationTransducer)
{
	AllophoneStateGraphRef modelAcceptor =
	Fsa::removeEpsilons(
	Fsa::removeDisambiguationSymbols(
	Fsa::projectInput(allophoneStateToLemmaPronuncationTransducer)));

	if (modelChannel_.isOpen()) {
		Fsa::info(modelAcceptor, modelChannel_);
		Fsa::drawDot(modelAcceptor, "/tmp/model.dot");
		Fsa::write(modelAcceptor, "bin:/tmp/model.binfsa.gz", Fsa::storeStates);
	}
	return modelAcceptor;
}

AllophoneStateGraphRef AllophoneStateGraphBuilder::build(
const Bliss::Coarticulated<Bliss::Pronunciation> &pronunciation)
{
	Core::Ref<Am::TransducerBuilder> tb = acousticModel_->createTransducerBuilder();
	if (flatModelAcceptor_)
		tb->selectFlatModel();
	else
		tb->selectTransitionModel();
	tb->selectAllophoneStatesAsInput();
	tb->setSilencesAndNoises(silencesAndNoises_.empty() ? 0 : &silencesAndNoises_);
	return finalizeTransducer(tb->createPronunciationTransducer(pronunciation));
}

AllophoneStateGraphRef AllophoneStateGraphBuilder::build(const Alignment &alignment)
{
	return build(alignment, singlePronunciationAllophoneStateToPhonemeTransducer());
}

AllophoneStateGraphRef AllophoneStateGraphBuilder::build(
const Alignment &alignment, const Bliss::Coarticulated<Bliss::Pronunciation> &pronunciation)
{
	return build(alignment, build(pronunciation));
}

AllophoneStateGraphRef AllophoneStateGraphBuilder::build(
const Alignment &alignment, AllophoneStateGraphRef allophoneStateGraph)
{
	Fsa::ConstAutomatonRef model =
	Fsa::trim(Fsa::composeMatching(createAlignmentGraph(alignment),
				       allophoneStateGraph));
	if (model->initialStateId() == Fsa::InvalidStateId)
		warning("Allophone state graph generated from alignment has no final state.");
	return finalizeTransducer(model);
}

Fsa::ConstAutomatonRef AllophoneStateGraphBuilder::createAlignmentGraph(const Alignment &alignment)
{
	Core::Ref<Fsa::StaticAutomaton> f(new Fsa::StaticAutomaton());
	f->setSemiring(Fsa::TropicalSemiring);
	f->setInputAlphabet(acousticModel_->allophoneStateAlphabet());
	f->setType(Fsa::TypeAcceptor);
	f->addProperties(Fsa::PropertyStorage | Fsa::PropertySortedByInput |
			 Fsa::PropertySortedByOutput | Fsa::PropertyAcyclic);

	Fsa::State *sp = 0;
	Fsa::StateId sid = f->newState(Fsa::StateTagNone, f->semiring()->one())->id();
	f->setInitialStateId(sid);
	for (Alignment::const_iterator al = alignment.begin(); al != alignment.end(); ++ al) {
		sp = f->fastState(sid);
		sid = f->newState(Fsa::StateTagNone, f->semiring()->one())->id();
		sp->newArc(sid, f->semiring()->one(), al->emission);
	}
	f->fastState(sid)->addTags(Fsa::StateTagFinal);
	f->normalize();
	return f;
}
