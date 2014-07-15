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
#include <Core/Configuration.hh>
#include <Core/Hash.hh>
#include <Core/Parameter.hh>

#include "FlfCore/Basic.hh"
#include "FlfCore/LatticeInternal.hh"
#include "FwdBwd.hh"
#include "Lexicon.hh"
#include "Map.hh"
#include "Rescore.hh"
#include "RescoreInternal.hh"


namespace Flf {

    // -------------------------------------------------------------------------
    /**
     * reduce many scores to a single score;
     * the sum of the scalar product remains unchanged
     **/
    class ReduceScoresLattice : public RescoreLattice {
	typedef RescoreLattice Precursor;
    private:
	ScoreIdList fromIds_;
	ScoreId toId_;
	ScoreList scales_;
    protected:
	void reduce(ScoresRef sr) const {
	    Score score = sr->project(scales_);
	    for (ScoreIdList::const_iterator it = fromIds_.begin(); it != fromIds_.end(); ++it)
		sr->set(*it, Semiring::One);
	    sr->set(toId_, score);
	}
    public:
	ReduceScoresLattice(ConstLatticeRef l, const ScoreIdList &fromIds, ScoreId toId, RescoreMode rescoreMode) :
	    Precursor(l, rescoreMode), fromIds_(fromIds), toId_(toId) {
	    const ScoreList &srScales(l->semiring()->scales());
	    scales_.resize(srScales.size(), 0.0);
	    for (ScoreIdList::const_iterator it = fromIds_.begin(); it != fromIds_.end(); ++it)
		scales_[*it] = srScales[*it] / srScales[toId];
	}
	virtual ~ReduceScoresLattice() {}

	virtual void rescore(State *sp) const {
	    if (sp->isFinal())
		reduce(sp->weight_);
	    for (State::iterator a = sp->begin(); a != sp->end(); ++a)
		reduce(a->weight_);
	}

	virtual std::string describe() const {
	    return Core::form("reduce(%s)", fsa_->describe().c_str());
	}
    };
    class ReduceScoresNode : public RescoreNode {
	typedef RescoreNode Precursor;
    public:
	static const Core::ParameterStringVector paramKeys;
    private:
	KeyList keys_;
	ScoreIdList ids_;
	ConstSemiringRef lastSemiring_;
	ConstSemiringRef lastReducedSemiring_;
    protected:
	virtual ConstLatticeRef rescore(ConstLatticeRef l) {
	    ConstSemiringRef semiring = l->semiring();
	    if (!lastReducedSemiring_ || (lastSemiring_.get() != semiring.get())) {
		lastSemiring_ = semiring;
		lastReducedSemiring_ = cloneSemiring(semiring);
		ids_.resize(keys_.size());
		ScoreIdList::iterator itId = ids_.begin();
		for (KeyList::const_iterator itKey = keys_.begin(); itKey != keys_.end(); ++itKey, ++itId) {
		    *itId = semiring->id(*itKey);
		    if (!semiring->hasId(*itId))
			criticalError("Semiring \"%s\" has no key \"%s\"",
				      semiring->name().c_str(), itKey->c_str());
		    lastReducedSemiring_->setScale(*itId, 0.0);
		}
		lastReducedSemiring_->setScale(ids_.front(), semiring->scale(ids_.front()));
	    }
	    l = ConstLatticeRef(new ReduceScoresLattice(l, ids_, ids_[0], rescoreMode));
	    l = changeSemiring(l, lastReducedSemiring_);
	    return l;
	}
    public:
	ReduceScoresNode(const std::string &name, const Core::Configuration &config) :
	    Precursor(name, config) {}
	virtual ~ReduceScoresNode() {}

	void init(const std::vector<std::string> &arguments) {
	    keys_ = paramKeys(config);
	    if (keys_.empty())
		criticalError("ReduceScoresNode: Key list is empty.");
	}
    };
    const Core::ParameterStringVector ReduceScoresNode::paramKeys(
	"keys",
	"key identifiers");
    NodeRef createReduceScoresNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new ReduceScoresNode(name, config));
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    /**
     * f(x;const)
     **/
    template<class Func>
    class ArithmeticLattice : public RescoreLattice {
	typedef RescoreLattice Precursor;
    private:
	ScoreId id_;
	Func f_;
    public:
	ArithmeticLattice(ConstLatticeRef l, ScoreId id, Score c, RescoreMode rescoreMode) :
	    Precursor(l, rescoreMode), id_(id), f_(c) {}
	virtual ~ArithmeticLattice() {}

	virtual void rescore(State *sp) const {
	    if (sp->isFinal())
		sp->weight_->set(id_, f_(sp->weight_->get(id_)));
	    for (State::iterator a = sp->begin(); a != sp->end(); ++a)
		a->weight_->set(id_, f_(a->weight_->get(id_)));
	}

	virtual std::string describe() const {
	    return Core::form("%s(%s,dim=%zu,const=%f)", Func::describe().c_str(), fsa_->describe().c_str(), id_, f_.c);
	}
    };
    template<class Func>
    ConstLatticeRef func(ConstLatticeRef l, ScoreId id, Score c, RescoreMode rescoreMode) {
	if (c == Func::neutral())
	    return l;
	return ConstLatticeRef(new ArithmeticLattice<Func>(l, id, c, rescoreMode));
    }

    template<class Func>
    class ArithmeticNode : public RescoreSingleDimensionNode {
    public:
	static const Core::ParameterFloat paramScore;
    private:
	Score c_;
    protected:
	virtual ConstLatticeRef rescore(ConstLatticeRef l, ScoreId id) {
	    if (!l)
		return ConstLatticeRef();
	    return func<Func>(l, id, c_, rescoreMode);
	}
    public:
	ArithmeticNode(const std::string &name, const Core::Configuration &config) :
	    RescoreSingleDimensionNode(name, config) {}
	virtual ~ArithmeticNode() {}
	virtual void init(const std::vector<std::string> &arguments) {
	    Score scale = paramScale(config, Semiring::Invalid),
		score = paramScore(config, Semiring::Invalid);
	    if (scale == Semiring::Invalid)
		c_ = (score == Semiring::Invalid) ? Func::def() : score;
	    else {
		if (score != Semiring::Invalid)
		    error("ArithmeticNode(%s): Scale (%f) and score (%f) given, use scale",
			  Func::describe().c_str(), scale, score);
		c_ = scale;
	    }
	}
    };
    template<class Func>
    const Core::ParameterFloat ArithmeticNode<Func>::paramScore(
	"score",
	"score",
	Semiring::Invalid);
    template<class Func>
    NodeRef createFuncNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new ArithmeticNode<Func>(name, config));
    }

    struct Add {
	const Score c;
	Add(Score c) : c(c) {}
	Score operator() (Score x) const { return x + c; }
	static std::string describe() { return "add"; }
	static Score def() { return 0.0; }
	static Score neutral() { return 0.0; }
    };
    ConstLatticeRef add(ConstLatticeRef l, ScoreId id, Score c, RescoreMode rescoreMode) {
	return func<Add>(l, id, c, rescoreMode);
    }
    NodeRef createAddNode(const std::string &name, const Core::Configuration &config) {
	return createFuncNode<Add>(name, config);
    }

    struct Multiply {
	const Score c;
	Multiply(Score c) : c(c) {}
	Score operator() (Score x) const { return c * x; }
	static std::string describe() { return "multiply"; }
	static Score def() { return 1.0; }
	static Score neutral() { return 1.0; }
    };
    ConstLatticeRef multiply(ConstLatticeRef l, ScoreId id, Score c, RescoreMode rescoreMode) {
	return func<Multiply>(l, id, c, rescoreMode);
    }
    NodeRef createMultiplyNode(const std::string &name, const Core::Configuration &config) {
	return createFuncNode<Multiply>(name, config);
    }

    struct Exp {
	const Score c;
	Exp(Score c) : c(c) {}
	Score operator() (Score x) const {
	    if (x == 0.0)
		return 1.0;
	    if (x == Core::Type<Score>::max)
		return (c < 0.0) ? 0.0 : Core::Type<Score>::max;
	    if (x == Core::Type<Score>::min)
		return (c < 0.0) ? Core::Type<Score>::max : 0.0;
	    return ::exp(c * x);
	}
	static std::string describe() { return "power"; }
	static Score def() { return 1.0; }
	static Score neutral() { return Semiring::Invalid; }
    };
    ConstLatticeRef exp(ConstLatticeRef l, ScoreId id, Score c, RescoreMode rescoreMode) {
	return func<Exp>(l, id, c, rescoreMode);
    }
    NodeRef createExpNode(const std::string &name, const Core::Configuration &config) {
	return createFuncNode<Exp>(name, config);
    }

    struct Log {
	const Score c;
	Log(Score c) : c(c) {}
	Score operator() (Score x) const {
	    if (x == 1.0)
		return 0.0;
	    if (x == 0.0)
		return (c < 0.0) ? Core::Type<Score>::max : Core::Type<Score>::min;
	    return c * ::log(x);
	}
	static std::string describe() { return "log"; }
	static Score def() { return 1.0; }
	static Score neutral() { return Semiring::Invalid; }
    };
    ConstLatticeRef log(ConstLatticeRef l, ScoreId id, Score c, RescoreMode rescoreMode) {
	return func<Log>(l, id, c, rescoreMode);
    }
    NodeRef createLogNode(const std::string &name, const Core::Configuration &config) {
	return createFuncNode<Log>(name, config);
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    /**
     * extend non-eps arcs by fixed score
     **/
    class ExtendByPenaltyLattice : public RescoreLattice {
	typedef RescoreLattice Precursor;
    private:
	Score penalty_;
	ScoreId id_;
	Score scale_;
    public:
	ExtendByPenaltyLattice(ConstLatticeRef l, Score penalty, ScoreId id, Score scale, RescoreMode rescoreMode) :
	    Precursor(l, rescoreMode), penalty_(penalty), id_(id), scale_(scale) {}
	virtual ~ExtendByPenaltyLattice() {}

	virtual void rescore(State *sp) const {
	    for (State::iterator a = sp->begin(); a != sp->end(); ++a) {
		Fsa::LabelId labelId = a->output();
		if ((Fsa::FirstLabelId <= labelId) && (labelId <= Fsa::LastLabelId))
		    a->weight_->add(id_, scale_ * penalty_);
	    }
	}

	virtual std::string describe() const {
	    return Core::form("extendByPenalty(%s,dim=%zu,penalty=%f)", fsa_->describe().c_str(), id_, penalty_);
	}
    };


    /**
     * extend by lemma specific penalty
     **/
    typedef std::vector<std::string> StringList;
    typedef std::vector<StringList> StringListList;

    class PenaltyMap : public Core::hash_map<Fsa::LabelId, Score>, public Core::ReferenceCounted {
	typedef Core::hash_map<Fsa::LabelId, Score> Precursor;
    public:
	PenaltyMap() : Precursor() {}
    };
    typedef Core::Ref<PenaltyMap> PenaltyMapRef;

    PenaltyMapRef buildPenaltyMap(const StringListList &orthClasses, const ScoreList &orthClassPenalties) {
	require(orthClasses.size() == orthClassPenalties.size());
	Lexicon *lexicon = Lexicon::us().get();
	PenaltyMap *penalties = new PenaltyMap;
	ScoreList::const_iterator itPenalty = orthClassPenalties.begin();
	for (StringListList::const_iterator itSS = orthClasses.begin();
	     itSS != orthClasses.end(); ++itSS, ++itPenalty)
	    for (StringList::const_iterator itS = itSS->begin();
		 itS != itSS->end(); ++itS) {
		Fsa::LabelId label = lexicon->lemmaId(*itS);
		if (label != Fsa::InvalidLabelId)
		    penalties->insert(std::make_pair(label, *itPenalty));
		else
		    Core::Application::us()->error(
			"PenaltyMap: Could not find a lemma for orthography \"%s\".",
			itS->c_str());
	    }
	return PenaltyMapRef(penalties);
    }

    class ExtendByLemmaSpecificPenaltyLattice : public RescoreLattice {
	typedef RescoreLattice Precursor;
    private:
	Score defaultPenalty_;
	PenaltyMapRef penalties_;
	ScoreId id_;
	Score scale_;
	Core::Ref<const Bliss::LemmaAlphabet> lAlphabet_;
	Core::Ref<const Bliss::LemmaPronunciationAlphabet> lpAlphabet_;
    protected:
	Fsa::LabelId getLemmaId(Fsa::LabelId labelId) const {
	    if (lAlphabet_)
		return labelId;
	    else
		return lpAlphabet_->lemmaPronunciation(labelId)->lemma()->id();
	}
    public:
	ExtendByLemmaSpecificPenaltyLattice(ConstLatticeRef l, Score defaultPenalty, PenaltyMapRef penalties, ScoreId id, Score scale, RescoreMode rescoreMode) :
	    Precursor(l, rescoreMode), defaultPenalty_(defaultPenalty), penalties_(penalties), id_(id), scale_(scale) {
	    require(penalties);
	    lAlphabet_ = Lexicon::us()->lemmaAlphabet();
	    lpAlphabet_ = Lexicon::us()->lemmaPronunciationAlphabet();
	    if (l->getOutputAlphabet() == lAlphabet_)
		lpAlphabet_.reset();
	    else if (l->getOutputAlphabet() == lpAlphabet_)
		lAlphabet_.reset();
	    else defect();
	}
	virtual ~ExtendByLemmaSpecificPenaltyLattice() {}

	virtual void rescore(State *sp) const {
	    for (State::iterator a = sp->begin(); a != sp->end(); ++a) {
		Fsa::LabelId labelId = a->output();
		if ((Fsa::FirstLabelId <= labelId) && (labelId <= Fsa::LastLabelId)) {
		    const Bliss::Lemma *lemma = (lAlphabet_) ?
			lAlphabet_->lemma(labelId) :
			lpAlphabet_->lemmaPronunciation(labelId)->lemma();
		    require_(lemma);
		    PenaltyMap::const_iterator it = penalties_->find(lemma->id());
		    if (it != penalties_->end())
			a->weight_->add(id_, scale_ * it->second);
		    else
			a->weight_->add(id_, scale_ * defaultPenalty_);
		}
	    }
	}

	ConstBoundariesRef getBoundaries() const {
	    return fsa_->getBoundaries();
	}

	virtual std::string describe() const {
	    return "extendByLemmaSpecificPenalties(" + fsa_->describe() + ",dim=" + Core::itoa(id_) + ")";
	}
    };


    class ExtendByPenaltyNode : public RescoreSingleDimensionNode {
    private:
	Score defaultPenalty_;
	PenaltyMapRef penalties_;
    public:
	typedef std::vector<std::string> StringList;
	static const Core::ParameterFloat paramPenalty;
	static const Core::ParameterStringVector paramPenaltyClasses;
	static const Core::ParameterString paramOrthographies;
    protected:
	virtual ConstLatticeRef rescore(ConstLatticeRef l, ScoreId id) {
	    if (penalties_) {
		l = mapOutputToLemmaOrLemmaPronunciation(l);
		return ConstLatticeRef(
		    new ExtendByLemmaSpecificPenaltyLattice(l, defaultPenalty_, penalties_, id, scale(), rescoreMode));
	    } else {
		if (defaultPenalty_ != 0.0)
		    return ConstLatticeRef(new ExtendByPenaltyLattice(l, defaultPenalty_, id, scale(), rescoreMode));
	    }
	    return l;
	}
    public:
	ExtendByPenaltyNode(const std::string &name, const Core::Configuration &config) :
	    RescoreSingleDimensionNode(name, config) {}
	~ExtendByPenaltyNode() {}
	virtual void init(const std::vector<std::string> &arguments) {
	    defaultPenalty_ = paramPenalty(config);
	    const Core::Configuration mapConfig(config, "mapping");
	    StringList classes = paramPenaltyClasses(mapConfig);
	    if (!classes.empty()) {
		StringListList orthClasses;
		ScoreList penalties;
		for (StringList::const_iterator it = classes.begin(); it != classes.end(); ++it) {
		    const Core::Configuration classConfig(mapConfig, *it);
		    std::string orthClassStr = paramOrthographies(classConfig);
		    if (!orthClassStr.empty()) {
			orthClasses.push_back(StringList());
			Core::strconv(orthClassStr, orthClasses.back());
			penalties.push_back(paramPenalty(classConfig));
		    }
		}
		penalties_ = buildPenaltyMap(orthClasses, penalties);
	    }
	}
    };
    const Core::ParameterFloat ExtendByPenaltyNode::paramPenalty(
	"penalty",
	"penalty",
	Semiring::One);
    const Core::ParameterStringVector ExtendByPenaltyNode::paramPenaltyClasses(
	"classes",
	"penalty classes");
    const Core::ParameterString ExtendByPenaltyNode::paramOrthographies(
	"orth",
	"list of orthographic forms",
	"");

    NodeRef createExtendByPenaltyNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new ExtendByPenaltyNode(name, config));
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    /**
     * extend by pronunciation scores
     **/
    class ExtendByPronunciationScoreLattice : public RescoreLattice {
	typedef RescoreLattice Precursor;
    private:
	Core::Ref<const Bliss::LemmaPronunciationAlphabet> lpAlphabet_;
	ScoreId id_;
	Score scale_;
    public:
	ExtendByPronunciationScoreLattice(
	    ConstLatticeRef l,
	    Core::Ref<const Bliss::LemmaPronunciationAlphabet> lpAlphabet,
	    ScoreId id,
	    Score scale,
	    RescoreMode rescoreMode) :
	    Precursor(l, rescoreMode), lpAlphabet_(lpAlphabet), id_(id), scale_(scale) {
	    require(lpAlphabet);
	}
	virtual ~ExtendByPronunciationScoreLattice() {}

	virtual void rescore(State *sp) const {
	    for (State::iterator a = sp->begin(); a != sp->end(); ++a) {
		Fsa::LabelId labelId = a->output();
		if ((Fsa::FirstLabelId <= labelId) && (labelId <= Fsa::LastLabelId))
		    a->weight_->add(id_, scale_ * lpAlphabet_->lemmaPronunciation(a->output())->pronunciationScore());
	    }
	}

	ConstBoundariesRef getBoundaries() const {
	    return fsa_->getBoundaries();
	}

	virtual std::string describe() const {
	    return Core::form("extendByPronunciation(%s,dim=%zu)", fsa_->describe().c_str(), id_);
	}
    };

    ConstLatticeRef extendByPronunciationScore(
	ConstLatticeRef l,
	Core::Ref<const Bliss::LemmaPronunciationAlphabet> lpAlphabet,
	ScoreId id,  Score scale,
	RescoreMode rescoreMode) {
	l = mapOutput(l, MapToLemmaPronunciation);
	return ConstLatticeRef(
	    new ExtendByPronunciationScoreLattice(l, lpAlphabet, id, scale, rescoreMode));
    }

    class ExtendByPronunciationScoreNode : public RescoreSingleDimensionNode {
    protected:
	virtual ConstLatticeRef rescore(ConstLatticeRef l, ScoreId id) {
	    return extendByPronunciationScore(l, Lexicon::us()->lemmaPronunciationAlphabet(), id, scale(), rescoreMode);
	}
    public:
	ExtendByPronunciationScoreNode(const std::string &name, const Core::Configuration &config) :
	    RescoreSingleDimensionNode(name, config) {}
	~ExtendByPronunciationScoreNode() {}
    };

    NodeRef createExtendByPronunciationScoreNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new ExtendByPronunciationScoreNode(name, config));
    }
    // -------------------------------------------------------------------------

} // namespace Flf
