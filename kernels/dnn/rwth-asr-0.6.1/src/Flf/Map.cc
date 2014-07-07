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
#include <Core/StringUtilities.hh>
#include <Fsa/Arithmetic.hh>
#include <Fsa/Compose.hh>
#include <Fsa/Project.hh>

#include "FlfCore/Basic.hh"
#include "FlfCore/Ftl.hh"
#include "FlfCore/LatticeInternal.hh"
#include "FlfCore/Traverse.hh"
#include "FlfCore/Utility.hh"
#include "Compose.hh"
#include "Lexicon.hh"
#include "Map.hh"

namespace Flf {

    // -------------------------------------------------------------------------
    class TransducerLattice : public SlaveLattice {
	typedef SlaveLattice Precursor;
    public:
	TransducerLattice(ConstLatticeRef l) : Precursor(l) {}
	virtual ~TransducerLattice() {}
	virtual Fsa::Type type() const { return Fsa::TypeTransducer; }
	virtual Fsa::ConstAlphabetRef getOutputAlphabet() const { return fsa_->getInputAlphabet(); }
	virtual std::string describe() const { return Core::form("transducer(%s)", fsa_->describe().c_str()); }
    };
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    ConstLatticeRef transducer(ConstLatticeRef l) {
	if (l->type() != Fsa::TypeAcceptor) return l;
	return ConstLatticeRef(new TransducerLattice(l));
    }

    ConstLatticeRef projectInput(ConstLatticeRef l) {
	if (l->type() == Fsa::TypeAcceptor)
	    return l;
	return FtlWrapper::projectInput(l);
    }

    ConstLatticeRef projectOutput(ConstLatticeRef l) {
	if (l->type() == Fsa::TypeAcceptor)
	    return l;
	return FtlWrapper::projectOutput(l);
    }

    ConstLatticeRef invert(ConstLatticeRef l) {
	if (l->type() == Fsa::TypeAcceptor)
	    return l;
	return FtlWrapper::invert(l);
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    /**
     * map alphabet
     **/
    const Core::Choice alphabetMappingActionChoice(
	"undefined",               UndefinedAlphabetMapping,
	"to-phoneme",              MapToPhoneme,
	"to-lemma-pron",           MapToLemmaPronunciation,
	"to-preferred-lemma-pron", MapToPreferredLemmaPronunciation,
	"to-lemma",                MapToLemma,
	"to-synt",                 MapToSyntacticTokenSequence,
	"to-eval",                 MapToEvaluationTokenSequences,
	"to-preferred-eval",       MapToPreferredEvaluationTokenSequence,
	Core::Choice::endMark());

    AlphabetMappingAction getAlphabetMappingAction(const std::string &name) {
	Core::Choice::Value c = alphabetMappingActionChoice[name];
	return c == Core::Choice::IllegalValue ? UndefinedAlphabetMapping : AlphabetMappingAction(c);
    }

    const std::string & getAlphabetMappingActionName(AlphabetMappingAction action) {
	return alphabetMappingActionChoice[action];
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    /**
     * The following mappings guarantee mapping success, i.e. no arcs labeled
     * Fsa::InvalidLabelId are produced, if the lexicon's read-only flag is
     * not set.
    **/
    class MapInputFromLemmaPronunciationToLemmaLattice : public ModifyLattice {
	typedef ModifyLattice Precursor;
    private:
	bool isAcceptor_;
	Core::Ref<const Bliss::LemmaPronunciationAlphabet> lpAlphabet_;
    public:
	MapInputFromLemmaPronunciationToLemmaLattice(ConstLatticeRef l) : Precursor(l) {
	    isAcceptor_ = (l->type() == Fsa::TypeAcceptor);
	    lpAlphabet_ = Lexicon::us()->lemmaPronunciationAlphabet();
	}
	virtual ~MapInputFromLemmaPronunciationToLemmaLattice() {}
	virtual void modifyState(State *sp) const {
	    if (isAcceptor_) {
		for (State::iterator a = sp->begin(); a != sp->end(); ++a)
		    if ((Fsa::FirstLabelId <= a->input()) && (a->input() <= Fsa::LastLabelId))
			a->input_ = a->output_ = lpAlphabet_->lemmaPronunciation(a->input_)->lemma()->id();
	    } else {
		for (State::iterator a = sp->begin(); a != sp->end(); ++a)
		    if ((Fsa::FirstLabelId <= a->input()) && (a->input() <= Fsa::LastLabelId))
			a->input_ = lpAlphabet_->lemmaPronunciation(a->input_)->lemma()->id();
	    }
	}
	virtual Fsa::ConstAlphabetRef getInputAlphabet() const {
	    return Lexicon::us()->lemmaAlphabet();
	}
	virtual Fsa::ConstAlphabetRef getOutputAlphabet() const {
	    if  (isAcceptor_)
		return Lexicon::us()->lemmaAlphabet();
	    else
		return fsa_->getOutputAlphabet();
	}
	virtual std::string describe() const {
	    return "mapInputFromLemmaPronunciationToLemma(" + fsa_->describe() + ")";
	}
    };

    class MapOutputFromLemmaPronunciationToLemmaLattice : public ModifyLattice {
	typedef ModifyLattice Precursor;
    private:
	Core::Ref<const Bliss::LemmaPronunciationAlphabet> lpAlphabet_;
    public:
	MapOutputFromLemmaPronunciationToLemmaLattice(ConstLatticeRef l) : Precursor(l) {
	    verify(l->type() != Fsa::TypeAcceptor);
	    lpAlphabet_ = Lexicon::us()->lemmaPronunciationAlphabet();
	}
	virtual ~MapOutputFromLemmaPronunciationToLemmaLattice() {}
	virtual void modifyState(State *sp) const {
	    for (State::iterator a = sp->begin(); a != sp->end(); ++a) {
		Fsa::LabelId labelId = a->output();
		if ((Fsa::FirstLabelId <= labelId) && (labelId <= Fsa::LastLabelId))
		    a->output_ = lpAlphabet_->lemmaPronunciation(labelId)->lemma()->id();
	    }
	}
	virtual Fsa::ConstAlphabetRef getOutputAlphabet() const {
	    return Lexicon::us()->lemmaAlphabet();
	}
	virtual std::string describe() const {
	    return "mapOutputFromLemmaPronunciationToLemma(" + fsa_->describe() + ")";
	}
    };

    class MapInputFromLemmaToSingleLemmaPronunciationLattice : public ModifyLattice {
	typedef ModifyLattice Precursor;
    private:
	bool isAcceptor_;
	Lexicon *lexicon_;
	Core::Ref<const Bliss::LemmaAlphabet> lAlphabet_;
    public:
	MapInputFromLemmaToSingleLemmaPronunciationLattice(ConstLatticeRef l) : Precursor(l) {
	    isAcceptor_ = (l->type() == Fsa::TypeAcceptor);
	    lexicon_ = Lexicon::us().get();
	    lAlphabet_ = Lexicon::us()->lemmaAlphabet();
	}
	virtual ~MapInputFromLemmaToSingleLemmaPronunciationLattice() {}
	virtual void modifyState(State *sp) const {
	    if (isAcceptor_) {
		for (State::iterator a = sp->begin(); a != sp->end(); ++a)
		    if ((Fsa::FirstLabelId <= a->input()) && (a->input() <= Fsa::LastLabelId)) {
			const Bliss::LemmaPronunciation *lp = lexicon_->lemmaPronunciation(lAlphabet_->lemma(a->input_), 0);
			a->input_ = a->output_ = (lp) ? Fsa::LabelId(lp->id()) : Fsa::InvalidLabelId;
		    }
	    } else {
		for (State::iterator a = sp->begin(); a != sp->end(); ++a)
		    if ((Fsa::FirstLabelId <= a->input()) && (a->input() <= Fsa::LastLabelId)) {
			const Bliss::LemmaPronunciation *lp = lexicon_->lemmaPronunciation(lAlphabet_->lemma(a->input_), 0);
			a->input_ = (lp) ? Fsa::LabelId(lp->id()) : Fsa::InvalidLabelId;
		    }
	    }
	}
	virtual Fsa::ConstAlphabetRef getInputAlphabet() const {
	    return Lexicon::us()->lemmaPronunciationAlphabet();
	}
	virtual Fsa::ConstAlphabetRef getOutputAlphabet() const {
	    if  (isAcceptor_)
		return Lexicon::us()->lemmaPronunciationAlphabet();
	    else
		return fsa_->getOutputAlphabet();
	}
	virtual std::string describe() const {
	    return "mapInputFromLemmaToSingleLemmaPronunciation(" + fsa_->describe() + ")";
	}
    };

    class MapOutputFromLemmaToSingleLemmaPronunciationLattice : public ModifyLattice {
	typedef ModifyLattice Precursor;
    private:
	Lexicon *lexicon_;
	Core::Ref<const Bliss::LemmaAlphabet> lAlphabet_;
    public:
	MapOutputFromLemmaToSingleLemmaPronunciationLattice(ConstLatticeRef l) : Precursor(l) {
	    verify(l->type() != Fsa::TypeAcceptor);
	    lexicon_ = Lexicon::us().get();
	    lAlphabet_ = Lexicon::us()->lemmaAlphabet();
	}
	virtual ~MapOutputFromLemmaToSingleLemmaPronunciationLattice() {}
	virtual void modifyState(State *sp) const {
	    for (State::iterator a = sp->begin(); a != sp->end(); ++a)
		if ((Fsa::FirstLabelId <= a->output()) && (a->output() <= Fsa::LastLabelId)) {
		    const Bliss::LemmaPronunciation *lp = lexicon_->lemmaPronunciation(lAlphabet_->lemma(a->output_), 0);
		    a->output_ = (lp) ? Fsa::LabelId(lp->id()) : Fsa::InvalidLabelId;
		}
	}
	virtual Fsa::ConstAlphabetRef getOutputAlphabet() const {
	    return Lexicon::us()->lemmaPronunciationAlphabet();
	}
	virtual std::string describe() const {
	    return "mapOutputFromLemmaToSingleLemmaPronunciation(" + fsa_->describe() + ")";
	}
    };
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    ConstLatticeRef mapInput(Fsa::ConstAutomatonRef f, ConstLatticeRef l) {
	return composeSequencing(fromUnweightedFsa(f, l->semiring(), l->semiring()->one()), l);
    }

    ConstLatticeRef mapOutput(ConstLatticeRef l, Fsa::ConstAutomatonRef f) {
	return composeSequencing(l, fromUnweightedFsa(f, l->semiring(), l->semiring()->one()));
    }

    ConstLatticeRef mapInput(ConstLatticeRef l, Lexicon::AlphabetId alphabetId) {
	switch (alphabetId) {
	case Lexicon::PhonemeAlphabetId:
	    return mapInput(l, MapToPhoneme);
	case Lexicon::LemmaPronunciationAlphabetId:
	    return mapInput(l, MapToLemmaPronunciation);
	case Lexicon::LemmaAlphabetId:
	    return mapInput(l, MapToLemma);
	case Lexicon::SyntacticTokenAlphabetId:
	    return mapInput(l, MapToSyntacticTokenSequence);
	case Lexicon::EvaluationTokenAlphabetId:
	    return mapInput(l, MapToEvaluationTokenSequences);
	default:
	    defect();
	    return ConstLatticeRef();
	}
    }

    ConstLatticeRef mapOutput(ConstLatticeRef l, Lexicon::AlphabetId alphabetId) {
	switch (alphabetId) {
	case Lexicon::PhonemeAlphabetId:
	    return mapOutput(l, MapToPhoneme);
	case Lexicon::LemmaPronunciationAlphabetId:
	    return mapOutput(l, MapToLemmaPronunciation);
	case Lexicon::LemmaAlphabetId:
	    return mapOutput(l, MapToLemma);
	case Lexicon::SyntacticTokenAlphabetId:
	    return mapOutput(l, MapToSyntacticTokenSequence);
	case Lexicon::EvaluationTokenAlphabetId:
	    return mapOutput(l, MapToEvaluationTokenSequences);
	default:
	    defect();
	    return ConstLatticeRef();
	}
    }

    ConstLatticeRef mapInput(ConstLatticeRef l, AlphabetMappingAction action) {
	Lexicon::AlphabetId alphabetId = Lexicon::us()->alphabetId(l->getInputAlphabet());
	switch (alphabetId) {
	case Lexicon::PhonemeAlphabetId:
	    switch (action) {
	    case MapToPhoneme:
		return l;
	    case MapToLemmaPronunciation:
		return mapInput(
		    Fsa::invert(Lexicon::us()->phonemeToLemmaPronunciationTransducer()), l);
	    case MapToPreferredLemmaPronunciation:
		Core::Application::us()->criticalError(
		    "Not available for \"%s\" alphabet",
		    Lexicon::us()->alphabetName(alphabetId).c_str());
		return ConstLatticeRef();
	    case MapToLemma:
		return mapInput(
		    Fsa::invert(Lexicon::us()->lemmaPronunciationToLemmaTransducer()),
		    mapInput(Fsa::invert(Lexicon::us()->phonemeToLemmaPronunciationTransducer()), l));
	    case MapToSyntacticTokenSequence:
		return mapInput(
		    Fsa::invert(Lexicon::us()->lemmaToSyntacticTokenTransducer()),
		    mapInput(Fsa::invert(Lexicon::us()->lemmaPronunciationToLemmaTransducer()),
			     mapInput(Fsa::invert(Lexicon::us()->phonemeToLemmaPronunciationTransducer()), l)));
	    case MapToEvaluationTokenSequences:
		return mapInput(
		    Fsa::invert(Lexicon::us()->lemmaToEvaluationTokenTransducer()),
		    mapInput(Fsa::invert(Lexicon::us()->lemmaPronunciationToLemmaTransducer()),
			     mapInput(Fsa::invert(Lexicon::us()->phonemeToLemmaPronunciationTransducer()), l)));
	    case MapToPreferredEvaluationTokenSequence:
		return mapInput(
		    Fsa::invert(Lexicon::us()->lemmaToPreferredEvaluationTokenSequenceTransducer()),
		    mapInput(Fsa::invert(Lexicon::us()->lemmaPronunciationToLemmaTransducer()),
			     mapInput(Fsa::invert(Lexicon::us()->phonemeToLemmaPronunciationTransducer()), l)));
	    default:
		break;
	    }
	case Lexicon::LemmaPronunciationAlphabetId:
	    switch (action) {
	    case MapToPhoneme:
		return mapInput(
		    Lexicon::us()->phonemeToLemmaPronunciationTransducer(), l);
	    case MapToLemmaPronunciation:
	    case MapToPreferredLemmaPronunciation:
		return l;
	    case MapToLemma:
		return ConstLatticeRef(new MapInputFromLemmaPronunciationToLemmaLattice(l));
	    case MapToSyntacticTokenSequence:
		return mapInput(
		    Fsa::invert(Lexicon::us()->lemmaToSyntacticTokenTransducer()),
		    ConstLatticeRef(new MapInputFromLemmaPronunciationToLemmaLattice(l)));
	    case MapToEvaluationTokenSequences:
		return mapInput(
		    Fsa::invert(Lexicon::us()->lemmaToEvaluationTokenTransducer()),
		    ConstLatticeRef(new MapInputFromLemmaPronunciationToLemmaLattice(l)));
	    case MapToPreferredEvaluationTokenSequence:
		return mapInput(
		    Fsa::invert(Lexicon::us()->lemmaToPreferredEvaluationTokenSequenceTransducer()),
		    ConstLatticeRef(new MapInputFromLemmaPronunciationToLemmaLattice(l)));
	    default:
		break;
	    }
	case Lexicon::LemmaAlphabetId:
	    switch (action) {
	    case MapToPhoneme:
		return mapInput(
		    Lexicon::us()->phonemeToLemmaPronunciationTransducer(),
		    mapInput(Lexicon::us()->lemmaPronunciationToLemmaTransducer(), l));
	    case MapToLemmaPronunciation:
		return mapInput(
		    Lexicon::us()->lemmaPronunciationToLemmaTransducer(), l);
	    case MapToPreferredLemmaPronunciation:
		return ConstLatticeRef(new MapInputFromLemmaToSingleLemmaPronunciationLattice(l));
	    case MapToLemma:
		return l;
	    case MapToSyntacticTokenSequence:
		return mapInput(
		    Fsa::invert(Lexicon::us()->lemmaToSyntacticTokenTransducer()), l);
	    case MapToEvaluationTokenSequences:
		return mapInput(
		    Fsa::invert(Lexicon::us()->lemmaToEvaluationTokenTransducer()), l);
	    case MapToPreferredEvaluationTokenSequence:
		return mapInput(
		    Fsa::invert(Lexicon::us()->lemmaToPreferredEvaluationTokenSequenceTransducer()), l);
	    default:
		break;
	    }
	case Lexicon::SyntacticTokenAlphabetId:
	    switch (action) {
	    case MapToLemma:
		return mapInput(
		    Lexicon::us()->lemmaToSyntacticTokenTransducer(), l);
	    case MapToSyntacticTokenSequence:
		return l;
	    default:
		break;
	    }
	case Lexicon::EvaluationTokenAlphabetId:
	    switch (action) {
	    case MapToLemma:
		return mapInput(
		    Lexicon::us()->lemmaToEvaluationTokenTransducer(), l);
	    case MapToEvaluationTokenSequences:
	    case MapToPreferredEvaluationTokenSequence:
		return l;
	    default:
		break;
	    }
	default:
	    break;
	}
	Core::Application::us()->criticalError(
	    "The implementation of the \"%s\"-mapping for input alphabet \"%s\" is pending ...",
	    alphabetMappingActionChoice[action].c_str(), Lexicon::us()->alphabetName(alphabetId).c_str());
	return ConstLatticeRef();
    }

    ConstLatticeRef mapOutput(ConstLatticeRef l, AlphabetMappingAction action) {
	l = transducer(l);
	Lexicon::AlphabetId alphabetId = Lexicon::us()->alphabetId(l->getOutputAlphabet());
	switch (alphabetId) {
	case Lexicon::PhonemeAlphabetId:
	    switch (action) {
	    case MapToPhoneme:
		return l;
	    case MapToLemmaPronunciation:
		return mapOutput(
		    l, Lexicon::us()->phonemeToLemmaPronunciationTransducer());
	    case MapToPreferredLemmaPronunciation:
		Core::Application::us()->criticalError(
		    "Not available for \"%s\" alphabet",
		    Lexicon::us()->alphabetName(alphabetId).c_str());
		return ConstLatticeRef();
	    case MapToLemma:
		return mapOutput(
		    mapOutput(l, Lexicon::us()->phonemeToLemmaPronunciationTransducer()),
		    Lexicon::us()->lemmaPronunciationToLemmaTransducer());
	    case MapToSyntacticTokenSequence:
		return mapOutput(
		    mapOutput(
			mapOutput(l, Lexicon::us()->phonemeToLemmaPronunciationTransducer()),
			Lexicon::us()->lemmaPronunciationToLemmaTransducer()),
		    Lexicon::us()->lemmaToEvaluationTokenTransducer());
	    case MapToEvaluationTokenSequences:
		return mapOutput(
		    mapOutput(
			mapOutput(l, Lexicon::us()->phonemeToLemmaPronunciationTransducer()),
			Lexicon::us()->lemmaPronunciationToLemmaTransducer()),
		    Lexicon::us()->lemmaToEvaluationTokenTransducer());
	    case MapToPreferredEvaluationTokenSequence:
		return mapOutput(
		    mapOutput(
			mapOutput(l, Lexicon::us()->phonemeToLemmaPronunciationTransducer()),
			Lexicon::us()->lemmaPronunciationToLemmaTransducer()),
		    Lexicon::us()->lemmaToPreferredEvaluationTokenSequenceTransducer());
	    default:
		break;
	    }
	case Lexicon::LemmaPronunciationAlphabetId:
	    switch (action) {
	    case MapToPhoneme:
		return mapOutput(
		    l, Fsa::invert(Lexicon::us()->phonemeToLemmaPronunciationTransducer()));
	    case MapToLemmaPronunciation:
	    case MapToPreferredLemmaPronunciation:
		return l;
	    case MapToLemma:
		return ConstLatticeRef(new MapOutputFromLemmaPronunciationToLemmaLattice(l));
	    case MapToSyntacticTokenSequence:
		return mapOutput(
		    ConstLatticeRef(new MapOutputFromLemmaPronunciationToLemmaLattice(l)),
		    Lexicon::us()->lemmaToSyntacticTokenTransducer());
	    case MapToEvaluationTokenSequences:
		return mapOutput(
		    ConstLatticeRef(new MapOutputFromLemmaPronunciationToLemmaLattice(l)),
		    Lexicon::us()->lemmaToEvaluationTokenTransducer());
	    case MapToPreferredEvaluationTokenSequence:
		return mapOutput(
		    ConstLatticeRef(new MapOutputFromLemmaPronunciationToLemmaLattice(l)),
		    Lexicon::us()->lemmaToPreferredEvaluationTokenSequenceTransducer());
	    default:
		break;
	    }
	case Lexicon::LemmaAlphabetId:
	    switch (action) {
	    case MapToPhoneme:
		return mapOutput(
		    mapOutput(l, Fsa::invert(Lexicon::us()->lemmaPronunciationToLemmaTransducer())),
		    Fsa::invert(Lexicon::us()->phonemeToLemmaPronunciationTransducer()));
	    case MapToLemmaPronunciation:
		return mapOutput(
		    l, Fsa::invert(Lexicon::us()->lemmaPronunciationToLemmaTransducer()));
	    case MapToPreferredLemmaPronunciation:
		return ConstLatticeRef(new MapOutputFromLemmaToSingleLemmaPronunciationLattice(l));
	    case MapToLemma:
		return l;
	    case MapToSyntacticTokenSequence:
		return mapOutput(
		    l, Lexicon::us()->lemmaToSyntacticTokenTransducer());
	    case MapToEvaluationTokenSequences:
		return mapOutput(
		    l, Lexicon::us()->lemmaToEvaluationTokenTransducer());
	    case MapToPreferredEvaluationTokenSequence:
		return mapOutput(
		    l, Lexicon::us()->lemmaToPreferredEvaluationTokenSequenceTransducer());
	    default:
		break;
	    }
	case Lexicon::SyntacticTokenAlphabetId:
	    switch (action) {
	    case MapToLemma:
		return mapOutput(
		    l, Fsa::invert(Lexicon::us()->lemmaToSyntacticTokenTransducer()));
	    case MapToSyntacticTokenSequence:
		return l;
	    default:
		break;
	    }
	case Lexicon::EvaluationTokenAlphabetId:
	    switch (action) {
	    case MapToLemma:
		return mapOutput(
		    l, Fsa::invert(Lexicon::us()->lemmaToEvaluationTokenTransducer()));
	    case MapToEvaluationTokenSequences:
	    case MapToPreferredEvaluationTokenSequence:
		return l;
	    default:
		break;
	    }
	default:
	    break;
	}
	Core::Application::us()->criticalError(
	    "The implementation of the \"%s\"-mapping for output alphabet \"%s\" is pending ...",
	    alphabetMappingActionChoice[action].c_str(), Lexicon::us()->alphabetName(alphabetId).c_str());
	return ConstLatticeRef();
    }

    ConstLatticeRef mapInputToLemmaOrLemmaPronunciation(ConstLatticeRef l) {
	Lexicon::AlphabetId alphabetId = Lexicon::us()->alphabetId(l->getInputAlphabet());
	switch (alphabetId) {
	case Lexicon::LemmaAlphabetId:
	case Lexicon::LemmaPronunciationAlphabetId:
	    return l;
	default:
	    return mapInput(l, MapToLemma);
	}
    }

    ConstLatticeRef mapOutputToLemmaOrLemmaPronunciation(ConstLatticeRef l) {
	l = transducer(l);
	Lexicon::AlphabetId alphabetId = Lexicon::us()->alphabetId(l->getInputAlphabet());
	switch (alphabetId) {
	case Lexicon::LemmaAlphabetId:
	case Lexicon::LemmaPronunciationAlphabetId:
	    return l;
	default:
	    return mapOutput(l, MapToLemma);
	}
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class AlphabetMapNode : public FilterNode {
    public:
	static const Core::ParameterString paramInputMapping;
	static const Core::ParameterString paramOutputMapping;
	static const Core::ParameterBool paramProjectInput;
	static const Core::ParameterBool paramProjectOutput;
	static const Core::ParameterBool paramInvert;

    private:
	AlphabetMappingAction inputMapping_;
	AlphabetMappingAction outputMapping_;
	bool projectInput_;
	bool projectOutput_;
	bool invert_;

    protected:
	virtual ConstLatticeRef filter(ConstLatticeRef l) {
	    if (!l)
		return l;
	    if (inputMapping_ != UndefinedAlphabetMapping)
		l = mapInput(l, inputMapping_);
	    if (outputMapping_ != UndefinedAlphabetMapping)
		l = mapOutput(l, outputMapping_);
	    if (projectInput_)
		l = projectInput(l);
	    if (projectOutput_)
		l = projectOutput(l);
	    if (invert_)
		l = invert(l);
	    return l;
	}
    public:
	AlphabetMapNode(const std::string &name, const Core::Configuration &config) :
	    FilterNode(name, config) {}
	virtual ~AlphabetMapNode() {}
	virtual void init(const std::vector<std::string> &arguments) {
	    inputMapping_ = outputMapping_ = UndefinedAlphabetMapping;
	    std::string name;
	    name = paramInputMapping(config);
	    if (!name.empty()) {
		inputMapping_ = getAlphabetMappingAction(name);
		if (inputMapping_ == UndefinedAlphabetMapping)
		    criticalError("MapNode: Unknown mapping \"%s\"",
				  name.c_str());
	    }
	    name = paramOutputMapping(config);
	    if (!name.empty()) {
		outputMapping_ = getAlphabetMappingAction(name);
		if (outputMapping_ == UndefinedAlphabetMapping)
		    criticalError("MapNode: Unknown mapping \"%s\"",
				  name.c_str());
	    }
	    projectInput_ = paramProjectInput(config);
	    projectOutput_ = paramProjectOutput(config);
	    invert_ = paramInvert(config);
	}
    };
    const Core::ParameterString AlphabetMapNode::paramInputMapping(
	"map-input",
	"apply mapping to input, result is transducer",
	"");
    const Core::ParameterString AlphabetMapNode::paramOutputMapping(
	"map-output",
	"apply mapping to output, result is transducer",
	"");
    const Core::ParameterBool AlphabetMapNode::paramProjectInput(
	"project-input",
	"make lattice an acceptor by mapping input to output",
	false);
    const Core::ParameterBool AlphabetMapNode::paramProjectOutput(
	"project-output",
	"make lattice an acceptor by mapping output to input",
	false);
    const Core::ParameterBool AlphabetMapNode::paramInvert(
	"invert",
	"invert input and output",
	false);

    NodeRef createAlphabetMapNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new AlphabetMapNode(name, config));
    }
    // -------------------------------------------------------------------------



    // -------------------------------------------------------------------------
    const LabelMap::Mapping LabelMap::Identity(0);

    const LabelMap::Mapping & LabelMap::operator[] (Fsa::LabelId label) {
	if ((Fsa::FirstLabelId <= label) && (label <= Fsa::LastLabelId)) {
	    verify(0 <= label);
	    if ((label >= Fsa::LabelId(mappings.size())) || !mappings[label].first) {
		mappings.grow(label, std::make_pair(false, Identity));
		mappings[label].first = true;
		createMapping(label, mappings[label].second);
	    }
	    return mappings[label].second;
	} else
	    return Identity;
    }

    u32 LabelMap::nMappings() const {
	u32 n = 0;
	for (MappingList::const_iterator it = mappings.begin(), end = mappings.end(); it != end; ++it)
	    if (it->first) ++n;
	return n;
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class ToLowerCaseMap : public LabelMap {
    private:
	Lexicon::SymbolMap symbolMap_;
    protected:
	virtual void createMapping(Fsa::LabelId label, Mapping &mapping) {
	    Fsa::LabelId targetLabel = symbolMap_.index(
		Core::convertToLowerCase(from->symbol(label)));
	    mapping.push_back(ExtendedLabel(targetLabel, 1.0));
	}

    public:
	ToLowerCaseMap(Fsa::ConstAlphabetRef from, Fsa::ConstAlphabetRef to) :
	    LabelMap(from, to) {
	    symbolMap_ = Lexicon::us()->symbolMap(Lexicon::us()->alphabetId(to));
	}
    };
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class NonWordToEpsilonMap : public LabelMap {
    public:
	typedef std::pair<Bliss::Lemma::EvaluationTokenSequenceIterator, Bliss::Lemma::EvaluationTokenSequenceIterator> EvalTokSeqRange;

	static const LabelMap::ExtendedLabel EpsilonLabel;
    protected:
	virtual const Bliss::Lemma * getLemma(Fsa::LabelId label) = 0;

	virtual void createMapping(Fsa::LabelId label, Mapping &mapping) {
	    const Bliss::Lemma* lemma = getLemma(label);
	    EvalTokSeqRange eRange = lemma->evaluationTokenSequences();
	    if (eRange.first != eRange.second) {
		for (; eRange.first != eRange.second; ++eRange.first)
		    if (eRange.first->isEpsilon()) {
			mapping.push_back(EpsilonLabel);
			break;
		    }
	    } else
		mapping.push_back(EpsilonLabel);
	}
	NonWordToEpsilonMap(Fsa::ConstAlphabetRef alphabet):
	    LabelMap(alphabet, alphabet) {
	    description = "non-word-to-epsilon";
	}
    };
    const LabelMap::ExtendedLabel NonWordToEpsilonMap::EpsilonLabel(
	Fsa::Epsilon, 1.0);

    class NonLemmaPronunciationToEpsilonMap : public NonWordToEpsilonMap {
    private:
	Core::Ref<const Bliss::LemmaPronunciationAlphabet> lpAlphabet_;
    protected:
	virtual const Bliss::Lemma * getLemma(Fsa::LabelId label) {
	    return lpAlphabet_->lemmaPronunciation(label)->lemma();
	}
    public:
	NonLemmaPronunciationToEpsilonMap():
	    NonWordToEpsilonMap(Lexicon::us()->lemmaPronunciationAlphabet()),
	    lpAlphabet_(Lexicon::us()->lemmaPronunciationAlphabet()) {}
    };

    class NonLemmaToEpsilonMap : public NonWordToEpsilonMap {
    private:
	Core::Ref<const Bliss::LemmaAlphabet> lAlphabet_;
    protected:
	virtual const Bliss::Lemma * getLemma(Fsa::LabelId label) {
	    return lAlphabet_->lemma(label);
	}
    public:
	NonLemmaToEpsilonMap():
	    NonWordToEpsilonMap(Lexicon::us()->lemmaAlphabet()),
	    lAlphabet_(Lexicon::us()->lemmaAlphabet()) {}
    };
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class CompoundWordSplittingMap : public LabelMap {
    protected:
	Fsa::ConstAlphabetRef alphabet_;
	Lexicon *lexicon_;

    protected:
	virtual std::string getSymbol(Fsa::LabelId label) = 0;
	virtual Fsa::LabelId getLabel(const std::string &symbol) = 0;

	/**
	   Split at whitespace or at "_" or "-", if they are not pending, i.e. succeeded or followed by whitespace.
	**/
	virtual void createMapping(Fsa::LabelId label, Mapping &mapping) {
	    std::string symbol = getSymbol(label);
	    const char *b(symbol.c_str()), *e(symbol.c_str() + symbol.size() - 1);
	    for (; ::isspace(*b); ++b);
	    for (; ::isspace(*e) && (e > b); --e);
	    if ((b + 2 <= e) && !(
		    ((*b == '[') && (*e == ']')) ||
		    ((*b == '{') && (*e == '}')) ||
		    ((*b == '<') && (*e == '>')) ||
		    ((*b == '(') && (*e == ')')))) {
		std::vector<size_t> splitIndices;
		const char *c = b + 2;
		for (char l, t = *b, n = *(b + 1); c <= e; ++c) {
		    l = t; t = n; n = *c;
		    if (::isspace(t) || (((t == '_') || (t == '-')) && !::isspace(l) && !(::isspace(n) || (n == '\0'))))
			splitIndices.push_back(c - b - 1);
		}
		if (!splitIndices.empty()) {
		    splitIndices.push_back(e - b + 1);
		    size_t begin = 0;
		    f32 totalLength = 0.0;
		    for (std::vector<size_t>::const_iterator itEnd = splitIndices.begin();
			 itEnd != splitIndices.end(); ++itEnd) {
			if (begin < *itEnd) {
			    std::string part(symbol.c_str() + begin, *itEnd - begin);
			    verify(part.size() > 0);
			    Fsa::LabelId partLabel = getLabel(part);
			    if (partLabel != Fsa::InvalidLabelId) {
				mapping.push_back(
				    LabelMap::ExtendedLabel(partLabel, f32(part.size())));
				totalLength += mapping.back().length;
			    } // else discard part
			    begin = *itEnd + 1;
			}
		    }
		    for (LabelMap::Mapping::iterator it = mapping.begin(), end = mapping.end();
			 it != end; ++it) it->length /= totalLength;
		}
	    }
	}
    public:
	CompoundWordSplittingMap(Fsa::ConstAlphabetRef alphabet):
	    LabelMap(alphabet, alphabet), alphabet_(alphabet), lexicon_(Lexicon::us().get()) {
	    description = "split-compound-words";
	}
    };

    class CompoundLemmaPronunciationSplittingMap : public CompoundWordSplittingMap {
    private:
	Core::Ref<const Bliss::LemmaPronunciationAlphabet> lpAlphabet_;
    protected:
	virtual std::string getSymbol(Fsa::LabelId label) {
	    return lpAlphabet_->lemmaPronunciation(label)->lemma()->symbol().str();
	}
	virtual Fsa::LabelId getLabel(const std::string &symbol) {
	    return lexicon_->lemmaPronunciationId(symbol, 1);
	}
    public:
	CompoundLemmaPronunciationSplittingMap():
	    CompoundWordSplittingMap(Lexicon::us()->lemmaPronunciationAlphabet()),
	    lpAlphabet_(Lexicon::us()->lemmaPronunciationAlphabet()) {
	}
    };

    class CompoundLemmaSplittingMap : public CompoundWordSplittingMap {
    protected:
	virtual std::string getSymbol(Fsa::LabelId label) {
	    return alphabet_->symbol(label);
	}
	virtual Fsa::LabelId getLabel(const std::string &symbol) {
	    return lexicon_->lemmaId(symbol);
	}
    public:
	CompoundLemmaSplittingMap():
	    CompoundWordSplittingMap(Lexicon::us()->lemmaAlphabet()) {}
    };

    class CompoundSyntacticTokenSplittingMap : public CompoundWordSplittingMap {
    protected:
	virtual std::string getSymbol(Fsa::LabelId label) {
	    return alphabet_->symbol(label);
	}
	virtual Fsa::LabelId getLabel(const std::string &symbol) {
	    return lexicon_->syntacticTokenId(symbol);
	}
    public:
	CompoundSyntacticTokenSplittingMap():
	    CompoundWordSplittingMap(Lexicon::us()->syntacticTokenAlphabet()) {}
    };

    class CompoundEvaluationTokenSplittingMap : public CompoundWordSplittingMap {
    protected:
	virtual std::string getSymbol(Fsa::LabelId label) {
	    return alphabet_->symbol(label);
	}
	virtual Fsa::LabelId getLabel(const std::string &symbol) {
	    return lexicon_->evaluationTokenId(symbol);
	}
    public:
	CompoundEvaluationTokenSplittingMap():
	    CompoundWordSplittingMap(Lexicon::us()->evaluationTokenAlphabet()) {}
    };
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class StaticLabelMap : public LabelMap {
    public:
	static const Core::ParameterString paramFromAlphabet;
	static const Core::ParameterString paramToAlphabet;
	static const Core::ParameterString paramFile;
	static const Core::ParameterString paramEncoding;

    private:
	Lexicon::SymbolMap fromMap_;
	Lexicon::SymbolMap toMap_;

    protected:
	virtual void createMapping(Fsa::LabelId label, Mapping &mapping) {}

    public:
	StaticLabelMap(Fsa::ConstAlphabetRef from, Fsa::ConstAlphabetRef to):
	    LabelMap(from, to) {
	    fromMap_ = Lexicon::us()->symbolMap(Lexicon::us()->alphabetId(from));
	    toMap_ = Lexicon::us()->symbolMap(Lexicon::us()->alphabetId(to));
	}

	void load(const std::string &filename, const std::string &encoding) {
	    TextFileParser parser(filename, encoding);
	    u32 nMappingsLoaded = 0;
	    for (;;) {
		const TextFileParser::StringList &cols = parser.next();
		if (!parser) break;
		Fsa::LabelId label = fromMap_.index(cols.front());
		if (label != Fsa::InvalidLabelId) {
		    mappings.grow(label, std::make_pair(true, Identity));
		    Mapping &mapping(mappings[label].second);
		    if (!mapping.empty()) {
			Core::Application::us()->warning(
			    "In file \"%s\": duplicate entry for \"%s\"; keep first mapping.",
			    filename.c_str(), from->symbol(label).c_str());
		    } else {
			if (cols.size() == 1) {
			    mapping.push_back(ExtendedLabel(Fsa::Epsilon, 1.0));
			} else {
			    f32 totalLength = 0.0;
			    for (u32 i = 1; i < cols.size(); ++i) {
				Fsa::LabelId partLabel = toMap_.index(cols[i]);
				if (partLabel == Fsa::InvalidLabelId)
				    Core::Application::us()->criticalError(
					"In file \"%s\": could not get id for \"%s\".",
					filename.c_str(), cols[i].c_str());
				mapping.push_back(LabelMap::ExtendedLabel(partLabel, f32(cols[i].size())));
				totalLength += mapping.back().length;
			    }
			    for (LabelMap::Mapping::iterator it = mapping.begin(), end = mapping.end();
				 it != end; ++it) it->length /= totalLength;
			}
			++nMappingsLoaded;
		    }
		} else
		    Core::Application::us()->warning(
			"In file \"%s\": Could not find \"%s\" in lexicon; discard.",
			filename.c_str(), cols.front().c_str());
	    }
	    description = Core::form("static(n=%d)", nMappingsLoaded);
	}

	static LabelMapRef create(const Core::Configuration &config, Fsa::ConstAlphabetRef defaultAlphabet) {
	    std::string filename = StaticLabelMap::paramFile(config);
	    if (filename.empty())
		return LabelMapRef();
	    if (!defaultAlphabet)
		defaultAlphabet = Lexicon::us()->alphabet(Lexicon::LemmaAlphabetId);
	    std::string fromAlphabetName = paramFromAlphabet(config);
	    Fsa::ConstAlphabetRef from = fromAlphabetName.empty() ?
		defaultAlphabet :
		Lexicon::us()->alphabet(Lexicon::us()->alphabetId(fromAlphabetName, true));
	    std::string toAlphabetName = paramToAlphabet(config);
	    Fsa::ConstAlphabetRef to = toAlphabetName.empty() ?
		from :
		Lexicon::us()->alphabet(Lexicon::us()->alphabetId(toAlphabetName, true));
	    StaticLabelMap *labelMap = new StaticLabelMap(from, to);
	    labelMap->load(filename, StaticLabelMap::paramEncoding(config));
	    return LabelMapRef(labelMap);
	}
    };
    const Core::ParameterString StaticLabelMap::paramFromAlphabet(
	"from",
	"name of source alphabet",
	"");
    const Core::ParameterString StaticLabelMap::paramToAlphabet(
	"to",
	"name of target alphabet",
	"");
    const Core::ParameterString StaticLabelMap::paramFile(
	"file",
	"name of label mapping file",
	"");
    const Core::ParameterString StaticLabelMap::paramEncoding(
	"encoding",
	"encoding of label mapping file",
	"utf-8");
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    struct LabelMap::Internal {
	Core::Vector<LabelMapRef> toLowerCaseMaps;
	Core::Vector<LabelMapRef> nonWordToEpsilonMaps;
	Core::Vector<LabelMapRef> compoundWordMaps;
    };

    LabelMap::Internal *LabelMap::internal_(new LabelMap::Internal);

    LabelMapRef LabelMap::createToLowerCaseMap(Lexicon::AlphabetId alphabetId) {
	internal_->toLowerCaseMaps.grow(alphabetId);
	if (internal_->toLowerCaseMaps[alphabetId])
	    return internal_->toLowerCaseMaps[alphabetId];
	switch (alphabetId) {
	case Lexicon::PhonemeAlphabetId:
	case Lexicon::LemmaPronunciationAlphabetId:
	    Core::Application::us()->criticalError(
		"Mapping to lower case makes no sense for the \"%s\" alphabet",
		Lexicon::us()->alphabetName(alphabetId).c_str());
	    return LabelMapRef();
	default:
	    Fsa::ConstAlphabetRef alphabet = Lexicon::us()->alphabet(alphabetId);
	    return (internal_->toLowerCaseMaps[alphabetId] =
		    LabelMapRef(new ToLowerCaseMap(alphabet, alphabet)));
	}
    }

    LabelMapRef LabelMap::createNonWordToEpsilonMap(Lexicon::AlphabetId alphabetId) {
	internal_->nonWordToEpsilonMaps.grow(alphabetId);
	if (internal_->nonWordToEpsilonMaps[alphabetId])
	    return internal_->nonWordToEpsilonMaps[alphabetId];
	switch (alphabetId) {
	case Lexicon::LemmaPronunciationAlphabetId:
	    return (internal_->nonWordToEpsilonMaps[alphabetId] = LabelMapRef(new NonLemmaPronunciationToEpsilonMap()));
	case Lexicon::LemmaAlphabetId:
	    return (internal_->nonWordToEpsilonMaps[alphabetId] = LabelMapRef(new NonLemmaToEpsilonMap()));
	default:
	    Core::Application::us()->criticalError(
		"Mapping non words to epsilon makes no sense for the \"%s\" alphabet",
		Lexicon::us()->alphabetName(alphabetId).c_str());
	    return LabelMapRef();
	}
    }

    LabelMapRef LabelMap::createCompoundWordSplittingMap(Lexicon::AlphabetId alphabetId) {
	internal_->compoundWordMaps.grow(alphabetId);
	if (internal_->compoundWordMaps[alphabetId])
	    return internal_->compoundWordMaps[alphabetId];
	switch (alphabetId) {
	case Lexicon::LemmaPronunciationAlphabetId:
	    return (internal_->compoundWordMaps[alphabetId] = LabelMapRef(new CompoundLemmaPronunciationSplittingMap()));
	case Lexicon::LemmaAlphabetId:
	    return (internal_->compoundWordMaps[alphabetId] = LabelMapRef(new CompoundLemmaSplittingMap()));
	case Lexicon::SyntacticTokenAlphabetId:
	    return (internal_->compoundWordMaps[alphabetId] = LabelMapRef(new CompoundSyntacticTokenSplittingMap()));
	case Lexicon::EvaluationTokenAlphabetId:
	    return (internal_->compoundWordMaps[alphabetId] = LabelMapRef(new CompoundEvaluationTokenSplittingMap()));
	default:
	    Core::Application::us()->criticalError(
		"Compound word splitting makes no sense for the \"%s\" alphabet",
		Lexicon::us()->alphabetName(alphabetId).c_str());
	    return LabelMapRef();
	}
    }

    LabelMapRef LabelMap::load(const Core::Configuration &config, Fsa::ConstAlphabetRef defaultAlphabet) {
	return StaticLabelMap::create(config, defaultAlphabet);
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    /*
      One-To-One mappings can be done lazy.
    */
    class OneToOneLabelMappingLattice : public ModifyLattice {
	typedef ModifyLattice Precursor;
    private:
	LabelMapRef labelMap_;
	bool isAcceptor_;
    public:
	OneToOneLabelMappingLattice(ConstLatticeRef l, LabelMapRef labelMap) :
	    Precursor(l), labelMap_(labelMap) {
	    isAcceptor_ = (l->type() == Fsa::TypeAcceptor);
	}
	virtual ~OneToOneLabelMappingLattice() {}
	virtual void modifyState(State *sp) const {
	    for (State::iterator a = sp->begin(); a != sp->end(); ++a) {
		const LabelMap::Mapping &mapping = (*labelMap_)[a->input()];
		switch (mapping.size()) {
		case 0:
		    // identity
		    break;
		case 1:
		    if (isAcceptor_)
			a->input_ = a->output_ = mapping.front().label;
		    else
			a->input_ = mapping.front().label;
		    break;
		default:
		    // not a one-to-one mapping
		    defect();
		}
	    }
	}
	virtual std::string describe() const {
	    return Core::form("oneToOneLabelMap(%s;%s)",
			      fsa_->describe().c_str(), labelMap_->description.c_str());
	}
	virtual Fsa::ConstAlphabetRef getInputAlphabet() const {
	    return labelMap_->to;
	}
	virtual Fsa::ConstAlphabetRef getOutputAlphabet() const {
	    return isAcceptor_ ? labelMap_->to : fsa_->getOutputAlphabet();
	}
    };
    ConstLatticeRef applyOneToOneLabelMap(ConstLatticeRef l, LabelMapRef labelMap) {
	if (!l)
	    return ConstLatticeRef();
	return ConstLatticeRef(new OneToOneLabelMappingLattice(l, labelMap));
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    /*
      One-To-Many mappings are done in a static way.
    */
    class LabelMapper : protected TraverseState {
    protected:
	typedef LabelMap::Mapping Mapping;
	typedef LabelMap::ExtendedLabel ExtLabel;

    private:
	StaticLatticeRef s_;
	StaticBoundariesRef b_;
	LabelMapRef labelMap_;
	bool isAcceptor_;
	ConstSemiringRef semiring_;
	Core::Vector<std::pair<Fsa::StateId, State::iterator> > compoundArcs_;

    private:
	ScoresRef toPartialScores(const ExtLabel &extLabel, ScoresRef compoundScores) {
	    ScoresRef scores = semiring_->clone(compoundScores);
	    for (Scores::iterator itScore = semiring_->begin(scores), endScore = semiring_->end(scores);
		 itScore != endScore; ++itScore) *itScore *= extLabel.length;
	    return scores;
	}

	Fsa::LabelId toPartialLabel(const ExtLabel &extLabel) {
	    return extLabel.label;
	}

	void splitCompoundArcs() {
	    for (Core::Vector<std::pair<Fsa::StateId, State::iterator> >::iterator it = compoundArcs_.begin();
		 it != compoundArcs_.end(); ++it) {
		Fsa::StateId sid = it->first;
		Arc &a(*(it->second));
		const Mapping &mapping((*labelMap_)[a.input()]);
		ScoresRef compoundScores = a.weight();
		Fsa::StateId compoundTarget = a.target();
		std::vector<Time> endTimes(0);
		if (b_) {
		    Time compoundStartTime = l->boundary(sid).time();
		    Time compoundEndTime = l->boundary(compoundTarget).time();
		    Time duration = compoundEndTime - compoundStartTime;
		    if (duration < Time(mapping.size()))
			Core::Application::us()->criticalError(
			    "Cannot distribute compound word \"%s\" with %d time frames over %d subwords.",
			    labelMap_->from->symbol(a.input()).c_str(), duration, u32(mapping.size()));
		    // distribute time frames over subwords;
		    // guarantee each subword at least a single frame
		    endTimes.resize(mapping.size());
		    f32 extraTime = f32(duration - mapping.size());
		    endTimes.front() = compoundStartTime + 1 + Time(std::floor(mapping.front().length * extraTime));
		    for (u32 i = 1; i < endTimes.size(); ++i)
			endTimes[i] = endTimes[i - 1] + 1 + Time(std::floor(mapping[i].length * extraTime));
		    verify(endTimes.back() <= compoundEndTime);
		    // distribute surplus time frames over last subwords;
		    // a better strategy would be to distribute them according to the floored value (see above) or relative sub word length,
		    // but this requires expensive sorting for doubtful precision gain.
		    verify((compoundEndTime - endTimes.back()) <= Time(mapping.size()));
		    for (u32 i = mapping.size() - 1, t = (compoundEndTime - endTimes.back()); t > 0; --i, --t)
			endTimes[i] += t;
		    verify(endTimes.back() == compoundEndTime);
		}
		State *nextSp = s_->newState();
		{
		    const ExtLabel &thisPart(mapping[0]);
		    Fsa::LabelId partLabel = toPartialLabel(thisPart);
		    a.input_ = partLabel; if (isAcceptor_) a.output_ = partLabel;
		    a.weight_ = toPartialScores(thisPart, compoundScores);
		    a.target_ = nextSp->id();
		    if (b_)
			b_->set(nextSp->id(), Boundary(endTimes.front(), Boundary::Transit(Bliss::Phoneme::term, Bliss::Phoneme::term, WithinWordBoundary)));
		}
		for (u32 i = 1; i < mapping.size() - 1; ++i) {
		    const ExtLabel &thisPart(mapping[i]);
		    State *thisSp = nextSp;
		    nextSp = s_->newState();
		    ScoresRef partScores = toPartialScores(thisPart, compoundScores);
		    Fsa::LabelId partLabel = toPartialLabel(thisPart);
		    thisSp->newArc(
			nextSp->id(), partScores, partLabel, (isAcceptor_ ? partLabel : Fsa::Epsilon));
		    if (b_)
			b_->set(nextSp->id(), Boundary(endTimes[i], Boundary::Transit(Bliss::Phoneme::term, Bliss::Phoneme::term, WithinWordBoundary)));
		}
		{
		    const ExtLabel &thisPart(mapping.back());
		    ScoresRef partScores = toPartialScores(thisPart, compoundScores);
		    Fsa::LabelId partLabel = toPartialLabel(thisPart);
		    nextSp->newArc(
			compoundTarget, partScores, partLabel, (isAcceptor_ ? partLabel : Fsa::Epsilon));
		}
	    }
	}

    protected:
	void exploreState(ConstStateRef sr) {
	    State *sp = new State(*sr);
	    s_->setState(sp);
	    if (b_)
		b_->set(sp->id(), l->boundary(sr->id()));
	    for (State::iterator a = sp->begin(); a != sp->end(); ++a) {
		if (a->input() != Fsa::Epsilon) {
		    const Mapping &mapping((*labelMap_)[a->input()]);
		    switch (mapping.size()) {
		    case 0:
			// identity
			break;
		    case 1:
			if (isAcceptor_)
			    a->input_ = a->output_ = mapping.front().label;
			else
			    a->input_ = mapping.front().label;
			break;
		    default:
			compoundArcs_.push_back(std::make_pair(sp->id(), a));
		    }
		}
	    }
	}

    public:
	LabelMapper(ConstLatticeRef l, StaticLatticeRef s, StaticBoundariesRef b,
		    LabelMapRef labelMap) :
	    TraverseState(l), s_(s), b_(b), labelMap_(labelMap) {
	    isAcceptor_ = (l->type() == Fsa::TypeAcceptor);
	    semiring_ = l->semiring();
	    traverse();
	    splitCompoundArcs();
	    s_->setInitialStateId(l->initialStateId());
	}
	virtual ~LabelMapper() {}
    };

    ConstLatticeRef applyLabelMap(ConstLatticeRef l, LabelMapRef labelMap) {
	ensure(Lexicon::us()->alphabetId(l->getInputAlphabet()) == Lexicon::us()->alphabetId(labelMap->from));
	StaticLatticeRef s = StaticLatticeRef(new StaticLattice(l->type()));
	s->setInputAlphabet(labelMap->to);
	if (s->type() != Fsa::TypeAcceptor)
	    s->setOutputAlphabet(l->getOutputAlphabet());
	s->setProperties(l->knownProperties(), l->properties());
	s->setSemiring(l->semiring());
	s->setBoundaries(InvalidBoundaries);
	s->setDescription(
	    Core::form("labelMap(%s;%s)", l->describe().c_str(), labelMap->description.c_str()));
	StaticBoundariesRef b;
	if (l->getBoundaries()->valid()) {
	    b = StaticBoundariesRef(new StaticBoundaries());
	    s->setBoundaries(b);
	}
	LabelMapper(l, s, b, labelMap);
	return s;
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class LabelMapNode : public FilterNode {
    public:
	static const Core::ParameterBool paramMapToLowerCase;
	static const Core::ParameterBool paramMapNonWordsToEpsilon;
	static const Core::ParameterBool paramSplitCompoundWords;
	static const Core::ParameterBool paramProjectInput;

    private:
	bool mapToLowerCase_;
	bool mapNonWordsToEpsilon_;
	bool splitCompoundWords_;
	bool applyUserMap_;
	bool projectInput_;

	LabelMapRef nonWordsToEpsilonMap_;
	LabelMapRef compoundWordsMap_;
	LabelMapRef userMap_;

    protected:
	virtual ConstLatticeRef filter(ConstLatticeRef l) {
	    if (!l)
		return ConstLatticeRef();
	    LexiconRef lexicon = Lexicon::us();
	    Lexicon::AlphabetId alphabetId = lexicon->alphabetId(l->getInputAlphabet());
	    if (mapToLowerCase_) {
		l = applyOneToOneLabelMap(l, LabelMap::createToLowerCaseMap(alphabetId));
	    }
	    if (mapNonWordsToEpsilon_) {
		l = applyOneToOneLabelMap(l, LabelMap::createNonWordToEpsilonMap(alphabetId));
	    }
	    if (splitCompoundWords_) {
		l = applyLabelMap(l, LabelMap::createCompoundWordSplittingMap(alphabetId));
	    }
	    if (applyUserMap_) {
		ensure(userMap_);
		if (Lexicon::us()->alphabetId(userMap_->to) != Lexicon::us()->alphabetId(l->getInputAlphabet()))
		    criticalError("LabelMapNode: Cannot apply user map, because lattice alphabet \"%s\" does not match mapping target alphabet \"%s\".",
				  Lexicon::us()->alphabetName(Lexicon::us()->alphabetId(l->getInputAlphabet())).c_str(),
				  Lexicon::us()->alphabetName(Lexicon::us()->alphabetId(userMap_->to)).c_str());
		l = applyLabelMap(l, userMap_);
	    }
	    if (projectInput_)
		l = projectInput(l);
	    return l;
	}
    public:
	LabelMapNode(const std::string &name, const Core::Configuration &config) :
	    FilterNode(name, config) {}
	~LabelMapNode() {}
	virtual void init(const std::vector<std::string> &arguments) {
	    mapToLowerCase_ = paramMapToLowerCase(config);
	    mapNonWordsToEpsilon_ = paramMapNonWordsToEpsilon(config);
	    splitCompoundWords_ = paramSplitCompoundWords(config);
	    userMap_ = LabelMap::load(select("map"), Lexicon::us()->alphabet(Lexicon::LemmaAlphabetId));
	    applyUserMap_ = bool(userMap_);
	    projectInput_ = paramProjectInput(config);
	}
    };
    const Core::ParameterBool LabelMapNode::paramMapToLowerCase(
	"map-to-lower-case",
	"map words to lower case",
	false);
    const Core::ParameterBool LabelMapNode::paramMapNonWordsToEpsilon(
	"map-non-words-to-eps",
	"map non-words, i.e. lemmas or lemma pronunciations having an empty evaluation token sequence, to epsilon",
	false);
    const Core::ParameterBool LabelMapNode::paramSplitCompoundWords(
	"split-compound-words",
	"split compound words",
	false);
    const Core::ParameterBool LabelMapNode::paramProjectInput(
	"project-input",
	"make lattice an acceptor by mapping input to output",
	false);
    NodeRef createLabelMapNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new LabelMapNode(name, config));
    }
    // -------------------------------------------------------------------------

} // namespace Flf
