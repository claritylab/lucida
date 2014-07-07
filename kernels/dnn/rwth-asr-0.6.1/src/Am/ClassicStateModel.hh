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
#ifndef _AM_CLASSIC_STATE_MODEL_HH
#define _AM_CLASSIC_STATE_MODEL_HH

#include <Core/Component.hh>
#include <Core/ReferenceCounting.hh>
#include <Core/Hash.hh>
#include <Bliss/Phonology.hh>
#include <Bliss/Fsa.hh>
#include <Fsa/Alphabet.hh>
#include <Am/ClassicHmmTopologySet.hh>

namespace Am {
    typedef ClassicHmmTopologySet HmmTopologySet;
    typedef ClassicHmmTopologySetRef HmmTopologySetRef;



    class Phonology : public Bliss::ContextPhonology {
	typedef Bliss::ContextPhonology Precursor;
    public:
	static const Core::ParameterInt  paramHistoryLength;
	static const Core::ParameterInt  paramFutureLength;
	static const Core::ParameterBool paramCrossWord;
    private:
	bool isCrossWord_;
    public:
	Phonology(const Core::Configuration &config, Bliss::PhonemeInventoryRef pi);
	bool isCrossWord() const { return isCrossWord_; }
	void setCrossWord(bool isCrossWord);
    };
    typedef Core::Ref<const Phonology> ConstPhonologyRef;


    struct Allophone: public Phonology::Allophone {
	typedef Phonology::Allophone Precursor;
	u8 boundary;
	static const u8 isWithinPhone;
	static const u8 isInitialPhone;
	static const u8 isFinalPhone;
	Allophone() :
	    Precursor() {}
	Allophone(Bliss::Phoneme::Id phoneme, s16 b) :
	    Precursor(phoneme), boundary(b) {}
	Allophone(const Phonology::Allophone &a, s16 b) :
	    Precursor(a), boundary(b) {}

	bool operator== (const Allophone &allo) const {
	    return (boundary == allo.boundary)
		&& Precursor::operator==(allo);
	}

	struct Hash {
	    Phonology::Allophone::Hash ah;
	    u32 operator()(const Allophone &a) const {
		return ah(a) ^ (u32(a.boundary) << 13);
	    }
	};

	bool hasBoundary() { return boundary != 0; }
    };
    typedef Fsa::LabelId AllophoneIndex;


    class ClassicTransducerBuilder;

    /*
      The first 26 bits of the allphone state id are used for the allophone index;
      the latter 6 bits for the state
      => the max. id for the allophone index is 2^26
    */
    class AllophoneAlphabet : public Fsa::Alphabet, public Core::Component {
	typedef Fsa::Alphabet Precursor;
	friend class ClassicTransducerBuilder;
    public:
	static const Fsa::LabelId MaxId             = 0x03ffffff;
	static const Fsa::LabelId DisambiguatorFlag = 0x40000000;

	static const Fsa::LabelId IdMask            = 0x03ffffff;
	static const Fsa::LabelId DisambiguatorMask = 0x3fffffff;

	static const Core::ParameterString paramAddFromFile;
	static const Core::ParameterString paramStoreToFile;
	static const Core::ParameterBool paramAddFromLexicon;
	static const Core::ParameterBool paramAddAll;

    private:
	struct AllophonePtrHash {
	    Allophone::Hash h;
	    size_t operator()(const Allophone* const a) const { return h(*a); }
	};
	struct AllophonePtrEq {
	    bool operator()(const Allophone* const a, const Allophone* const b) const { return *a == *b; }
	};
	typedef Core::hash_map<const Allophone*, Fsa::LabelId, AllophonePtrHash, AllophonePtrEq> AllophoneMap;
    public:
	typedef std::vector<const Allophone*> AllophoneList;

    private:
	ConstPhonologyRef phonology_;
	Core::Ref<const Bliss::PhonemeInventory> pi_;
	bool isCrossWord_;
	mutable AllophoneMap allophoneMap_;
	mutable AllophoneList allophones_;
	typedef Core::hash_set<Bliss::Phoneme::Id> PhonemeIdSet;
	mutable PhonemeIdSet silPhonemes_;

	mutable Fsa::LabelId nDisambiguators_;

    protected:
	/*
	  takes ownership
	*/
	std::pair<AllophoneMap::iterator, bool> insert(const Allophone *allo) const;
	std::pair<AllophoneMap::iterator, bool> insert(const Allophone &allo) const;
	std::pair<AllophoneMap::iterator, bool> createAndInsert
	(const std::vector<bool> &cd, const Bliss::Pronunciation &p, s32 b, s32 c, s32 e, s16 boundary) const;

	/*
	  reset
	*/
	void clear();

	/*
	  Index all allophones
	*/
	void add(Core::Ref<const Bliss::PhonemeInventory> pi);
	/*
	  Index allophones in lexicon
	*/
	void add(Bliss::LexiconRef lexicon);
	/*
	  Index allophones listed in file
	*/
	void load(const std::string &filename);

	/*
	  Store list of allophones in alphabet
	*/
	void store(const std::string &filename);

	/*
	  Remember silence phonemes from lexicon
	*/
	void setSilence(Bliss::LexiconRef lexicon);

    public:
	AllophoneAlphabet(const Core::Configuration &config, ConstPhonologyRef phonology, Bliss::LexiconRef lexicon);
	virtual ~AllophoneAlphabet();

	const Allophone * allophone(Fsa::LabelId) const;
	Fsa::LabelId index(const Allophone*, bool create = true) const;
	bool hasIndex(const Allophone*) const;

	const Allophone * allophone(const Allophone &allo) const
	    { return allophone(index(&allo)); }
	/* If 'create' is true, a new label is created for allophones which don't have a label yet,
	 * otherwise Fsa::InvalidLabelId is returned in such cases. */
	Fsa::LabelId index(const Allophone &allo, bool create = true) const
	    { return index(&allo, create); }

	virtual std::string symbol(Fsa::LabelId) const;
	virtual Fsa::LabelId index(const std::string &symbol) const;

	Allophone fromString(const std::string &s) const;
	std::string toString(const Allophone &allo) const;

	bool isSilence(const Allophone &allo) const
	    { return silPhonemes_.find(allo.central()) != silPhonemes_.end(); }

	const AllophoneList & allophones() const {
	    return allophones_;
	}

	ConstPhonologyRef phonology() const {
	    return phonology_;
	}
	Fsa::LabelId nClasses() const {
	    return allophones_.size();
	}

	Fsa::LabelId nDisambiguators() const {
	    return nDisambiguators_;
	}
	Fsa::LabelId disambiguator(Fsa::LabelId d) const;
	virtual bool isDisambiguator(Fsa::LabelId id) const;

	virtual const_iterator begin() const;
	virtual const_iterator end() const;

	virtual void writeXml(Core::XmlWriter &os) const;
	void dump(Core::XmlWriter &xml) const;
	void dumpPlain(std::ostream &os) const;
    };
    typedef Core::Ref<const AllophoneAlphabet> ConstAllophoneAlphabetRef;


    class AllophoneState {
	friend class AllophoneStateAlphabet;
    private:
	const Allophone *allo_;
	s16 state_;
    private:
	AllophoneState(const Allophone *allo, s16 state) :
	    allo_(allo), state_(state) {}
    public:
	AllophoneState() :
	    allo_(0), state_(0) {}

	operator const Allophone*() const {
	    require(allo_);
	    return allo_;
	}

	operator const Allophone&() const {
	    require(allo_);
	    return *allo_;
	}

	AllophoneState & operator= (const AllophoneState &alloState) {
	    allo_ = alloState.allo_;
	    state_ = alloState.state_;
	    return *this;
	}

	bool operator== (const AllophoneState &alloState) const {
	    return (allo_ == alloState.allo_)
		&& (state_ == alloState.state_);
	}

	const Allophone *allophone() const { require(allo_); return allo_; }
	s16 state() const { return state_; }

	struct Hash {
	    Allophone::Hash ah;
	    u32 operator()(const AllophoneState &alloState) const {
		return ah(alloState) ^ (u32(alloState.state()) << 21);
	    }
	};
    };
    typedef Fsa::LabelId AllophoneStateIndex;


    class AllophoneStateAlphabet;
    typedef Core::Ref<const AllophoneStateAlphabet> ConstAllophoneStateAlphabetRef;


    class AllophoneStateIterator {
	friend class AllophoneStateAlphabet;
    private:
	ConstAllophoneStateAlphabetRef asAlphabet_;
	mutable AllophoneAlphabet::AllophoneList::const_iterator itAllo_, endAllo_;
	mutable s16 state_, endState_;
    private:
	AllophoneStateIterator(ConstAllophoneStateAlphabetRef asAlphabet);
	void resetState() const;
    public:
	void operator++ () const
	    { if (++state_ == endState_) { ++itAllo_; resetState(); } }
	bool operator!= (const AllophoneStateIterator &it) const
	    { return (itAllo_ != it.itAllo_) || (state_ != it.state_); }
	AllophoneState operator* () const
	    { return allophoneState(); }
	Fsa::LabelId id() const;
	AllophoneState allophoneState () const;
    };


    class AllophoneStateAlphabet : public Fsa::Alphabet, public Core::Component {
	friend class AllophoneStateIterator;
    public:
	static const s16 MaxStateId = 0x000f;
	static const Fsa::LabelId StateMask = 0x3c000000;

    private:
	ConstAllophoneAlphabetRef allophoneAlphabet_;
	HmmTopologySetRef hmmTopologies_;

    public:
	AllophoneStateAlphabet(
	    const Core::Configuration &config,
	    ConstAllophoneAlphabetRef allophoneAlphabet,
	    ClassicHmmTopologySetRef hmmTopologies);

	bool isSilence(const AllophoneState &alloState) const
	    { return allophoneAlphabet_->isSilence(alloState); }

	AllophoneState allophoneState(Fsa::LabelId) const;
	AllophoneState allophoneState(const Allophone *allo, s16 state) const;
	Fsa::LabelId index(const AllophoneState&) const;

	std::string toString(const AllophoneState &alloState) const;

	virtual Fsa::LabelId index(const std::string &symbol) const {
	    return index(symbol, true);
	}

	Fsa::LabelId index(const std::string &symbol, bool create) const;

	virtual std::string symbol(Fsa::LabelId) const;

	/* If 'create' is true, a new label is created for allophone states which don't have a label yet,
	 * otherwise Fsa::InvalidLabelId is returned in such cases. */
	Fsa::LabelId index(const Allophone *allo, s16 state, bool create = true) const;
	std::pair<Fsa::LabelId, s16> allophoneIndexAndState(Fsa::LabelId) const;

	std::pair<AllophoneStateIterator, AllophoneStateIterator> allophoneStates() const;

	Fsa::LabelId nClasses() const;

	Fsa::LabelId nDisambiguators() const {
	    return allophoneAlphabet_->nDisambiguators();
	}
	Fsa::LabelId disambiguator(Fsa::LabelId d) const {
	    return allophoneAlphabet_->disambiguator(d);
	}
	virtual bool isDisambiguator(Fsa::LabelId id) const {
	    return allophoneAlphabet_->isDisambiguator(id);
	}

	virtual const_iterator begin() const;
	virtual const_iterator end() const;

	virtual void writeXml(Core::XmlWriter &os) const;
	void dump(Core::XmlWriter &xml) const;
	void dumpPlain(std::ostream &os) const;
    };


    class ClassicStateModel : public Core::ReferenceCounted {
	friend class ClassicAcousticModel;
    private:
	ConstPhonologyRef phonologyRef_;
	Core::Ref<const Bliss::PhonemeInventory> piRef_;
	ConstAllophoneAlphabetRef allophoneAlphabetRef_;
	ConstAllophoneStateAlphabetRef allophoneStateAlphabetRef_;
	HmmTopologySetRef hmmTopologySetRef_;
	std::vector<std::string> conditions_;

    public:
	ClassicStateModel(
	    ConstPhonologyRef phonologyRef,
	    ConstAllophoneAlphabetRef allophoneAlphabetRef,
	    ConstAllophoneStateAlphabetRef allophoneStateAlphabetRef,
	    HmmTopologySetRef hmmTopologySetRef,
	    std::vector<std::string> conditions = std::vector<std::string>());

	const Phonology & phonology() const { return *phonologyRef_; }
	ConstPhonologyRef getPhonology() const { return phonologyRef_; }

	const Bliss::PhonemeInventory & phonemeInventory() const { return *piRef_; }
	Core::Ref<const Bliss::PhonemeInventory> getPhonemeInventory() const { return piRef_; }

	const AllophoneAlphabet & allophoneAlphabet() const { return *allophoneAlphabetRef_; }
	ConstAllophoneAlphabetRef getAllophoneAlphabet() const { return allophoneAlphabetRef_; }

	const AllophoneStateAlphabet & allophoneStateAlphabet() const { return *allophoneStateAlphabetRef_; }
	ConstAllophoneStateAlphabetRef getAllophoneStateAlphabet() const { return allophoneStateAlphabetRef_; }
	AllophoneState allophoneState(const Allophone *allo, s16 state) const;

	const HmmTopologySet & hmmTopologySet() const { return *hmmTopologySetRef_; }
	HmmTopologySetRef getHmmTopologySet() const { return hmmTopologySetRef_; }
	const ClassicHmmTopology * hmmTopology(const Allophone *allo) const;

	const std::vector<std::string> & conditions() const { return conditions_; }
    };
    typedef Core::Ref<const ClassicStateModel> ClassicStateModelRef;

} // namespace Am

#endif // _AM_CLASSIC_STATE_MODEL_HH
