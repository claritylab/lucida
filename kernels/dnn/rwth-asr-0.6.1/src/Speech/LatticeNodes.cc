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
#include "LatticeNodes.hh"
#include "ModelCombination.hh"
#include <Flow/DataAdaptor.hh>
#include <Lattice/Archive.hh>
#include <Flf/FlfCore/Lattice.hh>
#include <Flf/FlfCore/Basic.hh>
#include <Flf/FlfCore/Ftl.hh>
#include <Flf/FlfCore/Semiring.hh>
#include <Fsa/Arithmetic.hh>
#include <Fsa/Cache.hh>
#include <Fsa/Compose.hh>
#include <Fsa/Sssp.hh>
#include <Fsa/Static.hh>

namespace Speech {

    class ToWordBoundariesDfs : public Fsa::DfsState
    {
    private:
	Flf::ConstBoundariesRef boundaries_;
	Core::Ref<Lattice::WordBoundaries> wordBoundaries_;
    public:
	ToWordBoundariesDfs(Fsa::ConstAutomatonRef f, Flf::ConstBoundariesRef boundaries) :
	    Fsa::DfsState(f),
	    boundaries_(boundaries),
	    wordBoundaries_(new Lattice::WordBoundaries) {}
	virtual void discoverState(Fsa::ConstStateRef sp) {
	    const Fsa::StateId s = sp->id();
	    const Flf::Boundary::Transit transit = boundaries_->transit(s);
	    Lattice::WordBoundary wordBoundary(
		boundaries_->time(s),
		std::make_pair(transit.final, transit.initial));
	    wordBoundaries_->set(s, wordBoundary);
	}
	Core::Ref<const Lattice::WordBoundaries> getWordBoundaries() {
	    dfs();
	    return wordBoundaries_;
	}
    };

    Lattice::ConstWordLatticeRef toWordLattice(Flf::ConstLatticeRef l)
    {
	Core::Ref<Lattice::WordLattice> result(new Lattice::WordLattice);
	std::vector<Fsa::ConstAutomatonRef> fsas = Flf::toFsaVector(l);
	require(!fsas.empty());
	ToWordBoundariesDfs s(fsas.front(), l->getBoundaries());
	result->setWordBoundaries(s.getWordBoundaries());
	for (u32 i = 0; i < fsas.size(); ++ i) {
	    result->setFsa(fsas[i], l->semiring()->key(i));
	}
	return result;
    }

    class FromWordBoundariesDfs : public Fsa::DfsState
    {
    private:
	Flf::StaticBoundaries *boundaries_;
	Core::Ref<const Lattice::WordBoundaries> wordBoundaries_;
    public:
	FromWordBoundariesDfs(Lattice::ConstWordLatticeRef lattice) :
	    Fsa::DfsState(lattice->part(0)),
	    boundaries_(new Flf::StaticBoundaries),
	    wordBoundaries_(lattice->wordBoundaries()) {}
	virtual void discoverState(Fsa::ConstStateRef sp) {
	    const Fsa::StateId s = sp->id();
	    boundaries_->set(
		s,
		Flf::Boundary(
		    wordBoundaries_->time(s),
		    Flf::Boundary::Transit(
			wordBoundaries_->transit(s).final,
			wordBoundaries_->transit(s).initial)));
	}
	Flf::ConstBoundariesRef getBoundaries() {
	    dfs();
	    return Flf::ConstBoundariesRef(boundaries_);
	}
    };

    Flf::ConstLatticeRef fromWordLattice(Lattice::ConstWordLatticeRef lattice)
    {
	std::vector<Fsa::ConstAutomatonRef> fsas;
	Flf::KeyList keys;
	require(lattice->nParts() > 0);
	for (size_t i = 0; i < lattice->nParts(); ++ i) {
	    fsas.push_back(lattice->part(i));
	    keys.push_back(Flf::Key(lattice->name(i)));
	}
	Flf::ConstLatticeRef l =
	    Flf::fromFsaVector(
		fsas,
		Flf::Semiring::create(
		    getSemiringType(fsas[0]->semiring()), //hack
		    fsas.size(),
		    Flf::ScoreList(fsas.size(), 1.0),
		    keys));
	FromWordBoundariesDfs s(lattice);
	l->setBoundaries(s.getBoundaries());
	return l;
    }

} // namespace Speech

using namespace Speech;

/** ModelCombinationNode
 */
Core::Choice ModelCombinationNode::choiceMode(
    "lexicon", ModelCombination::useLexicon,
    "lexicon+am", ModelCombination::useAcousticModel,
    "lexicon+lm", ModelCombination::useLanguageModel,
    "complete", ModelCombination::complete,
    Core::Choice::endMark());

Core::ParameterChoice ModelCombinationNode::paramMode(
    "mode",
    &choiceMode,
    "parts to load",
    ModelCombination::useLexicon);

Core::Choice ModelCombinationNode::choiceAcousticModelMode(
    "complete", Am::AcousticModel::complete,
    "no-emissions", Am::AcousticModel::noEmissions,
    "no-state-tying", Am::AcousticModel::noStateTying,
    "no-state-transition", Am::AcousticModel::noStateTransition,
    "no-emissions+no-state-tying", Am::AcousticModel::noEmissions | Am::AcousticModel::noStateTying,
    "no-emissions+no-state-transition", Am::AcousticModel::noEmissions | Am::AcousticModel::noStateTransition,
    "no-state-tying+no-state-transition", Am::AcousticModel::noStateTying | Am::AcousticModel::noStateTransition,
    "no-emissions+no-state-tying+no-state-transition", Am::AcousticModel::noEmissions | Am::AcousticModel::noStateTying | Am::AcousticModel::noStateTransition,
    Core::Choice::endMark());

Core::ParameterChoice ModelCombinationNode::paramAcousticModelMode(
    "acoustic-model-mode",
    &choiceAcousticModelMode,
    "parts to load from acoustic model",
    Am::AcousticModel::complete);

ModelCombinationNode::ModelCombinationNode(const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c),
    mode_(paramMode(config)),
    acousticModelMode_(paramAcousticModelMode(config)),
    needInit_(true)
{
    addInputs(0);
    addOutputs(1);
}

bool ModelCombinationNode::configure()
{
    Core::Ref<Flow::Attributes> attributes(new Flow::Attributes);

    if (needInit_) initialize();

    attributes->set("datatype", Flow::DataAdaptor<ModelCombinationRef>::type()->name());
    return putOutputAttributes(0, attributes);
}

void ModelCombinationNode::initialize()
{
    modelCombination_ = ModelCombinationRef(
	new ModelCombination(
	    select("model-combination"),
	    mode_, acousticModelMode_));
    modelCombination_->load();
    out_ = new Flow::DataAdaptor<ModelCombinationRef>();
    out_->data() = modelCombination_;

    needInit_ = false;
}

bool ModelCombinationNode::work(Flow::PortId p)
{
    if (needInit_) initialize();

    verify(out_->data());

    return putData(0, out_);
}

/** LatticeIoNode
 */
const Core::ParameterString LatticeIoNode::paramSegmentId(
    "id",
    "segment identifier for caches.");

LatticeIoNode::LatticeIoNode(const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c),
    needInit_(true),
    segmentId_(paramSegmentId(config))
{
    addInputs(1);
    addOutputs(1);
}

bool LatticeIoNode::setParameter(const std::string &name, const std::string &value)
{
    if (paramSegmentId.match(name)) segmentId_ = paramSegmentId(value);
    else return false;
    return true;
}

bool LatticeIoNode::configure()
{
    Core::Ref<Flow::Attributes> attributes(new Flow::Attributes);
    getInputAttributes(0, *attributes);

    if (!configureDatatype(attributes, Flow::DataAdaptor<ModelCombinationRef>::type())) {
	return false;
    }

    if (segmentId_.empty()) {
	error("Segment identifier is not given.");
	return false;
    }

    attributes->set("datatype", Flow::DataAdaptor<Flf::ConstLatticeRef>::type()->name());
    return putOutputAttributes(0, attributes);
}

bool LatticeIoNode::work(Flow::PortId p)
{
    if (needInit_) {
	Flow::DataPtr<Flow::DataAdaptor<ModelCombinationRef> > in;
	if (!getData(0, in)) {
	    error("could not read port 0");
	    return putData(0, in.get());
	}
	initialize(in->data());
	//	require(!getData(0, in));
    }
    return true;
}

/** LatticeReadNode
 */
const Core::ParameterStringVector LatticeReadNode::paramReaders(
    "readers",
    "set of lattice extractors, type=reader",
    ",");

LatticeReadNode::LatticeReadNode(const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c),
    latticeArchiveReader_(0)
{
    addOutputs(1);
    readers_ = paramReaders(config);
}

LatticeReadNode::~LatticeReadNode()
{
    delete latticeArchiveReader_;
}

void LatticeReadNode::initialize(ModelCombinationRef modelCombination)
{
    verify(!latticeArchiveReader_);
    latticeArchiveReader_ = Lattice::Archive::openForReading(
	select("lattice-archive"), modelCombination->lexicon());
    if (!latticeArchiveReader_ || latticeArchiveReader_->hasFatalErrors()) {
	delete latticeArchiveReader_; latticeArchiveReader_ = 0;
	error("failed to open lattice archive");
    }
    respondToDelayedErrors();
    needInit_ = false;
}

bool LatticeReadNode::work(Flow::PortId p)
{
    if (!Precursor::work(p)) {
	return false;
    }
    verify(latticeArchiveReader_);
    Lattice::ConstWordLatticeRef lattice = latticeArchiveReader_->get(segmentId_, readers_);
    if (!lattice || !lattice->nParts()) {
	error("lattice could not be read");
	return putEos(0);
    }
    Flow::DataAdaptor<Flf::ConstLatticeRef> *out = new Flow::DataAdaptor<Flf::ConstLatticeRef>();
    out->data() = fromWordLattice(lattice);
    return putData(0, out) && putEos(0);
}

/** LatticeWriteNode
 */
LatticeWriteNode::LatticeWriteNode(const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c),
    latticeArchiveWriter_(0)
{
    addInputs(1);
}
LatticeWriteNode::~LatticeWriteNode()
{
    delete latticeArchiveWriter_;
}

void LatticeWriteNode::initialize(ModelCombinationRef modelCombination)
{
    Precursor::initialize(modelCombination);
    verify(!latticeArchiveWriter_);
    latticeArchiveWriter_ = Lattice::Archive::openForWriting(
	select("lattice-archive"), modelCombination->lexicon());
    if (!latticeArchiveWriter_ || latticeArchiveWriter_->hasFatalErrors()) {
	delete latticeArchiveWriter_; latticeArchiveWriter_ = 0;
	error("failed to open lattice archive");
	return;
    }
    respondToDelayedErrors();
    needInit_ = false;
}

bool LatticeWriteNode::configure()
{
    Core::Ref<Flow::Attributes> attributes(new Flow::Attributes);
    getInputAttributes(1, *attributes);
    if (!configureDatatype(attributes, Flow::DataAdaptor<Flf::ConstLatticeRef>::type())) {
	return false;
    }
    return Precursor::configure();
}

bool LatticeWriteNode::work(Flow::PortId p)
{
    if (!Precursor::work(p)) {
	return false;
    }
    Flow::DataPtr<Flow::DataAdaptor<Flf::ConstLatticeRef> > in;
    if (!getData(1, in)) {
	error("could not read port 1");
	return false;
    }
    verify(latticeArchiveWriter_);
    latticeArchiveWriter_->store(segmentId_, toWordLattice(in->data()));
    putData(0, in.get());
    require(!getData(1, in));
    return putData(0, in.get());
}

/** LatticeSemiringNode
 */

LatticeSemiringNode::LatticeSemiringNode(const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c)
{
    addInputs(1);
    addOutputs(1);

    semiring_ = Flf::Semiring::create(c);
}


bool LatticeSemiringNode::configure()
{
    Core::Ref<Flow::Attributes> attributes(new Flow::Attributes());
    getInputAttributes(0, *attributes);

    if (!configureDatatype(attributes, Flow::DataAdaptor<Flf::ConstLatticeRef>::type())) {
	return false;
    }

    return putOutputAttributes(0, attributes);
}


bool LatticeSemiringNode::work(Flow::PortId p)
{

    Flow::DataPtr<Flow::DataAdaptor<Flf::ConstLatticeRef> > in;
    while(getData(0, in)) {
	Flow::DataAdaptor<Flf::ConstLatticeRef> *out = new Flow::DataAdaptor<Flf::ConstLatticeRef>();
	out->data() = Flf::changeSemiring(in->data(), semiring_);
	putData(0, out);
    }
    return putData(0, in.get());
}



/** LatticeSimpleModifyNode
 */
const Core::ParameterFloatVector LatticeSimpleModifyNode::paramScales(
    "scales",
    "set the semiring scales");


LatticeSimpleModifyNode::LatticeSimpleModifyNode(const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c)
{
    addInputs(1);
    addOutputs(1);
    std::vector<f64> scales = paramScales(config);
    for (u32 i = 0; i < scales.size(); ++ i) {
	scales_.push_back(Flf::Score(scales[i]));
    }

}

bool LatticeSimpleModifyNode::setParameter(const std::string &name, const std::string &value)
{
    if (paramScales.match(name)) {
	std::vector<f64> scales = paramScales(config);
	for (u32 i = 0; i < scales.size(); ++ i) {
	    scales_.push_back(Flf::Score(scales[i]));
	}
    }
    else return false;
    return true;
}

bool LatticeSimpleModifyNode::configure()
{
    Core::Ref<Flow::Attributes> attributes(new Flow::Attributes());
    getInputAttributes(0, *attributes);

    if (!configureDatatype(attributes, Flow::DataAdaptor<Flf::ConstLatticeRef>::type())) {
	return false;
    }

    return putOutputAttributes(0, attributes);
}

bool LatticeSimpleModifyNode::work(Flow::PortId p)
{

    Flow::DataPtr<Flow::DataAdaptor<Flf::ConstLatticeRef> > in;
    while(getData(0, in)) {
	Flow::DataAdaptor<Flf::ConstLatticeRef> *out = new Flow::DataAdaptor<Flf::ConstLatticeRef>();
	Flf::ConstLatticeRef l = in->data();

	l = changeSemiring(l, Flf::Semiring::create(Fsa::SemiringTypeLog, scales_.size(), scales_, l->semiring()->keys()));

	out->data() = l;
	putData(0, out);
    }

    return putData(0, in.get());
}

/** LatticeTransformNode
 */
const Core::ParameterBool LatticeTransformNode::paramAppendScore(
    "append-score",
    "append posterior scores to lattice (increase the lattice layers)",
    false);

LatticeTransformNode::LatticeTransformNode(
    const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c),
    appendScore_(paramAppendScore(c))
{
    addInputs(1);
    addOutputs(1);
}

bool LatticeTransformNode::configure()
{
    Core::Ref<Flow::Attributes> attributes(new Flow::Attributes);
    getInputAttributes(0, *attributes);
    if (!configureDatatype(attributes, Flow::DataAdaptor<Flf::ConstLatticeRef>::type())) {
	return false;
    }
    return putOutputAttributes(0, attributes);
}

bool LatticeTransformNode::work(Flow::PortId p)
{
    Flow::DataPtr<Flow::DataAdaptor<Flf::ConstLatticeRef> > in;
    if (!getData(0, in)) {
	error("could not read port 0");
	return putData(0, in.get());
    }

    Flf::ConstLatticeRef l = in->data();
    if (appendScore_) {
	Flf::ScoreList scales = l->semiring()->scales();
	scales.push_back(Flf::Score(1.0));
	std::vector<Fsa::ConstAutomatonRef> fsas = Flf::toFsaVector(in->data());
	fsas.push_back(transform(Flf::toFsa(l)));
	Flf::ConstSemiringRef sr = Flf::cloneSemiring(l->semiring());
	l = Flf::fromFsaVector(fsas, Flf::appendSemiring(sr));
    } else {
	l = Flf::fromFsa(transform(toFsa(l)),
			 Flf::Semiring::create(l->semiring()->type(), 1),
			 0);
    }
    l->setBoundaries(in->data()->getBoundaries());

    Flow::DataAdaptor<Flf::ConstLatticeRef> *out = new Flow::DataAdaptor<Flf::ConstLatticeRef>();
    out->data() = l;

    require(!getData(0, in));
    return putData(0, out) && putData(0, in.get());
}

/** LatticeTransform2Node
 */
LatticeTransform2Node::LatticeTransform2Node(
    const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c)
{
    addInputs(1);
}

bool LatticeTransform2Node::configure()
{
    Core::Ref<Flow::Attributes> attributes(new Flow::Attributes);
    getInputAttributes(1, *attributes);
    if (!configureDatatype(attributes, Flow::DataAdaptor<Flf::ConstLatticeRef>::type())) {
	return false;
    }
    return Precursor::configure();
}

bool LatticeTransform2Node::work(Flow::PortId p)
{
    Flow::DataPtr<Flow::DataAdaptor<Flf::ConstLatticeRef> > in1;
    if (!getData(0, in1)) {
	error("could not read port 0");
	return putData(0, in1.get());
    }
    Flow::DataPtr<Flow::DataAdaptor<Flf::ConstLatticeRef> > in2;
    if (!getData(1, in2)) {
	error("could not read port 1");
	return putData(0, in2.get());
    }
    Flf::ConstLatticeRef l = in1->data();
    if (appendScore_) {
	Flf::ScoreList scales = l->semiring()->scales();
	scales.push_back(Flf::Score(1.0));
	std::vector<Fsa::ConstAutomatonRef> fsas = Flf::toFsaVector(in1->data());
	fsas.push_back(transform(Flf::toFsa(in1->data()), Flf::toFsa(in2->data())));
	Flf::ConstSemiringRef sr = Flf::cloneSemiring(l->semiring());
	l = Flf::fromFsaVector(fsas, Flf::appendSemiring(sr));
    } else {
	l = Flf::fromFsa(transform(Flf::toFsa(in1->data()), Flf::toFsa(in2->data())),
			 Flf::Semiring::create(l->semiring()->type(), 1),
			 0);
    }
    l->setBoundaries(in1->data()->getBoundaries());

    Flow::DataAdaptor<Flf::ConstLatticeRef> *out = new Flow::DataAdaptor<Flf::ConstLatticeRef>();
    out->data() = l;
    require(!getData(0, in1));
    require(!getData(1, in2));
    return putData(0, out) && putData(0, in1.get());
}

/** LatticeWordPosteriorNode
 */
LatticeWordPosteriorNode::LatticeWordPosteriorNode(const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c)
{}

Fsa::ConstAutomatonRef LatticeWordPosteriorNode::transform(Fsa::ConstAutomatonRef fsa) const
{
    return Fsa::posterior64(fsa);
}

/** LatticeCopyNode
 */
LatticeCopyNode::LatticeCopyNode(const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c)
{}

Fsa::ConstAutomatonRef LatticeCopyNode::transform(Fsa::ConstAutomatonRef fsa) const
{
    return Fsa::staticCopy(fsa);
}

/** LatticeCacheNode
 */
LatticeCacheNode::LatticeCacheNode(const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c)
{}

Fsa::ConstAutomatonRef LatticeCacheNode::transform(Fsa::ConstAutomatonRef fsa) const
{
    return Fsa::cache(fsa);
}

/** LatticeExpmNode
 */
LatticeExpmNode::LatticeExpmNode(const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c)
{}

Fsa::ConstAutomatonRef LatticeExpmNode::transform(Fsa::ConstAutomatonRef fsa) const
{
    return Fsa::expm(fsa);
}

/** LatticeExpectationPosteriorNode
 */
const Core::ParameterBool LatticeExpectationPosteriorNode::paramNormalize(
    "normalize",
    "posteriors are v-normalized, i.e., mean = 0",
    true);

LatticeExpectationPosteriorNode::LatticeExpectationPosteriorNode(const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c),
    normalize_(paramNormalize(config))
{}

Fsa::ConstAutomatonRef LatticeExpectationPosteriorNode::transform(
    Fsa::ConstAutomatonRef fsa1, Fsa::ConstAutomatonRef fsa2) const
{
    Fsa::Weight dummy;
    return Fsa::posteriorE(fsa1, fsa2, dummy, normalize_);
}

/** LatticeNBestNode
 */
LatticeNBestNode::LatticeNBestNode(const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c)
{
    addInputs(1);
    addOutputs(1);
}

bool LatticeNBestNode::configure()
{
    Core::Ref<Flow::Attributes> attributes(new Flow::Attributes());
    getInputAttributes(0, *attributes);

    if (!configureDatatype(attributes, Flow::DataAdaptor<Flf::ConstLatticeRef>::type())) {
	return false;
    }

    return putOutputAttributes(0, attributes);
}


bool LatticeNBestNode::work(Flow::PortId p)
{

    Flow::DataPtr<Flow::DataAdaptor<Flf::ConstLatticeRef> > in;
    while(getData(0, in)) {
	Flow::DataAdaptor<Flf::ConstLatticeRef> *out = new Flow::DataAdaptor<Flf::ConstLatticeRef>();
	out->data() = FtlWrapper::firstbest(in->data());
	putData(0, out);
    }

    return putData(0, in.get());
}


/** LatticeDumpCtmNode
 */
const Core::ParameterString LatticeDumpCtmNode::paramSegmentId(
   "id",
   "segment identifier.");

const Core::ParameterString LatticeDumpCtmNode::paramTrackId(
   "track",
   "audio track or channel identifier.",
   "1");

LatticeDumpCtmNode::LatticeDumpCtmNode(const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c),
    ctmChannel_(config, "ctm"),
    segmentId_(paramSegmentId(config)),
    trackId_(paramTrackId(config))
{
    addInputs(2);
    addOutputs(1);
}

bool LatticeDumpCtmNode::setParameter(const std::string &name, const std::string &value)
{
    if (paramSegmentId.match(name)) segmentId_ = paramSegmentId(value);
    else if (paramTrackId.match(name)) trackId_ = paramTrackId(value);
    else return false;
    return true;
}

bool LatticeDumpCtmNode::configure()
{
    Core::Ref<Flow::Attributes> attributes(new Flow::Attributes());

    if (inputConnected(1)) {
	getInputAttributes(1, *attributes);
	if(!configureDatatype(attributes, Feature::FlowFeature::type()))
	    return false;
    }

    getInputAttributes(0, *attributes);

    if (!configureDatatype(attributes, Flow::DataAdaptor<Flf::ConstLatticeRef>::type())) {
	return false;
    }

    return putOutputAttributes(0, attributes);
}


bool LatticeDumpCtmNode::work(Flow::PortId p)
{

    std::vector<f32> featureStartTimes;
    if (inputConnected(1)) {
	f32 endTime;
	Flow::DataPtr<Feature::FlowFeature> f;
	while (getData(1, f)) {
	    featureStartTimes.push_back(f32(f->startTime()));
	    endTime = f32(f->endTime());
	}
	featureStartTimes.push_back(endTime);
    }

    Flow::DataPtr<Flow::DataAdaptor<Flf::ConstLatticeRef> > in;
    while(getData(0, in)) {
	Flow::DataAdaptor<Flf::ConstLatticeRef> *out = new Flow::DataAdaptor<Flf::ConstLatticeRef>();

	out->data() = in->data();

	Flf::ConstLatticeRef l = in->data();
	if(ctmChannel_.isOpen()) {
	    ctmChannel_ << ";; segment comment" << std::endl;
	}
	for (Flf::ConstStateRef sr = l->getState(l->initialStateId()); sr->hasArcs();
	     sr = l->getState(sr->begin()->target())) {
	    const Flf::Arc &a(*sr->begin());
	    f32 wordBegin = 0.0;
	    f32 wordEnd = 0.0;
	    u32 wordBeginFrame = l->boundary(sr->id()).time();
	    u32 wordEndFrame = l->boundary(a.target()).time();
	    if(inputConnected(1)){
		if(wordBeginFrame < featureStartTimes.size())
		    wordBegin = featureStartTimes[wordBeginFrame];
		if(wordEndFrame < featureStartTimes.size())
		    wordEnd   = featureStartTimes[wordEndFrame];
	    } else {
		wordBegin = f32(l->boundary(sr->id()).time()) / 100.00;
		wordEnd   = f32(l->boundary(a.target()).time()) / 100.00;
	    }
	    if(ctmChannel_.isOpen() && a.input() != Fsa::Epsilon) {
		ctmChannel_ << segmentId_ << " "
			    << trackId_ << " "
			    << Core::form("%.3f", (wordBegin)) << " "
			    << Core::form("%.3f", (wordEnd-wordBegin)) << " "
			    << l->getInputAlphabet()->symbol(a.input()) << " "
			    << Core::form("%.4f", l->semiring()->project(a.weight()))
			    << std::endl;
	    }
	}
	putData(0, out);
    }

    return putData(0, in.get());
}
