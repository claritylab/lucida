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
#ifndef _LATTICE_LATTICE_HH
#define _LATTICE_LATTICE_HH

#include <Bliss/Fsa.hh>
#include <Bliss/Lexicon.hh>
#include <Core/Types.hh>
#include <Core/Description.hh>
#include <Core/ReferenceCounting.hh>
#include <Fsa/Alphabet.hh>
#include <Fsa/Dfs.hh>
#include <Core/Vector.hh>
#include <Speech/Types.hh>

namespace Lattice {

    class WordBoundary {
    public:
	struct Transit {
	    Bliss::Phoneme::Id final, initial;
	    Transit() : final(Bliss::Phoneme::term),
			initial(Bliss::Phoneme::term) {}
	    Transit(const std::pair<Bliss::Phoneme::Id, Bliss::Phoneme::Id> &transit) :
		final(transit.first), initial(transit.second) {}
	    Transit(Bliss::Phoneme::Id _final, Bliss::Phoneme::Id _initial) :
		final(_final), initial(_initial) {}

	    bool operator==(const Transit &rhs) const {
		return final == rhs.final && initial == rhs.initial;
	    }
	    bool readBinary(Core::BinaryInputStream&);
	    bool writeBinary(Core::BinaryOutputStream&) const;
	};
    private:
	Speech::TimeframeIndex time_;
	Transit transit_;
    public:
	WordBoundary(Speech::TimeframeIndex time = Speech::InvalidTimeframeIndex) : time_(time) {}
	WordBoundary(Speech::TimeframeIndex time, const Transit &transit) :
	    time_(time), transit_(transit) {}
	WordBoundary(Speech::TimeframeIndex time,
		     const std::pair<Bliss::Phoneme::Id, Bliss::Phoneme::Id> &transit) :
	    time_(time), transit_(transit) {}

	void setTime(Speech::TimeframeIndex time) { time_ = time; }
	Speech::TimeframeIndex time() const { return time_; }
	void setTransit(const Transit &transit) { transit_ = transit; }
	Transit transit() const { return transit_; }
	bool operator== (const WordBoundary &b) const {
	    return (time_ == b.time_) && (transit_ == b.transit_);
	}
	size_t hashKey() const { return time_; }
	bool valid() const { return time_ != Speech::InvalidTimeframeIndex; }
    };

    class WordBoundaries : public Core::ReferenceCounted
    {
	struct Header;
    public:
	typedef Core::Vector<WordBoundary>::iterator iterator;
	typedef Core::Vector<WordBoundary>::const_iterator const_iterator;
    protected:
	Core::Vector<WordBoundary> internal_;
    protected:
	const_iterator begin() const {
	    return internal_.begin();
	}
    public:
	WordBoundaries() {}

	void clear() {
	    internal_.clear();
	}
	u32 size() const {
	    return internal_.size();
	}
	void grow(Fsa::StateId size) {
	    internal_.grow(size);
	}
	bool operator==(const WordBoundaries &wordBoundaries) const {
	    if (size() != wordBoundaries.size()) return false;
	    return std::equal(internal_.begin(), internal_.end(), wordBoundaries.begin());
	}
	bool operator!=(const WordBoundaries &wordBoundaries) const {
	    return !(*this == wordBoundaries);
	}
	virtual const WordBoundary& operator[](Fsa::StateId id) const {
	    require(id < size());
	    return internal_[id];
	}
	virtual WordBoundary& operator[](Fsa::StateId id) {
	    internal_.grow(id);
	    return internal_[id];
	}
	void set(Fsa::StateId id, const WordBoundary &wordBoundary) {
	    internal_.grow(id);
	    if (internal_[id].valid()) verify(internal_[id] == wordBoundary);
	    internal_[id] = wordBoundary;
	}
	void push_back(const WordBoundary &wordBoundary) {
	    internal_.push_back(wordBoundary);
	}
	Speech::TimeframeIndex time(Fsa::StateId id) const {
	    return internal_[id].time();
	}
	WordBoundary::Transit transit(Fsa::StateId id) const {
	    return internal_[id].transit();
	}
	/*
	 * Find word boundary with maximum timeframe index.
	 */
	const WordBoundary getFinalWordBoundary() const;

	bool writeBinary(Core::BinaryOutputStream&) const;
	bool readBinary(Core::BinaryInputStream&);
    };

    /*
     * A Word Lattice.
     *
     * A set (vector) of fsas All fsas are expected to be isomorphic,
     * i.e. they have the same state numbering, same arcs and same
     * lables; only the weights may differ.  The fsa are
     * LemmaPronunciation acceptors.  Word boundary information is
     * assciated with the states of the FSAs via the state id.
     *
     * Note: Previosly the word lattice was a LemmaPronunciation to
     * SyntacticToken transducer.  Although this was more consistent
     * with state-machine orthodoxy, the SyntacticToken labels have
     * proved to be not only redundant, but actually useless in
     * practice.  A convenience slave adaptor could be provided to
     * restore them.  This should then include the correct
     * pronunciation and syntactic class weights.  (Since these had
     * been missing, the SyntacticToken labels were useless.)
     *
     * Plans: In the future we want to to represent the set of FSA
     * automata by weight multiplexing: We use a single FSA with a
     * dummy semiring.  The weights of the automaton are actually
     * indices enumerating the arcs.  The different weight assignments
     * are represented by a set of vectors.  FSA adaptors (derived
     * from ModifyAutomaton) will be provided to emulate a normal
     * Fsa::Automaton with the selected weights.
     */

    class WordLattice : public Core::ReferenceCounted {
    public:
	/**
	 *  Name of the main fsa in the lattice.
	 *  Tipical usage: lattices with one fsa.
	 *  Remark: acousticFsa is used at many places as main fsa without
	 *  any real connection to the acoustics. This fsa name is introduced
	 *  to clean this up.
	 */
	static const char* mainFsa;
	/** Name for getting Fsa with acoustic scores. */
	static const char* acousticFsa;
	/** Name for getting Fsa with lnguage model scores. */
	static const char* lmFsa;
	/** Name for getting Fsa with word accuracies. */
	static const char* accuracyFsa;
	/** Name for getting Fsa with total scores. */
	static const char* totalFsa;
	/** Name for getting Fsa with posteriors. */
	static const char* posteriorFsa;

	typedef Core::Choice::Value PartIndex;
    protected:
	Core::Choice parts_;
	Core::Vector<Fsa::ConstAutomatonRef> fsas_;
	Core::Ref<const WordBoundaries> wordBoundaries_;
    public:
	WordLattice();

	const std::string& mainName() const {
	    require(fsas_.size() == 1);
	    return name(0);
	}
	const std::string& name(PartIndex index) const {
	    return parts_[index];
	}
	void setFsa(Fsa::ConstAutomatonRef fsa, const std::string &name) {
	    PartIndex index = parts_[name];
	    if (index == Core::Choice::IllegalValue) {
		parts_.addChoice(name.c_str(), fsas_.size());
		fsas_.push_back(fsa);
	    } else {
		fsas_[index] = fsa;
	    }
	}
	Fsa::ConstAutomatonRef mainPart() const {
	    require(!fsas_.empty());
	    return fsas_.front();
	}
	Fsa::ConstAutomatonRef part(size_t index) const {
	    require(index < fsas_.size());
	    return fsas_[index];
	}
	Fsa::ConstAutomatonRef part(const std::string &name) const {
	    PartIndex index = parts_[name];
	    require(index != Core::Choice::IllegalValue);
	    return fsas_[index];
	}
	size_t nParts() const { return fsas_.size(); }

	void setWordBoundaries(Core::Ref<const WordBoundaries> wordBoundaries) {
	    wordBoundaries_ = wordBoundaries;
	}
	Core::Ref<const WordBoundaries> wordBoundaries() const {
	    return wordBoundaries_;
	}
	const WordBoundary& wordBoundary(Fsa::StateId id) const {
	    return (*wordBoundaries_)[id];
	}
	Speech::TimeframeIndex time(Fsa::StateId id) const {
	    return (*wordBoundaries_)[id].time();
	}
	Speech::TimeframeIndex maximumTime() const;
	bool hasPart(const std::string &name) const {
	    return parts_[name] != Core::Choice::IllegalValue;
	}
    };

    /**
     * A "normal" word lattice with acoustic and language model scores
     * and word boundary information as it is produced by
     * WordConditionedTreeSearch.  This (badly named) class
     * facilitates the creation of word graph in
     * WordConditionedTreeSearch.  It will probably disappear in the
     * near future, when the WordLattice interface is refactored.
     */
    class StandardWordLattice :
	public WordLattice
    {
    private:
	Core::Ref<Fsa::StaticAutomaton> acoustic_, lm_;
	Fsa::State *initialState_, *finalState_;
    public:
	StandardWordLattice(Bliss::LexiconRef);

	Fsa::State *newState();
	Fsa::State *initialState() { return initialState_; }
	Fsa::State *finalState() { return finalState_; }

	void newArc(Fsa::State *source,
		    Fsa::State *target,
		    const Bliss::LemmaPronunciation*,
		    Speech::Score acoustic, Speech::Score lm);

	void addAcyclicProperty();
    };

    typedef Core::Ref<WordLattice> WordLatticeRef;
    typedef Core::Ref<const WordLattice> ConstWordLatticeRef;

    class SlaveWordLattice : public Fsa::SlaveAutomaton
    {
    protected:
	Core::Ref<const WordBoundaries> wordBoundaries_;
	const Fsa::State *initialState_, *finalState_;
    public:
	SlaveWordLattice(ConstWordLatticeRef wordLattice, size_t index = 0) :
	    Fsa::SlaveAutomaton(wordLattice->part(index)),
	    wordBoundaries_(wordLattice->wordBoundaries()),
	    initialState_(NULL), finalState_(NULL)
	{
	    if (fsa_) require(wordBoundaries_);
	}
	virtual ~SlaveWordLattice() {}

	Core::Ref<const WordBoundaries> wordBoundaries() const {
	    return wordBoundaries_;
	}
    };

    class ModifyWordLattice : public Fsa::ModifyAutomaton
    {
    protected:
	Core::Ref<const WordBoundaries> wordBoundaries_;
    public:
	ModifyWordLattice(ConstWordLatticeRef wordLattice, size_t index = 0) :
	    Fsa::ModifyAutomaton(wordLattice->part(index)),
	    wordBoundaries_(wordLattice->wordBoundaries()) {
	    if (fsa_) require(wordBoundaries_);
	}
	virtual ~ModifyWordLattice() {}

	Core::Ref<const WordBoundaries> wordBoundaries() const {
	    return wordBoundaries_;
	}
    };

    class DfsState : public Fsa::DfsState
    {
    protected:
	Core::Ref<const WordBoundaries> wordBoundaries_;
    public:
	DfsState() : Fsa::DfsState(Fsa::ConstAutomatonRef()) {}
	DfsState(ConstWordLatticeRef wordLattice, size_t index = 0) :
	    Fsa::DfsState(wordLattice->part(index)),
	    wordBoundaries_(wordLattice->wordBoundaries()) {
	}
	virtual ~DfsState() {}
    };

    class WordLatticeDescription : public Core::Description
    {
	typedef Core::Description Precursor;
    public:
	static const std::string defaultName;
	static const std::string nameModel;
	static const std::string nameLevel;
	static const std::string defaultLevel;
    private:
	/*
	 *  Initilizes the WordLatticeDescription by parameters of a lexicon.
	 */
	void initialize(ConstWordLatticeRef);
    public:
	/**
	 *  Initilizes the WordLatticeDescription.
	 */
	WordLatticeDescription(const std::string &);
	/**
	 *  Initilizes the WordLatticeDescription.
	 *  Name of the description is created form the full name of @c parent.
	 */
	WordLatticeDescription(const Core::Configurable &parent);
	/**
	 *  Initilizes the WordLatticeDescription by parameters of a lattice.
	 */
	WordLatticeDescription(const std::string &, ConstWordLatticeRef);
	/**
	 *  Initilizes the WordLatticeDescription by parameters of a lattice.
	 *  Name of the description is created form the full name of @c parent.
	 */
	WordLatticeDescription(const Core::Configurable &parent, ConstWordLatticeRef);
    };

    ConstWordLatticeRef timeConditionedWordLattice(ConstWordLatticeRef in);


} // namespace Lattice

#endif // _LATTICE_LATTICE_HH
