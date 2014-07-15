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
#ifndef _AM_CLASSIC_STATE_TYING_HH
#define _AM_CLASSIC_STATE_TYING_HH

#include <Core/Component.hh>
#include <Core/Parameter.hh>
#include <Core/ReferenceCounting.hh>
#include <Core/Version.hh>
#include <Core/Hash.hh>
#include <Bliss/Phonology.hh>
#include <Bliss/Fsa.hh>
#include <Fsa/Alphabet.hh>
#include <Fsa/Static.hh>
#include <Mm/Types.hh>
#include <Am/ClassicStateModel.hh>

namespace Am {

    typedef Mm::MixtureIndex EmissionLabel;

    class EmissionAlphabet : public Fsa::Alphabet {
    private:
	Mm::MixtureIndex nMixtures_;
	mutable Fsa::LabelId nDisambiguators_;
    public:
	EmissionAlphabet(Mm::MixtureIndex nMixtures = 0) :
	    nMixtures_(nMixtures), nDisambiguators_(0) {}
	Mm::MixtureIndex nMixtures() const { return nMixtures_; }
	Fsa::LabelId nDisambiguators() const { return nDisambiguators_; }
	Fsa::LabelId disambiguator(Fsa::LabelId d) const {
	    if (nDisambiguators_ >= d) nDisambiguators_ = d + 1;
	    return nMixtures_ + d;
	}
	virtual bool isDisambiguator(Fsa::LabelId m) const {
	    return m >= Fsa::LabelId(nMixtures_);
	}
	virtual const_iterator end() const {
	    return const_iterator(Fsa::ConstAlphabetRef(this),
				  nMixtures_ + nDisambiguators_);
	}

	virtual std::string symbol(Fsa::LabelId) const;
	virtual Fsa::LabelId index(const std::string&) const;
	virtual void writeXml(Core::XmlWriter &os) const;
    };
    typedef Core::Ref<const EmissionAlphabet> ConstEmissionAlphabetRef;


    class ClassicStateTying :
	public virtual Core::Component,
	public Core::ReferenceCounted {
    public:
	static const Core::ParameterString paramFilename;
    private:
	typedef Core::HashMap<AllophoneStateIndex, Mm::MixtureIndex> CacheMap;
	mutable CacheMap classifyIndexCache_;
    protected:
	ConstAllophoneStateAlphabetRef alphabetRef_;
	mutable Core::Channel classifyDumpChannel_;
    public:
	ClassicStateTying(const Core::Configuration & config, ClassicStateModelRef stateModel) :
	    Core::Component(config),
	    alphabetRef_(stateModel->getAllophoneStateAlphabet()),
	    classifyDumpChannel_(config, "dump-state-tying") {}
	virtual ~ClassicStateTying() {}
	virtual void getDependencies(Core::DependencySet&) const {}
	virtual Mm::MixtureIndex nClasses() const = 0;
	virtual Mm::MixtureIndex classify(const AllophoneState &as) const = 0;
	virtual Mm::MixtureIndex classifyIndex(AllophoneStateIndex index) const {
	    CacheMap::const_iterator iter = classifyIndexCache_.find(index);
	    if (iter == classifyIndexCache_.end()) {
		Mm::MixtureIndex mix = classify(alphabetRef_->allophoneState(index));
		classifyIndexCache_[index] = mix;
		return mix;
	    }
	    else
		return iter->second;
	}
	ConstAllophoneStateAlphabetRef allophoneStateAlphabet() const { return alphabetRef_; }

	/*
	  //TODO
	virtual void set(const Conditions & cond) {}
	*/

	/** creates a classic state tying reference for a given config file and a state model reference. */
	static Core::Ref<const ClassicStateTying> createClassicStateTyingRef(
	    const Core::Configuration & config, ClassicStateModelRef stateModel);

	Fsa::ConstAutomatonRef createMixtureToAllophoneStateTransducer(
	    s32 nDisambiguators = 0) const;

	void dumpStateTying(Core::Channel &dump) const;
    };
    typedef Core::Ref<const ClassicStateTying> ClassicStateTyingRef;



    // ============================================================================



    class NoStateTying :
	public ClassicStateTying {
    public:
	NoStateTying(const Core::Configuration & config, ClassicStateModelRef stateModel) :
	    Core::Component(config),
	    ClassicStateTying(config, stateModel) {
	    if (classifyDumpChannel_.isOpen())
		dumpStateTying(classifyDumpChannel_);
	}

	virtual Mm::MixtureIndex nClasses() const {
	    return alphabetRef_->nClasses();
	}
	virtual Mm::MixtureIndex classifyIndex(AllophoneStateIndex i) const {
	    return i;
	}
	virtual Mm::MixtureIndex classify(const AllophoneState &as) const {
	    return alphabetRef_->index(as);
	}
    };



    // ============================================================================



    class MonophoneStateTying :
	public ClassicStateTying {
    private:
	typedef std::vector<Mm::MixtureIndex> MixtureIndexList;

    private:
	Mm::MixtureIndex nPhonemes_;
	Mm::MixtureIndex nClasses_;
	MixtureIndexList classIds_;

    public:
	MonophoneStateTying(const Core::Configuration & config, ClassicStateModelRef stateModel);

	Mm::MixtureIndex nClasses() const {
	    return nClasses_;
	}
	Mm::MixtureIndex classifyIndex(AllophoneStateIndex i) const {
	    return classify(alphabetRef_->allophoneState(i));
	}
	Mm::MixtureIndex classify(const AllophoneState &as) const {
	    return classIds_[as.allophone()->central() + as.state() * nPhonemes_];
	}
    };



    // ============================================================================



    class LutStateTying :
	public ClassicStateTying {
    private:
	typedef Core::HashMap<AllophoneStateIndex, Mm::MixtureIndex> LookupTable;

    private:
	Mm::MixtureIndex nClasses_;
	LookupTable lut_;

    private:
	bool loadLut(const std::string & filename);

    public:
	LutStateTying(const Core::Configuration & config, ClassicStateModelRef stateModel) :
	    Core::Component(config),
	    ClassicStateTying(config, stateModel),
	    nClasses_(0) {
	    if (!loadLut(paramFilename(config))) {
		error("error while reading lookup table from file \"%s\"", paramFilename(config).c_str());
		return;
	    }
	}

	Mm::MixtureIndex nClasses()const { return nClasses_; }

	Mm::MixtureIndex classify(const AllophoneState &as) const {
	    return classifyIndex(alphabetRef_->index(as));
	}

	Mm::MixtureIndex classifyIndex(AllophoneStateIndex index) const {
	    LookupTable::const_iterator it = lut_.find(index);
	    if (it != lut_.end()) {
		return it->second;
	    } else {
		error("unknown allophone state index %d=%s", index, alphabetRef_->symbol(index).c_str());
		return Mm::invalidMixture;
	    }
	}
    };

} // namespace Am

#endif // _AM_CLASSIC_STATE_TYING_HH
