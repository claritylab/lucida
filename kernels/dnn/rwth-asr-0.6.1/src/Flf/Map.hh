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
#ifndef _FLF_MAP_HH
#define _FLF_MAP_HH

#include <Core/ReferenceCounting.hh>
#include <Core/Vector.hh>
#include <Fsa/Automaton.hh>

#include "FlfCore/Lattice.hh"
#include "Lexicon.hh"
#include "Network.hh"


namespace Flf {

    /**
     * Switch between acceptor and transducer
     **/
    ConstLatticeRef transducer(ConstLatticeRef l);
    ConstLatticeRef projectInput(ConstLatticeRef l);
    ConstLatticeRef projectOutput(ConstLatticeRef l);
    ConstLatticeRef invert(ConstLatticeRef l);


    /**
     * Map alphabets
     **/
    typedef enum {
	UndefinedAlphabetMapping,
	MapToPhoneme,
	MapToLemmaPronunciation,
	MapToPreferredLemmaPronunciation,
	MapToLemma,
	MapToSyntacticTokenSequence,
	MapToEvaluationTokenSequences,
	MapToPreferredEvaluationTokenSequence,
    } AlphabetMappingAction;
    AlphabetMappingAction getAlphabetMappingAction(const std::string &name);
    const std::string & getAlphabetMappingActionName(AlphabetMappingAction action);

    ConstLatticeRef mapInput(Fsa::ConstAutomatonRef targetToInputFsa, ConstLatticeRef l);
    ConstLatticeRef mapInput(ConstLatticeRef l, Lexicon::AlphabetId alphabetId);
    ConstLatticeRef mapInput(ConstLatticeRef l, AlphabetMappingAction action);
    ConstLatticeRef mapInputToLemmaOrLemmaPronunciation(ConstLatticeRef l);

    ConstLatticeRef mapOutput(ConstLatticeRef l, Fsa::ConstAutomatonRef outputToTargetFsa);
    ConstLatticeRef mapOutput(ConstLatticeRef l, Lexicon::AlphabetId alphabetId);
    ConstLatticeRef mapOutput(ConstLatticeRef l, AlphabetMappingAction action);
    ConstLatticeRef mapOutputToLemmaOrLemmaPronunciation(ConstLatticeRef l);

    NodeRef createAlphabetMapNode(const std::string &name, const Core::Configuration &config);


    /**
     * Manipulate labels
     *
     * A restricted composition:
     * Supports 1->[0,n] mappings;
     * preserves time boundaries and transits,
     * times and scores are distributed linearlly over the new arcs,
     * i.e. the sum over the new scores equals the old score
     *
     * Label map provides a lazy implementation.
     **/
    class LabelMap;
    typedef Core::Ref<LabelMap> LabelMapRef;
    typedef Core::Vector<LabelMapRef> LabelMapList;
    class LabelMap : public Core::ReferenceCounted {
    public:
	struct ExtendedLabel {
	    Fsa::LabelId label;
	    f32 length;
	    Bliss::Phoneme::Id initial, final;
	    ExtendedLabel() {}
	    ExtendedLabel(Fsa::LabelId label, f32 length) :
		label(label), length(length),
		initial(Boundary::Transit::InvalidId), final(Boundary::Transit::InvalidId) {}
	    ExtendedLabel(Fsa::LabelId label, f32 length, Bliss::Phoneme::Id initial, Bliss::Phoneme::Id final) :
		label(label), length(length),
		initial(initial), final(final) {}
	};
	typedef Core::Vector<ExtendedLabel> Mapping;
    protected:
	typedef Core::Vector<std::pair<bool, Mapping> > MappingList;
    public:
	static const Mapping Identity;
    public:
	Fsa::ConstAlphabetRef from, to;
	std::string description;
    protected:
	MappingList mappings;
    protected:
	virtual void createMapping(Fsa::LabelId label, Mapping &mapping) = 0;
    public:
	LabelMap(Fsa::ConstAlphabetRef from, Fsa::ConstAlphabetRef to) :
	    from(from), to(to), mappings(0) {}
	const Mapping & operator[] (Fsa::LabelId label);
	u32 nMappings() const;
	const Mapping & identity() const {
	    return Identity;
	}

    private:
	struct Internal;
	static Internal *internal_;
    public:
	static LabelMapRef createToLowerCaseMap(Lexicon::AlphabetId alphabetId);
	static LabelMapRef createNonWordToEpsilonMap(Lexicon::AlphabetId alphabetId);
	static LabelMapRef createCompoundWordSplittingMap(Lexicon::AlphabetId alphabetId);
	static LabelMapRef load(const Core::Configuration &config, Fsa::ConstAlphabetRef defaultAlphabet = Fsa::ConstAlphabetRef());
    };

    // lazy, but only 1-to-1 or identity mappings
    ConstLatticeRef applyOneToOneLabelMap(ConstLatticeRef l, LabelMapRef labelMap);
    // static
    ConstLatticeRef applyLabelMap(ConstLatticeRef l, LabelMapRef labelMap);

    NodeRef createLabelMapNode(const std::string &name, const Core::Configuration &config);

} // namespace Flf

#endif // _FLF_MAP_HH
