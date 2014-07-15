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
#ifndef _LATTICE_HTK_READER_HH
#define _LATTICE_HTK_READER_HH

#include <Bliss/Lexicon.hh>
#include <Bliss/Orthography.hh>
#include <Core/Component.hh>
#include <Core/Version.hh>
#include <Fsa/Alphabet.hh>
#include <Fsa/Automaton.hh>
#include <Fsa/Basic.hh>
#include <Fsa/Semiring.hh>
#include <Fsa/Storage.hh>

#include "Lattice.hh"

namespace Lattice {

    /**
     * Parse a lattice in HTK SLF format.
     *
     */
    class HtkReader :
	Core::Component {
    private:
	typedef HtkReader Self;
	typedef Core::Component Precursor;
	typedef std::pair<std::string, std::string> StringPair;
	typedef std::vector<StringPair>             StringPairList;
	typedef Bliss::OrthographicParser::LemmaIterator LemmaIterator;
	typedef Bliss::OrthographicParser::LemmaRange LemmaRange;

	struct Node {
	    u32 id;
	    Speech::TimeframeIndex timeframeIndex;
	    std::string label;
	    LemmaRange lemmas;
	    bool hasLemmas() const { return lemmas.first != lemmas.second; }
	};

	struct Link {
		/**
		 * J=...
		 */
	    u32 id;
	    /**
	     * S=... & E=...
	     */
	    size_t sourceState, targetState;
	    /**
	     * a=... & l=...
	     */
	    f64 amScore, lmScore;
	    /**
	     * W=... std::string representation
	     */
	    std::string label;
	    LemmaRange lemmas;
	    bool hasLemmas() const { return lemmas.first != lemmas.second; }
	};

	struct State {
	    Fsa::State * fsaState;
	    std::string label;
	    LemmaRange lemmas;
	    bool hasLemmas() const { return lemmas.first != lemmas.second; }
	};
	typedef std::vector<State> StateList;

    protected:
	typedef enum StatusEnum {
	    htkOk = 0,
	    htkPropertyParseError,
	    htkUnexpectedLine,
	    htkUnexpectedNode,
	    htkParseNodeError,
	    htkAddNodeError,
	    htkParseLinkError,
	    htkAddLinkError,
	    htkNoInitialStateError,
	    htkNoFinalStateError
	} Status;

    private:
	Bliss::LexiconRef lexiconRef_;
	f64 amScale_;
	f64 lmScale_;
	f64 penaltyScale_;
	bool keepVariants_;
	f64 wordPenalty_;
	f64 silPenalty_;
	bool isCapitalize_;

	Fsa::LabelId unkLemmaId_;
	Fsa::LabelId silLemmaId_;

	Fsa::StorageAutomaton * lattice_;
	WordBoundaries * wordBoundaries_;
	StateList states_;

	static const Core::ParameterFloat paramTimeframeIndexScale;
	f64 timeframeIndexScale_;
	mutable f64 timeOffset_;

	Fsa::ConstAlphabetRef htkAlphabetRef_;
	Fsa::ConstAutomatonRef lemmaPronunciationToHtkTransducerRef_;
	Bliss::OrthographicParser * orthParser_;

	u32 line_;
	Status status_;

    private:
	void nextLine(std::istream & is, std::string & s);

	Speech::TimeframeIndex timeframeIndex(f64 time) const {
	    if (timeOffset_ == Core::Type<f64>::min) timeOffset_ = time;
	    return static_cast<Speech::TimeframeIndex>(round((time - timeOffset_) * timeframeIndexScale_));
	}

    protected:
	// conversion to fsa
	Fsa::Weight  getWeight (const Link &, Fsa::LabelId);
	Fsa::StateId getStateId(u32);

	bool checkStatus(Status status);

	Status init();
	Status addState(Node & node);
	Status addArc(Link & link);
	Status setInitialState();
	Status setFinalStates();

	// handle
	void comment(const std::string & s) {
	    log("comment: ") << s.c_str();
	}

	Status start() { return init(); }

	Status header(const StringPairList & props) {
	    for (StringPairList::const_iterator it = props.begin();
		 it != props.end(); ++it)
		log() << it->first << ": " << it->second;
	    return htkOk;
	}

	Status node(Node & node) { return addState(node); }

	Status link(Link & link) { return addArc(link); }

	Status end() {
	    Status status = setInitialState();
	    return (status == htkOk) ? setFinalStates() : status;
	}

	// parse
	Status add(const std::string  & s, StringPairList & props);

	LemmaRange parseLabel(std::string & label, const std::string & variant = "");

	Status set(const StringPairList & props, Node & node);

	Status set(const StringPairList & props, Link & link);

    public:
	HtkReader(
	    const Core::Configuration & config,
	    Bliss::LexiconRef lexiconRef,
	    f64 amScale,
	    f64 lmScale,
	    f64 penaltyScale,
	    f64 pronunciationScale,
	    bool keepVariants);

	~HtkReader();

	void setWordPenalty(f64 penalty) {
	    wordPenalty_ = penalty;
	    log("htk lattice settings:\nword-penalty : %f",
		wordPenalty_);
	}

	void setCapitalize(bool isCapitalize) {
	    isCapitalize_ = isCapitalize;
	    if (isCapitalize_) log("capitalization activated");
	    else log("capitalization deactivated");
	}

	void setSilencePenalty(f64 penalty) {
	    silPenalty_ = penalty;
	    log("htk lattice settings:\nsilence-penalty : %f",
		silPenalty_);
	}

	Fsa::ConstAlphabetRef getAlphabet() {
	    return htkAlphabetRef_;
	}

	ConstWordLatticeRef read(std::istream &);
    };


} // namespace Lattice

#endif // _LATTICE_HTK_READER_HH
