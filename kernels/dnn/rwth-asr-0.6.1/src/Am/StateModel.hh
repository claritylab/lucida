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
#ifndef _AM_STATE_MODEL_HH
#define _AM_STATE_MODEL_HH

#include <Core/Component.hh>
#include <Core/Hash.hh>
#include <Bliss/Phonology.hh>
#include <Bliss/Fsa.hh>
#include <Fsa/Alphabet.hh>
#include <Fsa/Static.hh>
#include <Mm/Types.hh>

namespace Am {

    typedef Mm::MixtureIndex EmissionLabel;

    class EmissionAlphabet : public Fsa::Alphabet {
    private:
	Mm::MixtureIndex nMixtures_;
	mutable u32 nDisambiguators_;
    public:
	EmissionAlphabet(Mm::MixtureIndex nMixtures = 0) :
	    nMixtures_(nMixtures), nDisambiguators_(0) {}
	Mm::MixtureIndex nMixtures() const { return nMixtures_; }
	u32 nDisambiguators() const { return nDisambiguators_; }
	Fsa::LabelId disambiguator(u32 d) const {
	    if (nDisambiguators_ >= d) nDisambiguators_ = d + 1;
	    return nMixtures_ + d;
	}
	virtual bool isDisambiguator(Fsa::LabelId m) const { return m >= Fsa::LabelId(nMixtures_); }

	virtual const_iterator end() const {
	    return const_iterator(Fsa::ConstAlphabetRef(this), nMixtures_ + nDisambiguators_);
	}
	virtual std::string symbol(Fsa::LabelId) const;
	virtual void writeXml(Core::XmlWriter&) const;
    };

    typedef Bliss::ContextPhonology Phonology;

    struct Allophone: public Phonology::Allophone {
	typedef Phonology::Allophone Precursor;
	s16 boundary;
	static const u8 isInitialPhone = 1;
	static const u8 isFinalPhone   = 2;
	Allophone() {}
	Allophone(const Phonology::Allophone &a, s16 b) :
	    Phonology::Allophone(a), boundary(b) {}
	struct Hash {
	    Phonology::Allophone::Hash ah;
	    u32 operator()(const Allophone &a) const {
		return ah(a) ^ (u32(a.boundary) << 13);
	    }
	};
    };

    struct AllophoneState : public Allophone {
	typedef Allophone Precursor;
	s16 state;
	AllophoneState() {}
	AllophoneState(const Allophone &a, s16 s) : Precursor(a), state(s) {}
	AllophoneState(const AllophoneState &as) : Precursor(as), state(as.state) {}

	bool operator== (const AllophoneState &rhs) const {
	    return Precursor::operator==(rhs)
		&& state == rhs.state;
	}
	struct Hash {
	    Precursor::Hash ah;
	    u32 operator()(const AllophoneState &a) const {
		return ah(a) ^ (u32(a.state) << 21);
	    }
	};
    };

    typedef u32 AllophoneStateIndex;

    class AllophoneStateAlphabet : public Fsa::Alphabet {
    public:
	typedef Fsa::LabelId Index;
    private:
	Core::Ref<const Bliss::PhonemeInventory> pi_;
	u32 contextLength_;
	u32 nStates_;
	void calcNClasses();
	u32 nClasses_;
	mutable u32 nDisambiguators_;
    public:
	AllophoneStateAlphabet();
	explicit AllophoneStateAlphabet(
	    Core::Ref<const Bliss::PhonemeInventory>,
	    u32 contextLength = 0, u32 nStates = 0);
	void set(Core::Ref<const Bliss::PhonemeInventory>, u32 contextLength, u32 nStates);
	Core::Ref<const Bliss::PhonemeInventory> phonemeInventory() const { return pi_; }
	Fsa::LabelId nClasses() const { return nClasses_; }
	Fsa::LabelId index(const AllophoneState&) const;
	AllophoneState allophoneState(Fsa::LabelId) const;
	u32 nDisambiguators() const { return nDisambiguators_; }
	Fsa::LabelId disambiguator(u32 d) const {
	    if (nDisambiguators_ >= d) nDisambiguators_ = d + 1;
	    return nClasses_ + 1 + d;
	}
	virtual bool isDisambiguator(Fsa::LabelId m) const { return m > Fsa::LabelId(nClasses_); }

	virtual const_iterator end() const {
	    return const_iterator(Fsa::ConstAlphabetRef(this), nClasses_ + 1 + nDisambiguators_);
	}
	virtual Fsa::LabelId next(Fsa::LabelId id) const { return ++id; }
	virtual std::string symbol(Fsa::LabelId) const;
	virtual void writeXml(Core::XmlWriter &os) const;
    };


    class EmissionToPhonemeTransducer : public Fsa::StaticAutomaton {
    public:
	EmissionToPhonemeTransducer() {}
	EmissionToPhonemeTransducer(u32 nMixtures, Core::Ref<const Bliss::PhonemeAlphabet>);
	const EmissionAlphabet* emissionAlphabet() const {
	    return dynamic_cast<const EmissionAlphabet*>(getInputAlphabet().get());
	}
	const Bliss::PhonemeAlphabet* phonemeAlphabet() const {
	    return dynamic_cast<const Bliss::PhonemeAlphabet*>(getOutputAlphabet().get());
	}
    };


    class AllophoneStateToPhonemeTransducer : public Fsa::StaticAutomaton {
    public:
	AllophoneStateToPhonemeTransducer() {}
	AllophoneStateToPhonemeTransducer(Core::Ref<const Bliss::PhonemeInventory>);
	const AllophoneStateAlphabet* allophoneStateAlphabet() const {
	    return dynamic_cast<const AllophoneStateAlphabet*>(getInputAlphabet().get());
	}
	const Bliss::PhonemeAlphabet* phonemeAlphabet() const {
	    return dynamic_cast<const Bliss::PhonemeAlphabet*>(getOutputAlphabet().get());
	}
    };

    class StateTying : public virtual Core::Component {
    private:
	typedef Core::HashMap<AllophoneStateAlphabet::Index, Mm::MixtureIndex> CacheMap;
	mutable CacheMap classifyIndexCache_;
    protected:
	const AllophoneStateAlphabet &alphabet_;
    public:
	StateTying(const Core::Configuration &c, const AllophoneStateAlphabet &a) :
	    Component(c), alphabet_(a) {}
	virtual ~StateTying() {}
	virtual void getDependencies(Core::DependencySet&) const {}
	const AllophoneStateAlphabet &allophoneStateAlphabet() const { return alphabet_; }
	virtual Mm::MixtureIndex nClasses() const = 0;
	virtual Mm::MixtureIndex classify(const AllophoneState &as) const = 0;
	virtual Mm::MixtureIndex classifyIndex(AllophoneStateAlphabet::Index index) const {
	    CacheMap::const_iterator iter = classifyIndexCache_.find(index);
	    if (iter == classifyIndexCache_.end()) {
		Mm::MixtureIndex mix = classify(alphabet_.allophoneState(index));
		classifyIndexCache_[AllophoneStateIndex(index)] = mix;
		return mix;
	    }
	    else {
		return iter->second;
	    }
	}
    };

    class NoStateTying : public StateTying {
    public:
	NoStateTying(const Core::Configuration &c, const AllophoneStateAlphabet &a) :
	    Component(c), StateTying(c, a) {}

	virtual Mm::MixtureIndex nClasses() const {
	    return alphabet_.nClasses();
	}
	virtual Mm::MixtureIndex classifyIndex(AllophoneStateAlphabet::Index i) const {
	    return i;
	}
	virtual Mm::MixtureIndex classify(const AllophoneState &as) const {
	    return alphabet_.index(as);
	}
    };

} // namespace Am

#endif // _AM_STATE_MODEL_HH
