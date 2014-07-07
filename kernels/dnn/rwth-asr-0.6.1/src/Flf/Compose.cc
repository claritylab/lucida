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
#include <Fsa/Arithmetic.hh>
#include <Fsa/Basic.hh>
#include <Fsa/Input.hh>
#include <Fsa/Static.hh>

#include "FlfCore/Basic.hh"
#include "FlfCore/Ftl.hh"
#include "FlfCore/LatticeInternal.hh"
#include "Compose.hh"
#include "Convert.hh"
#include "Copy.hh"
#include "Lexicon.hh"
#include "Map.hh"
#include "RescoreInternal.hh"

namespace Flf {

    // -------------------------------------------------------------------------
    class UnweightLattice : public SlaveLattice {
	typedef SlaveLattice Precursor;
    private:
	ConstSemiringRef semiring_;
	ScoresRef one_;

    public:
	UnweightLattice(ConstLatticeRef l, ConstSemiringRef semiring) :
	    Precursor(l), semiring_(semiring), one_(semiring->one()) {
	    setBoundaries(InvalidBoundaries);
	}
	virtual ~UnweightLattice() {}

	ConstSemiringRef semiring() const {
	    return semiring_;
	}

	ConstStateRef getState(Fsa::StateId sid) const {
	    State *sp = new State(*fsa_->getState(sid));
	    if (sp->isFinal())
		sp->weight_ = one_;
	    for (State::iterator a = sp->begin(), end = sp->end(); a != end; ++a)
		a->weight_ = one_;
	    return ConstStateRef(sp);
	}
	std::string describe() const
	    { return Core::form("unweight(%s)", fsa_->describe().c_str()); }
    };
    ConstLatticeRef unweight(ConstLatticeRef l, ConstSemiringRef semiring) {
	if (!semiring)
	    semiring = l->semiring();
	return ConstLatticeRef(new UnweightLattice(l, semiring));
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class ComposeNode : public Node {
	typedef Node Precursor;
    public:
	static const Core::ParameterBool paramUnweightLeft;
	static const Core::ParameterBool paramUnweightRight;
    private:
	bool unweightLeft_;
	bool unweightRight_;
    protected:
	virtual bool init_(NetworkCrawler &crawler, const std::vector<std::string> &arguments) {
	    bool b = Precursor::init_(crawler, arguments);
	    if (!connected(0))
		criticalError("ComposeNode: Need a data source at port 0.");
	    if (!connected(1))
		criticalError("ComposeNode: Need a data source at port 1.");
	    unweightLeft_ = paramUnweightLeft(config);
	    unweightRight_ = paramUnweightRight(config);
	    if (unweightLeft_ && unweightRight_)
		criticalError("ComposeNode: try to unweight both lattices");
	    return b;
	}
	virtual ConstLatticeRef compose(ConstLatticeRef l, ConstLatticeRef r) = 0;
    public:
	ComposeNode(const std::string &name, const Core::Configuration &config) :
	    Node(name, config) {
	}
	virtual ~ComposeNode() {}

	virtual ConstLatticeRef sendLattice(Port to) {
	    ConstLatticeRef l = requestLattice(0);
	    ConstLatticeRef r = requestLattice(1);
	    if (!l || !r) {
		warning("ComposeNode: At least one of the lattices to be composed is empty.");
		return ConstLatticeRef();
	    }
	    if (unweightLeft_)
		l = unweight(l, r->semiring());
	    else if (unweightRight_)
		r = unweight(r, l->semiring());
	    return compose(l, r);
	}
    };
    const Core::ParameterBool ComposeNode::paramUnweightLeft(
	"unweight-left",
	"make left lattice an unweighted transducer",
	false);
    const Core::ParameterBool ComposeNode::paramUnweightRight(
	"unweight-right",
	"make right lattice an unweighted transducer",
	false);
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    ConstLatticeRef unifySemirings(ConstLatticeRef l, ConstLatticeRef r) {
	if (l->semiring().get() != r->semiring().get()) {
	    if (*l->semiring() == *r->semiring())
		r = changeSemiring(r, l->semiring());
	    else {
		// TODO
		defect();
	    }
	}
	return r;
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    void composeBoundaries(ConstLatticeRef composition, ConstLatticeRef l, ConstLatticeRef r) {
	ConstBoundariesRef b = InvalidBoundaries;
	if (l->getBoundaries()->valid()) {
	    if (r->getBoundaries()->valid()) {
		if (l->getBoundaries() != r->getBoundaries()) {
		    Core::Application::us()->warning(
			"In \"%s\": Both child lattices define boundaries, "
			"keep boundaries from the left lattice.",
			composition->describe().c_str());
		    b = ConstBoundariesRef(new MappedBoundaries(
					       l->getBoundaries(),
					       FtlWrapper::mapToLeft(composition)));
		} else
		    b = ConstBoundariesRef(new MappedBoundaries(
					       l->getBoundaries(),
					       FtlWrapper::mapToLeft(composition)));
	    } else
		b = ConstBoundariesRef(new MappedBoundaries(
					   l->getBoundaries(),
					   FtlWrapper::mapToLeft(composition)));
	} else if (r->getBoundaries()->valid()) {
	    b = ConstBoundariesRef(new MappedBoundaries(
				       r->getBoundaries(),
				       FtlWrapper::mapToRight(composition)));
	}
	composition->setBoundaries(b);
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    /**
     * compose two lattices
     **/
    ConstLatticeRef composeMatching(ConstLatticeRef l, ConstLatticeRef r) {
	r = unifySemirings(l, r);
	ConstLatticeRef k = FtlWrapper::composeMatching(l, r);
	composeBoundaries(k, l, r);
	return k;
    }

    class ComposeMatchingNode : public ComposeNode {
    protected:
	virtual ConstLatticeRef compose(ConstLatticeRef l, ConstLatticeRef r) {
	    return composeMatching(l, r);
	}
    public:
	ComposeMatchingNode(const std::string &name, const Core::Configuration &config) :
	    ComposeNode(name, config) {}
	~ComposeMatchingNode() {}
    };

    NodeRef createComposeMatchingNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new ComposeMatchingNode(name, config));
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    /**
     * compose two lattices
     **/
    ConstLatticeRef composeSequencing(ConstLatticeRef l, ConstLatticeRef r) {
	r = unifySemirings(l, r);
	ConstLatticeRef k = FtlWrapper::composeSequencing(l, r);
	composeBoundaries(k, l, r);
	return k;
    }

    class ComposeSequencingNode : public ComposeNode {
    protected:
	virtual ConstLatticeRef compose(ConstLatticeRef l, ConstLatticeRef r) {
	    return composeSequencing(l, r);
	}
    public:
	ComposeSequencingNode(const std::string &name, const Core::Configuration &config) :
	    ComposeNode(name, config) {}
	~ComposeSequencingNode() {}
    };

    NodeRef createComposeSequencingNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new ComposeSequencingNode(name, config));
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    /**
     * build difference of two lattices
     **/
    ConstLatticeRef differ(ConstLatticeRef l, ConstLatticeRef r) {
	r = unifySemirings(l, r);
	ConstLatticeRef k = FtlWrapper::difference(l, r);
	composeBoundaries(k, l, r);
	return k;
    }

    class DifferenceNode : public ComposeNode {
    protected:
	virtual ConstLatticeRef compose(ConstLatticeRef l, ConstLatticeRef r) {
	    return differ(l, r);
	}
    public:
	DifferenceNode(const std::string &name, const Core::Configuration &config) :
	    ComposeNode(name, config) {}
	~DifferenceNode() {}
    };

    NodeRef createDifferenceNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new DifferenceNode(name, config));
    }
    // -------------------------------------------------------------------------



    // -------------------------------------------------------------------------
    /**
     * intersect two lattices
     **/
    ConstLatticeRef intersect(ConstLatticeRef l, ConstLatticeRef r, bool appendScores) {
	if (appendScores) {
	    ConstSemiringRef semiring = appendSemiring(l->semiring(), r->semiring());
	    r = offsetSemiring(r, semiring, l->semiring()->size());
	    l = offsetSemiring(l, semiring, 0);
	} else
	    r = unifySemirings(l, r);
	if (r->type() != Fsa::TypeAcceptor)
	    r = projectOutput(r);
	ConstLatticeRef k = composeSequencing(l, r);
	return k;
    }

    class IntersectionNode : public ComposeNode {
    public:
	static const Core::ParameterBool paramAppend;
    private:
	bool append_;
    protected:
	virtual ConstLatticeRef compose(ConstLatticeRef l, ConstLatticeRef r) {
	    return intersect(l, r, append_);
	}
    public:
	IntersectionNode(const std::string &name, const Core::Configuration &config) :
	    ComposeNode(name, config) {
	    append_ = paramAppend(config);
	}
	~IntersectionNode() {}
    };
    const Core::ParameterBool IntersectionNode::paramAppend(
	"append",
	"append scores instead of extend",
	false);

    NodeRef createIntersectionNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new IntersectionNode(name, config));
    }
    // -------------------------------------------------------------------------



    // -------------------------------------------------------------------------
    /**
     * compose with fsa
     **/
    ConstLatticeRef composeWithFsa(ConstLatticeRef l, Fsa::ConstAutomatonRef f, Score scale) {
	ConstSemiringRef semiring = appendSemiring(l->semiring(), scale);
	l = offsetSemiring(l, semiring, 0);
	return composeWithFsa(l, f, semiring->size() - 1, Semiring::DefaultScale);
    }

    ConstLatticeRef composeWithFsa(ConstLatticeRef l, Fsa::ConstAutomatonRef f, ScoreId id, Score scale) {
	l = mapOutput(l, Lexicon::us()->alphabetId(f->getInputAlphabet()));
	if (scale != Semiring::DefaultScale)
	    f = Fsa::multiply(f, Fsa::Weight(scale));
	return composeSequencing(
	    l, fromFsa(f, l->semiring(), id, l->semiring()->one()));
    }

    class ComposeWithFsaNode : public RescoreSingleDimensionNode {
    public:
	static const Core::ParameterString paramFile;
	static const Core::ParameterString paramName;
    private:
	Lexicon::AlphabetId inputAlphabetId_;
	Lexicon::AlphabetId outputAlphabetId_;
	Fsa::ConstAutomatonRef fsa_;
	ConstLatticeRef cachedL2_;
    protected:
	virtual ConstLatticeRef rescore(ConstLatticeRef l1, ScoreId id) {
	    ConstLatticeRef l2;
	    if (connected(1)) {
		Fsa::ConstAutomatonRef fsa = Fsa::ConstAutomatonRef(static_cast<const Fsa::Automaton*>(requestData(1)));
		fsa->releaseReference();
		if (scale() != Semiring::DefaultScale)
		    fsa = Fsa::multiply(fsa, Fsa::Weight(scale()));
		l2 = fromFsa(fsa_, l1->semiring(), l1->semiring()->one(), id, inputAlphabetId_, outputAlphabetId_);
	    } else {
		verify(fsa_);
		if (!cachedL2_ || (l1->semiring().get() != cachedL2_->semiring().get())) {
		    cachedL2_ = persistent(fromFsa(fsa_, l1->semiring(), l1->semiring()->one(), id, inputAlphabetId_, outputAlphabetId_));
		}
		l2 = cachedL2_;
	    }
	    return composeSequencing(l1, l2);
	}

	virtual void setRescoreMode(RescoreMode _rescoreMode) {
	    if (_rescoreMode != RescoreModeClone)
		warning("Compose supports only rescoring by scores cloning.");
	    rescoreMode = RescoreModeClone;
	}

    public:
	ComposeWithFsaNode(const std::string &name, const Core::Configuration &config) :
	    RescoreSingleDimensionNode(name, config) {}
	virtual ~ComposeWithFsaNode() {}

	virtual void init(const std::vector<std::string> &arguments) {
	    std::string alphabetName = paramName(select("alphabet"), "");
	    if (alphabetName.empty()) {
		inputAlphabetId_ = Lexicon::us()->alphabetId(paramName(select("input-alphabet")), true);
		outputAlphabetId_ = Lexicon::us()->alphabetId(paramName(select("output-alphabet")), true);
	    } else
		inputAlphabetId_ = outputAlphabetId_ = Lexicon::us()->alphabetId(alphabetName, true);
	    if (!connected(1)) {
		std::string filename = paramFile(config);
		if (filename.empty())
		    criticalError("ComposeWithFsaNode: Need either input at port 1 or a valid filename");
		fsa_ = Fsa::read(filename);
		if (!fsa_)
		    criticalError("ComposeWithFsaNode: Could not load fsa \"%s\"",
				  filename.c_str());
		if (scale() != Semiring::DefaultScale)
		    fsa_ = Fsa::staticCopy(Fsa::multiply(fsa_, Fsa::Weight(scale())));
		log("Compose with fsa \"%s\".", fsa_->describe().c_str());
	    } else
		log("Compose with fsa from port 1.");
	}
    };
    const Core::ParameterString ComposeWithFsaNode::paramFile(
	"file",
	"name of fsa file",
	"");
    const Core::ParameterString ComposeWithFsaNode::paramName(
	"name",
	"name",
	"lemma");

    NodeRef createComposeWithFsaNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new ComposeWithFsaNode(name, config));
    }
    // -------------------------------------------------------------------------

} // namespace Flf
