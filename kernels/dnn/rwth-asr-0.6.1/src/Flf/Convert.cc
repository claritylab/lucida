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
#include <Core/Application.hh>
#include <Core/Parameter.hh>
#include <Core/ReferenceCounting.hh>
#include <Core/StringUtilities.hh>

#include "FlfCore/Basic.hh"
#include "FlfCore/LatticeInternal.hh"
#include "Convert.hh"


namespace Flf {

    // -------------------------------------------------------------------------
    class SymbolicMapping : public std::vector<KeyList> {
	typedef std::vector<KeyList> Precursor;
    public:
	SymbolicMapping(size_t capacity = 0) : Precursor(capacity) {}

	static const Core::ParameterStringVector paramRow;
	static SymbolicMapping loadSymbolicMapping(const Core::Configuration &config, ConstSemiringRef target) {
	    SymbolicMapping symbolicMapping(target->size());
	    SymbolicMapping::iterator itSymbolicMapping = symbolicMapping.begin();
	    for (KeyList::const_iterator itKey = target->keys().begin();
		 itKey != target->keys().end(); ++itKey, ++itSymbolicMapping) {
		if (itKey->empty())
		    Core::Application::us()->warning("Dimension with no symbolic identifier in \"%s\"",
					       target->name().c_str());
		else
		    *itSymbolicMapping = paramRow(Core::Configuration(config, *itKey));
	    }
	    return symbolicMapping;
	}
    };
    const Core::ParameterStringVector SymbolicMapping::paramRow(
	"row",
	"row in matrix projection");


    class MappingBuilder {
    private:
	ConstSemiringRef source_;
	ConstSemiringRef target_;

    public:
	MappingBuilder(ConstSemiringRef source, ConstSemiringRef target) :
	    source_(source), target_(target) {}

	ProjectionMatrix buildLinearMapping(ScoreId offset = 0, bool scaled = true) const {
	    ProjectionMatrix scaledMapping(target_->size());
	    ProjectionMatrix::iterator itProjectionMatrix = scaledMapping.begin();
	    ScoreId id = 0;
	    for (; (itProjectionMatrix != scaledMapping.end()) && (id < offset); ++itProjectionMatrix, ++id)
		itProjectionMatrix->resize(source_->size(), 0.0);
	    id = 0;
	    for (; (itProjectionMatrix != scaledMapping.end()) && (id < source_->size()); ++itProjectionMatrix, ++id) {
		itProjectionMatrix->resize(source_->size(), 0.0);
		(*itProjectionMatrix)[id] = scaled ? source_->scale(id) : Semiring::DefaultScale;
	    }
	    for (; (itProjectionMatrix != scaledMapping.end()); ++itProjectionMatrix)
		itProjectionMatrix->resize(source_->size(), 0.0);
	    return scaledMapping;
	}

	ProjectionMatrix buildMapping(const SymbolicMapping &symbolicMapping, bool scaled = true) const {
	    require(symbolicMapping.size() == target_->size());
	    ProjectionMatrix scaledMapping(target_->size());
	    ProjectionMatrix::iterator itProjectionMatrix = scaledMapping.begin();
	    for (SymbolicMapping::const_iterator itKeyList = symbolicMapping.begin();
		 itKeyList != symbolicMapping.end(); ++itKeyList, ++itProjectionMatrix) {
		itProjectionMatrix->resize(source_->size(), 0.0);
		for (KeyList::const_iterator itKey = itKeyList->begin(); itKey != itKeyList->end(); ++itKey) {
		    ScoreId id = source_->id(*itKey);
		    if (source_->hasId(id))
			(*itProjectionMatrix)[id] = scaled ? source_->scale(id) : Semiring::DefaultScale;
		    else
			Core::Application::us()->error("No dimension \"%s\" found in semiring \"%s\"",
			      itKey->c_str(), source_->name().c_str());
		}
	    }
	    return scaledMapping;
	}
    };

    class ProjectSemiringLattice : public ModifyLattice {
	typedef ModifyLattice Precursor;
    private:
	ConstSemiringRef semiring_;
	const ProjectionMatrix mapping_;
    protected:
	ScoresRef project(ScoresRef src) const {
	    ScoresRef score = semiring_->create();
	    Scores::iterator itScore = score->begin();
	    for (ProjectionMatrix::const_iterator itScales = mapping_.begin();
		 itScales != mapping_.end(); ++itScales, ++itScore)
		*itScore = src->project(*itScales);
	    return score;
	}
    public:
	ProjectSemiringLattice(ConstLatticeRef l, ConstSemiringRef targetSemiring, const ProjectionMatrix &mapping) :
	    Precursor(l), semiring_(targetSemiring), mapping_(mapping) {}
	virtual ~ProjectSemiringLattice() {}

	virtual ConstSemiringRef semiring() const {
	    return semiring_;
	}

	virtual void modifyState(State *sp) const {
	    if (sp->isFinal())
		sp->setWeight(project(sp->weight()));
	    else
		sp->setWeight(semiring_->one());
	    u32 i = 0;
	    for (State::iterator a = sp->begin(); a != sp->end(); ++a, ++i)
		a->setWeight(project(a->weight()));
	}

	virtual std::string describe() const {
	    return Core::form("projectSemiring(%s;%s,%s)",
			      fsa_->describe().c_str(), fsa_->semiring()->name().c_str(), semiring_->name().c_str());
	}
    };
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    ConstLatticeRef offsetSemiring(ConstLatticeRef l, ConstSemiringRef targetSemiring, ScoreId offset) {
	MappingBuilder mb(l->semiring(), targetSemiring);
	return projectSemiring(l, targetSemiring, mb.buildLinearMapping(offset, false));
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class ChangeSemiringNode : public FilterNode {
	friend class Network;
    private:
	ConstSemiringRef semiring_;
    protected:
	virtual ConstLatticeRef filter(ConstLatticeRef l) {
	    if (l->semiring()->size() == semiring_->size())
		return changeSemiring(l, semiring_);
	    else
		return offsetSemiring(l, semiring_, 0);
	}
    public:
	ChangeSemiringNode(const std::string &name, const Core::Configuration &config) :
	    FilterNode(name, config) {}
	virtual void init(const std::vector<std::string> &arguments) {
	    semiring_ = Semiring::create(select("semiring"));
	    if (!semiring_)
		criticalError("ChangeSemiringNode: Failed to load semiring");
	}
	virtual ~ChangeSemiringNode() {}
    };
    NodeRef createChangeSemiringNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new ChangeSemiringNode(name, config));
    }


    class ProjectSemiringNode : public FilterNode {
	friend class Network;
    public:
	static const Core::ParameterBool paramScaled;
    private:
	ConstSemiringRef semiring_, lastSemiring_;
	SymbolicMapping symbolicMapping_;
	bool scaled_;
	ProjectionMatrix scaledMapping_;
    protected:
	virtual ConstLatticeRef filter(ConstLatticeRef l) {
	    if (!lastSemiring_
		|| (l->semiring().get() != lastSemiring_.get())
		|| !(*l->semiring() == *lastSemiring_)) {
		lastSemiring_ = l->semiring();
		MappingBuilder mb(l->semiring(), semiring_);
		scaledMapping_ = mb.buildMapping(symbolicMapping_, scaled_);
	    }
	    l = projectSemiring(l, semiring_, scaledMapping_);
	    return l;
	}
    public:
	ProjectSemiringNode(const std::string &name, const Core::Configuration &config) :
	    FilterNode(name, config) {}
	virtual ~ProjectSemiringNode() {}
	virtual void init(const std::vector<std::string> &arguments) {
	    semiring_ = Semiring::create(select("semiring"));
	    if (!semiring_)
		criticalError("ProjectSemiringNode: Failed to load semiring");
	    symbolicMapping_ = SymbolicMapping::loadSymbolicMapping(select("matrix"), semiring_);
	    scaled_ = paramScaled(config);
	}
    };
    const Core::ParameterBool ProjectSemiringNode::paramScaled(
	"scaled",
	"scale projection",
	true);

    NodeRef createProjectSemiringNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new ProjectSemiringNode(name, config));
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    /**
     * fsa to lattice
     **/
    class FsaToLattice : public Lattice {
    public:
	class WeightMap : public Core::ReferenceCounted {
	public:
	    virtual ~WeightMap() {}
	    virtual ScoresRef operator() (Fsa::Weight w) const = 0;
	};
	typedef Core::Ref<WeightMap> WeightMapRef;

	class WeightToScoresMap : public WeightMap {
	private:
	    ConstSemiringRef semiring_;
	    ScoreId id_;
	    ScoresRef scores_;
	public:
	    WeightToScoresMap(ConstSemiringRef semiring, ScoreId id, ScoresRef scores) :
		semiring_(semiring), id_(id), scores_(scores) {
		verify(id < semiring->size());
	    }
	    virtual ScoresRef operator() (Fsa::Weight w) const {
		ScoresRef scores = semiring_->clone(scores_);
		scores->set(id_, Score(w));
		return scores;
	    }
	};

	class ConstantToScoresMap : public WeightMap {
	private:
	    ScoresRef scores_;
	public:
	    ConstantToScoresMap(ScoresRef scores) : scores_(scores) {}
	    virtual ScoresRef operator() (Fsa::Weight w) const { return scores_; }
	};

    private:
	Fsa::ConstAutomatonRef fsa_;
	bool isAcceptor_;
	ConstSemiringRef semiring_;
	WeightMapRef weightMap_;
	Lexicon::AlphabetMapRef inputMap_;
	Lexicon::AlphabetMapRef outputMap_;

    public:
	FsaToLattice(Fsa::ConstAutomatonRef fsa,
		     ConstSemiringRef semiring, WeightMapRef weightMap,
		     Lexicon::AlphabetMapRef inputMap, Lexicon::AlphabetMapRef outputMap) :
	    fsa_(fsa), isAcceptor_(fsa->type() == Fsa::TypeAcceptor),
	    semiring_(semiring), weightMap_(weightMap),
	    inputMap_(inputMap), outputMap_(outputMap) {
	    setProperties(fsa->knownProperties(), fsa->properties());
	}

	virtual ~FsaToLattice() {}

	virtual Fsa::Type type() const {
	    return fsa_->type();
	}

	virtual ConstSemiringRef semiring() const {
	    return semiring_;
	}

	virtual ConstStateRef getState(Fsa::StateId sid) const {
	    Fsa::ConstStateRef sr = fsa_->getState(sid);
	    State *sp = new State(sid);
	    if (sr->isFinal())
		sp->setFinal((*weightMap_)(sr->weight()));
	    if (isAcceptor_)
		for (Fsa::State::const_iterator a = sr->begin(); a != sr->end(); ++a)
		    sp->newArc(a->target(), (*weightMap_)(a->weight()), (*inputMap_)[a->input()]);
	    else
		for (Fsa::State::const_iterator a = sr->begin(); a != sr->end(); ++a)
		    sp->newArc(a->target(), (*weightMap_)(a->weight()), (*inputMap_)[a->input()], (*outputMap_)[a->output()]);
	    return ConstStateRef(sp);
	}

	virtual Fsa::ConstAlphabetRef getInputAlphabet() const {
	    return inputMap_->to();
	}

	virtual Fsa::ConstAlphabetRef getOutputAlphabet() const {
	    return (isAcceptor_) ? inputMap_->to() : outputMap_->to();
	}

	virtual std::string describe() const {
	    return "fsa2flf(" + fsa_->describe() + ")";
	}

	virtual Fsa::StateId initialStateId() const {
	    return fsa_->initialStateId();
	}

	virtual size_t getMemoryUsed() const {
	    return fsa_->getMemoryUsed();
	}

	virtual void dumpMemoryUsage(Core::XmlWriter &o) const {
	    fsa_->dumpMemoryUsage(o);
	}

	virtual void dumpState(Fsa::StateId s, std::ostream &o) const {
	    fsa_->dumpState(s, o);
	}
    };

    ConstLatticeRef fromFsa(Fsa::ConstAutomatonRef fsa,
			    ConstSemiringRef semiring, ScoresRef scores, ScoreId id,
			    Lexicon::AlphabetId inputAlphabetId, Lexicon::AlphabetId outputAlphabetId) {
	if (!semiring) {
	    semiring = Semiring::create(Fsa::SemiringTypeTropical, 1);
	    semiring->setKey(0, "weight");
	}
	if (!scores)
	    scores = semiring->one();
	FsaToLattice::WeightMapRef weightMap;
	if (id == Semiring::InvalidId)
	    weightMap = FsaToLattice::WeightMapRef(new FsaToLattice::ConstantToScoresMap(scores));
	else
	    weightMap = FsaToLattice::WeightMapRef(new FsaToLattice::WeightToScoresMap(semiring, id, scores));
	Lexicon::AlphabetMapRef inputMap =
	    Lexicon::us()->alphabetMap(fsa->getInputAlphabet(), inputAlphabetId);
	verify(inputMap);
	Lexicon::AlphabetMapRef outputMap;
	if (fsa->type() != Fsa::TypeAcceptor) {
	    outputMap = Lexicon::us()->alphabetMap(fsa->getInputAlphabet(), inputAlphabetId);
	    verify(outputMap);
	}
	return ConstLatticeRef(new FsaToLattice(fsa, semiring, weightMap, inputMap, outputMap));
    }

    ConstLatticeRef fromUnweightedFsa(Fsa::ConstAutomatonRef fsa,
				      ConstSemiringRef semiring, ScoresRef constScore,
				      Lexicon::AlphabetId inputAlphabetId, Lexicon::AlphabetId outputAlphabetId) {
	return fromFsa(fsa, semiring, constScore, Semiring::InvalidId, inputAlphabetId, outputAlphabetId);
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    /**
     * string to lattice
     **/
    class StringConverterNode : public Node {
	friend class Network;
    public:
	static const Core::ParameterString paramAlphabet;
    private:
	Lexicon::SymbolMap symbolMap_;
	ConstSemiringRef semiring_;
	std::string s_;
	std::vector<Fsa::LabelId> labels_;
	ConstLatticeRef l_;

    public:
	StringConverterNode(const std::string &name, const Core::Configuration &config) :
	    Node(name, config) {}
	virtual ~StringConverterNode() {}

	virtual void init(const std::vector<std::string> &arguments) {
	    if (!connected(0))
		criticalError("StringConverterNode: String input at port 0 required.");
	    symbolMap_ = Lexicon::us()->symbolMap(Lexicon::us()->alphabetId(paramAlphabet(config), true));
	    semiring_ = Semiring::create(select("semiring"));
	    if (!semiring_)
		semiring_ = Semiring::create(Fsa::SemiringTypeTropical, 0);
	}

	virtual void sync() {
	    s_.clear();
	    labels_.clear();
	    l_.reset();
	}

	virtual ConstLatticeRef sendLattice(Port to) {
	    if (!l_) {
		s_ = requestString(0);
		symbolMap_.indices(s_, labels_);
		StaticLattice *s = new StaticLattice;
		s->setDescription("fromString");
		s->setType(Fsa::TypeAcceptor);
		s->setProperties(Fsa::PropertyAcyclic | Fsa::PropertyLinear, Fsa::PropertyAll);
		s->setInputAlphabet(symbolMap_.alphabet());
		s->setSemiring(semiring_);
		State *sp = s->newState();
		s->setInitialStateId(sp->id());
		for (std::vector<Fsa::LabelId>::const_iterator it = labels_.begin(); it != labels_.end(); ++it) {
		    State *nextSp = s->newState();
		    sp->newArc(nextSp->id(), semiring_->one(), *it);
		    sp = nextSp;
		}
		sp->setFinal(semiring_->one());
		l_ = ConstLatticeRef(s);
	    }
	    return l_;
	}
    };
    const Core::ParameterString StringConverterNode::paramAlphabet(
	"alphabet",
	"name of alphabet",
	"lemma");
    NodeRef createStringConverterNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new StringConverterNode(name, config));
    }
    // -------------------------------------------------------------------------

} // namespace Flf
