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
#include "PhonemeSequenceAlignmentGenerator.hh"
#include "FsaCache.hh"
#include <Fsa/Automaton.hh>
#include <Fsa/Basic.hh>
#include <Fsa/Cache.hh>
#include <Fsa/Properties.hh>
#include <Fsa/Sssp.hh>
#include <Flf/FlfCore/Lattice.hh>
#include <Flf/FlfCore/Basic.hh>
#include <Flf/FlfCore/Traverse.hh>

using namespace Speech;

const Core::ParameterString PhonemeSequenceAlignmentGenerator::Cache::paramCache(
    "path",
    "directory of observation-to-state alignments cache");

const Core::ParameterBool PhonemeSequenceAlignmentGenerator::Cache::paramReadOnly(
    "read-only",
    "if true cache is only read",
    false);

PhonemeSequenceAlignmentGenerator::Cache::Cache(
    const Core::Configuration &c, const ModelCombination &modelCombination) :
    Core::Component(c),
    archive_(0),
    dirty_(false)
{
    initializeArchive(modelCombination);
}

PhonemeSequenceAlignmentGenerator::Cache::~Cache()
{
    if (!segmentId_.empty() && dirty()) write(segmentId_);
    clear();
    delete archive_;
}

void PhonemeSequenceAlignmentGenerator::Cache::setSpeechSegmentId(const std::string &segmentId)
{
    if (!segmentId_.empty() && dirty()) write(segmentId_);
    clear();
    segmentId_ = segmentId;
    if (archive_) read(segmentId_);
}

bool PhonemeSequenceAlignmentGenerator::Cache::findForReadAccess(
    const Key &key, ConstAlignmentPtr *result)
{
    AlignmentMap::iterator itAlignment = alignments_.find(key);
    if (itAlignment == alignments_.end()) {
	*result = 0;
	return false;
    } else {
	*result = itAlignment->second;
	return true;
    }
}

bool PhonemeSequenceAlignmentGenerator::Cache::insert(const Key &key, Alignment *alignment)
{
    AlignmentMap::iterator itAlignment = alignments_.find(key);
    if (itAlignment == alignments_.end()) {
	alignments_[key] = alignment;
	setDirty();
	return true;
    } else if (itAlignment->second->score() != alignment->score()) {
	delete itAlignment->second;
	itAlignment->second = alignment;
	setDirty();
	return true;
    }
    return false;
}

void PhonemeSequenceAlignmentGenerator::Cache::initializeArchive(const ModelCombination &modelCombination)
{
    verify(!archive_);
    Core::DependencySet dependencies;
    modelCombination.getDependencies(dependencies);
    /*! \todo missing dependency: pruning parameters */

    if (!paramCache(config).empty()) {
	const bool readOnly = paramReadOnly(config);
	Core::Archive::AccessMode accessMode = Core::Archive::AccessModeReadWrite;
	if (readOnly) {
	    accessMode = Core::Archive::AccessModeRead;
	}
	archive_ = Core::Archive::create(config, paramCache(config), accessMode);

	Core::ArchiveReader ar(*archive_, "DEPENDENCIES");
	if (ar.isOpen()) {
	    Core::DependencySet archiveDependencies;
	    if (!archiveDependencies.read(config, ar)) {
		error("Failed to read dependencies from alignment cache \"%s\".",
		      archive_->path().c_str());
	    } else if (archiveDependencies == dependencies) {
		if (!readOnly) {
		    warning("Archive \"%s\" exists already and will be overwritten.",
			    archive_->path().c_str());
		}
	    }
	} else {
	    Core::XmlOutputStream xw(new Core::ArchiveWriter(*archive_, "DEPENDENCIES"));
	    dependencies.write(xw);
	}
    }
}

void PhonemeSequenceAlignmentGenerator::Cache::clear()
{
    verify(!dirty());
    for (AlignmentMap::iterator it = alignments_.begin(); it != alignments_.end(); ++it)
	delete it->second;
    alignments_.clear();
}

void PhonemeSequenceAlignmentGenerator::Cache::read(const std::string &segmentId)
{
    verify(archive_);
    verify(!dirty());
    Core::ArchiveReader reader(*archive_, segmentId);
    if (reader.isOpen()) {
	Core::BinaryInputStream i(reader);
	u32 size;
	Key key;
	for (i >> size; i && alignments_.size() < size;) {
	    i >> key;
	    alignments_[key] = new Alignment();
	    i >> *alignments_[key];
	}
	if (!i)
	    criticalError("Failed to read observation-to-state alignments \"%s\" from cache \"%s\"",
			  segmentId.c_str(), archive_->path().c_str());

	u32 nScores = 0, nMatchingScores = 0;
	for (i >> size; i && nScores < size; ++nScores) {
	    i >> key;
	    f32 score; i >> score;
	    AlignmentMap::iterator itAlignment = alignments_.find(key);
	    if (itAlignment != alignments_.end()) {
		itAlignment->second->setScore(score);
		++nMatchingScores;
	    }
	}
	if (nScores != nMatchingScores)
	    warning("Found %d scores without matching alignment; discard scores.", (nScores - nMatchingScores));
	if (!i)
	    criticalError("Failed to read observation-to-state alignments \"%s\" from cache \"%s\"",
			  segmentId.c_str(), archive_->path().c_str());
    }
}

void PhonemeSequenceAlignmentGenerator::Cache::write(const std::string &segmentId)
{
    verify(archive_);
    verify(dirty());
    if (archive_->hasAccess(Core::Archive::AccessModeWrite)) {
	Core::ArchiveWriter writer(*archive_, segmentId, false);
	if (writer.isOpen()) {
	    Core::BinaryOutputStream o(writer);
	    o << (u32) alignments_.size();
	    for (AlignmentMap::const_iterator it = alignments_.begin(); o && it != alignments_.end(); ++it)
		o << it->first << *it->second;
	    if (!o)
		error("could not write state lattice \"%s\" to cache \"%s\"", segmentId.c_str(),
		      archive_->path().c_str());

	    o << (u32) alignments_.size();
	    for (AlignmentMap::const_iterator it = alignments_.begin(); o && it != alignments_.end(); ++it)
		o << it->first << (f32)it->second->score();
	    if (!o)
		error("could not write state lattice \"%s\" to cache \"%s\"", segmentId.c_str(),
		      archive_->path().c_str());
	} else {
	    error("could not open file \"%s\" for writing in cache \"%s\"",
		  segmentId.c_str(), archive_->path().c_str());
	}
    } else {
	warning("could not write state lattice because archive is read only");
    }
    resetDirty();
}



//===============================================================================================

const Core::ParameterBool PhonemeSequenceAlignmentGenerator::paramAddEmissionScores(
    "add-emission-scores",
    "copy emission scores into alignment weights",
    false);

PhonemeSequenceAlignmentGenerator::PhonemeSequenceAlignmentGenerator(
    const Core::Configuration &c, Core::Ref<const ModelCombination> mc) :
    Precursor(c, mc),
    useAlignmentCache_(true),
    addEmissionScores_(paramAddEmissionScores(config))
{
    modelAcceptorCache_ = new FsaCache(select("model-acceptor-cache"), Fsa::storeStates);
    alignmentCache_ = new Cache(select("alignment-cache"), *modelCombination());
}

PhonemeSequenceAlignmentGenerator::~PhonemeSequenceAlignmentGenerator()
{
    delete modelAcceptorCache_;
    delete alignmentCache_;
}

void PhonemeSequenceAlignmentGenerator::update(const std::string &segmentId)
{
    alignmentCache_->setSpeechSegmentId(segmentId);
}

void PhonemeSequenceAlignmentGenerator::addEmissionScores(
    Alignment &alignment,
    std::vector<Mm::FeatureScorer::Scorer> &scorers)
{
    for (Alignment::iterator a = alignment.begin(); a != alignment.end(); ++ a) {
	a->weight = scorers[a->time]->score(acousticModel()->emissionIndex(a->emission));
    }
}

void PhonemeSequenceAlignmentGenerator::setSpeechSegment(Bliss::SpeechSegment *segment)
{
    Precursor::setSpeechSegment(segment);
    update(segment->fullName());
}

void PhonemeSequenceAlignmentGenerator::setSpeechSegmentId(const std::string &segmentId) {
    Precursor::setSpeechSegmentId(segmentId);
    update(segmentId);
}

const Alignment* PhonemeSequenceAlignmentGenerator::getAlignment(
    const Bliss::LemmaPronunciation &p, TimeframeIndex tbeg, TimeframeIndex tend)
{
    return getAlignment(Bliss::Coarticulated<Bliss::LemmaPronunciation>(p), tbeg, tend);
}

const Alignment* PhonemeSequenceAlignmentGenerator::getAlignment(
    const Bliss::Coarticulated<Bliss::LemmaPronunciation> &lemmaPronunciation,
    TimeframeIndex tbeg, TimeframeIndex tend)
{
    const Key key(lemmaPronunciation, tbeg, tend);
    const Alignment *result = 0;
    if (!useAlignmentCache_ or !alignmentCache_->findForReadAccess(key, &result)) {
	aligner_.setModel(
	    modelAcceptorCache_->get(allophoneStateGraphBuilder_->createFunctor(
					 Bliss::Coarticulated<Bliss::Pronunciation>(
					     *lemmaPronunciation.object().pronunciation(),
					     lemmaPronunciation.leftContext(),
					     lemmaPronunciation.rightContext()))),
	    acousticModel());
	std::vector<Mm::FeatureScorer::Scorer> scorers;
	getScorers(tbeg, tend, scorers);
	aligner_.feed(scorers);
	Alignment *alignment = new Alignment;
	aligner_.getAlignment(*alignment);

	verify(alignment->empty() or (alignment->score() != Core::Type<Score>::max));

	if (addEmissionScores_) {
	    addEmissionScores(*alignment, scorers);
	}

	alignment->addTimeOffset(tbeg);
	if (useAlignmentCache_ and !alignmentCache_->insert(key, alignment)) {
	    defect();
	}
	result = alignment;
    }
    /// TODO Make skipping optional
    if (result) // Eventually perform mapping of cached alignments
	const_cast<Alignment*>(result)->setAlphabet(acousticModel()->allophoneStateAlphabet(), true);
    return result;
}

Score PhonemeSequenceAlignmentGenerator::alignmentScore(
    const Bliss::Coarticulated<Bliss::LemmaPronunciation> &lemmaPronunciation,
    TimeframeIndex tbeg, TimeframeIndex tend)
{
    return getAlignment(lemmaPronunciation, tbeg, tend)->score();
}


/**
 * Transform a lattice containing the acoustic scores as arc weights
 * into an alignment fsa with posterior probabilities,
 * cf. Aligner::getAlignmentPosteriorFsa(Fsa::ConstAutomatonRef alignmentFsa).
 */
class LatticeToAlignmentFsa : public Lattice::DfsState
{
private:
    PhonemeSequenceAlignmentGenerator *alignmentGenerator_;
    Fsa::StaticAutomaton *f_;
    Core::hash_map<Fsa::LabelId, Fsa::LabelId> mapping_;
    Alignment dummyAlignment_;
private:
    Fsa::State* boundaryState(Fsa::StateId s);
    void insertAlignment(const Alignment *alignment, Fsa::StateId from, const Fsa::State::const_iterator &arc);
public:
    LatticeToAlignmentFsa(PhonemeSequenceAlignmentGenerator*);
    ~LatticeToAlignmentFsa() {}

    virtual void discoverState(Fsa::ConstStateRef);
    std::pair<Fsa::ConstAutomatonRef, Fsa::Weight> convert(Lattice::ConstWordLatticeRef);
};

LatticeToAlignmentFsa::LatticeToAlignmentFsa(
    PhonemeSequenceAlignmentGenerator *alignmentGenerator)
    :
    alignmentGenerator_(alignmentGenerator),
    f_(0)
{}

Fsa::State* LatticeToAlignmentFsa::boundaryState(Fsa::StateId s)
{
    if (mapping_.find(s) == mapping_.end()) {
	mapping_[s] = f_->newState()->id();
	if (s == fsa_->initialStateId()) {
	    f_->setInitialStateId(mapping_[s]);
	}
	if (fsa_->getState(s)->isFinal()) {
	    f_->setStateFinal(f_->fastState(mapping_[s]));
	}
    }
    return f_->fastState(mapping_[s]);
}

void LatticeToAlignmentFsa::insertAlignment(
    const Alignment *alignment,
    Fsa::StateId from,
    const Fsa::State::const_iterator &arc)
{
    // assumption: Viterbi alignment
    if (alignment->size() == 1) {
	boundaryState(from)->newArc(boundaryState(arc->target())->id(), arc->weight(), alignment->front().emission);
    } else if (alignment->size() > 1) {
	Fsa::State *sp = boundaryState(from);
	Fsa::State *_sp = f_->newState();
	Alignment::const_iterator a = alignment->begin();
	sp->newArc(_sp->id(), arc->weight(), a->emission);
	sp = _sp;
	++ a;
	for (; a != alignment->end() - 1; ++ a) {
	    _sp = f_->newState();
	    sp->newArc(_sp->id(), arc->weight(), a->emission);
	    sp = _sp;
	}
	sp->newArc(boundaryState(arc->target())->id(), arc->weight(), a->emission);
    }
}

void LatticeToAlignmentFsa::discoverState(Fsa::ConstStateRef sp)
{
    const TimeframeIndex begtime = wordBoundaries_->time(sp->id());
    require(begtime != InvalidTimeframeIndex);
    const Bliss::LemmaPronunciationAlphabet *alphabet =
	required_cast(const Bliss::LemmaPronunciationAlphabet*, fsa_->getInputAlphabet().get());
    for (Fsa::State::const_iterator a = sp->begin(); a != sp->end(); ++ a) {
	const Alignment *alignment = 0;
	const Bliss::LemmaPronunciation *pronunciation = alphabet->lemmaPronunciation(a->input());
	const TimeframeIndex endtime = wordBoundaries_->time(a->target());
	if (pronunciation) {
	    Bliss::Coarticulated<Bliss::LemmaPronunciation> coarticulatedPronunciation(
		*pronunciation, wordBoundaries_->transit(sp->id()).final,
		wordBoundaries_->transit(fsa_->getState(a->target())->id()).initial);
	    alignment = alignmentGenerator_->getAlignment(coarticulatedPronunciation, begtime, endtime);
	} else {
	    dummyAlignment_.resize(endtime - begtime, AlignmentItem(InvalidTimeframeIndex, Fsa::Epsilon));
	    alignment = &dummyAlignment_;
	}
	insertAlignment(alignment, sp->id(), a);
    }
}

std::pair<Fsa::ConstAutomatonRef, Fsa::Weight> LatticeToAlignmentFsa::convert(
    Lattice::ConstWordLatticeRef lattice)
{
    Fsa::Weight totalInv;
    fsa_ =
	Fsa::cache(
	    Fsa::posterior64(
		Fsa::changeSemiring(
		    lattice->mainPart(),
		    Fsa::LogSemiring),
		totalInv));
    if (fsa_->semiring()->compare(totalInv, fsa_->semiring()->zero()) == 0) {
	Core::Application::us()->warning("no valid path in fsa");
    }
    wordBoundaries_ = lattice->wordBoundaries();
    f_ = new Fsa::StaticAutomaton;
    f_->setType(Fsa::TypeAcceptor);
    f_->setSemiring(Fsa::TropicalSemiring);
    f_->setInputAlphabet(alignmentGenerator_->allophoneStateAlphabet());
    f_->setProperties(Fsa::PropertyAcyclic, Fsa::hasProperties(fsa_, Fsa::PropertyAcyclic) ? Fsa::PropertyAcyclic : 0);
    f_->setDescription(std::string("alignment-fsa(") + fsa_->describe() + ")");
    mapping_.clear();
    dfs();
    return std::make_pair(Fsa::ConstAutomatonRef(f_), totalInv);
}

//Iteration over states (DfsState):
//do coarticulated pronunciation alignment
//for all outgoing arcs from a state (getAlignment(coarticulatedPronunciation, begtime, endtime))
class LatticeToAlignment : public Flf::DfsState
{
private:
    PhonemeSequenceAlignmentGenerator *alignmentGenerator_;
    Alignment latticeAlignment_;
public:
    LatticeToAlignment(PhonemeSequenceAlignmentGenerator*, Flf::ConstLatticeRef);
    ~LatticeToAlignment() {}

    virtual void discoverState(Flf::ConstStateRef);
    void getAlignment(Speech::Alignment&);
};

LatticeToAlignment::LatticeToAlignment(
    PhonemeSequenceAlignmentGenerator *alignmentGenerator, Flf::ConstLatticeRef lattice)
    :
    Flf::DfsState(lattice), alignmentGenerator_(alignmentGenerator)
{}

void LatticeToAlignment::discoverState(Flf::ConstStateRef sp)
{
    const TimeframeIndex begtime = fsa_->boundary(sp->id()).time();
    if (begtime == InvalidTimeframeIndex) {
	return;
    }
    const Bliss::LemmaPronunciationAlphabet *alphabet =
	required_cast(const Bliss::LemmaPronunciationAlphabet*, fsa_->getInputAlphabet().get());
    for (Flf::State::const_iterator arc = sp->begin(); arc != sp->end(); ++ arc) {
	const Bliss::LemmaPronunciation *pronunciation = alphabet->lemmaPronunciation(arc->input());
	if (pronunciation) {
	    const TimeframeIndex endtime = fsa_->boundary(arc->target()).time();
	    Bliss::Coarticulated<Bliss::LemmaPronunciation> coarticulatedPronunciation(
		*pronunciation, fsa_->boundary(sp->id()).transit().final,
		fsa_->boundary(arc->target()).transit().initial);
	    const Alignment *arcAlignment =
		alignmentGenerator_->getAlignment(coarticulatedPronunciation, begtime, endtime);
	    require(arcAlignment);
	    const Mm::Weight arcWeight = f32(fsa_->semiring()->project(arc->weight()));
	    for (Alignment::const_iterator in = arcAlignment->begin(); in != arcAlignment->end(); ++ in) {
		latticeAlignment_.push_back(AlignmentItem((*in).time, (*in).emission, arcWeight));
	    }
	}
    }
}

void LatticeToAlignment::getAlignment(Alignment &alignment)
{
    latticeAlignment_ = Alignment();
    dfs();
    alignment = latticeAlignment_;
    alignment.sortStableItems();
}

void PhonemeSequenceAlignmentGenerator::getAlignment(
    Alignment &alignment, Lattice::ConstWordLatticeRef lattice)
{
    LatticeToAlignmentFsa c(this);
    std::pair<Fsa::ConstAutomatonRef, Fsa::Weight> alignmentPosteriorFsa(c.convert(lattice));
    aligner_.selectMode(Search::Aligner::modeBaumWelch);
    aligner_.getAlignment(alignment, alignmentPosteriorFsa);
    aligner_.selectMode();
}

void PhonemeSequenceAlignmentGenerator::getAlignment(
    Alignment &alignment, Flf::ConstLatticeRef lattice)
{
    LatticeToAlignment c(this, lattice);
    c.getAlignment(alignment);
}
