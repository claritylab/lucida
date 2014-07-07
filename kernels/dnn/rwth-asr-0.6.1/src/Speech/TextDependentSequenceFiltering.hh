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
#ifndef _SPEECH_TEXT_DEPENDENT_SEQUENCE_FILTERING_HH
#define _SPEECH_TEXT_DEPENDENT_SEQUENCE_FILTERING_HH

#include <Core/Hash.hh>
#include <Flow/Node.hh>
#include <Flow/Timestamp.hh>
#include <Flow/Vector.hh>
#include <Am/AcousticModel.hh>

namespace Speech {

    /**
     *  Converts alignment into sequence selection and/or calculates ratio of the not-filtered timeframes.
     *  Input:
     *    default port: alignment
     *  Outputs:
     *    default port: vector-bool i.e. selection of remaining timeframes
     *    ratio port: f32, i.e. #remaining / #all timeframes.
     *
     *  -Selection set types:
     *    -set of central phonemes; set name 'phoneme-selection'.
     *    -set of state identifiers (tipically 0, 1, 2); set name 'state-selection'
     *  -Parametrization of selection sets:
     *    -parameter 'symbols' is list of symbols inlcuded in the set (e.g. in configuration files:
     *     phoneme-selection.symbols=a v f; in flow files: phoneme-selection.symbols="a v f")
     *    -parameter 'complement': if true the symbol set is complemented (e.g. in configuration files:
     *     state-selection.complement=true; in flow files: state-selection.complement="true");
     *     Note: for empty sets, 'complement' is set automatically to 'true'.
     *  -Selection sets are in 'AND' relation, thus those timeframes are selected whose allophone
     *   states are included in all sets.
     *  -Sequence selection object is a vector of bool values. Timeframes not included in one of
     *   the selection sets are marked by false and timeframes included in all selection sets are marked by true.
     *   @see Flow::SequenceFilterNode for further explanations.
     */
    class AlignmentToSequenceSelectionNode : public Flow::Node {
    private:
	/** Base class for sets */
	class Set : public Core::Component, public Core::ReferenceCounted {
	private:
	    static const Core::ParameterStringVector paramSymbols;
	    static const Core::ParameterBool paramShallComplement;
	private:
	    std::vector<std::string> symbols_;
	    bool shallComplement_;
	    bool needInit_;
	private:
	    void storeSymbols(const std::vector<std::string> &symbols) { symbols_ = symbols; needInit_ = true; }
	    void storeComplementing(bool shallComplement) { setComplementing(shallComplement); needInit_ = true; }
	    void setComplementing(bool shallComplement) { shallComplement_ = shallComplement; }
	    /**
	     *  Removes the prefix 'object-name." from @param n.
	     *  @return is false if n does not begin with the prefix.
	     */
	    bool acceptParameterNamePrefix(std::string &n) const;
	protected:
	    Core::Ref<const Bliss::PhonemeInventory> phonemeInventory_;
	    Core::Ref<const Am::AllophoneStateAlphabet> allophoneStateAlphabet_;
	protected:
	    virtual void setSymbols(const std::vector<std::string> &) = 0;
	    bool complementIfRequired(bool contains) const { return shallComplement_ ? !contains : contains; }
	public:
	    Set(const Core::Configuration &c);
	    virtual ~Set();

	    /**
	     *  Set parameters according to the configuration.
	     *  Note: initilization is not possible in the constructor, because of the virtual functions.
	     *        Calling of initialize is thus essential after having set the parameters.
	     */
	    void initialize();
	    virtual bool setParameter(const std::string &name, const std::string &value);

	    virtual bool contains(const Am::AllophoneState &as, s16 position = 0) const = 0;
	    bool contains(Am::AllophoneStateIndex emission, s16 position = 0) const {
		return contains(allophoneStateAlphabet_->allophoneState(emission), position);
	    }
	    virtual bool empty() const = 0;
	};
	typedef std::vector<Core::Ref<Set> > Sets;

	/**
	 *  Set of phonemes (e.g.: vocals only).
	 *  Configuration:
	 *    necessary inputs:
	 *     -lexicon
	 *     -allophone state alphabet of a acoustic model.
	 *    set of phonemes:
	 *
	 *   Usage:
	 */
	class PhonemeSet : public Set {
	    typedef Set Precursor;
	private:
	    /**
	     *   All phonemes of the phoneme inventory of the lexicon are mapped to a bool value.
	     *   Workaround: bool is not supported by Bliss::PhonemeMap, so we have use char now.
	     */
	    Bliss::PhonemeMap<char> *phonemeMap_;
	    bool empty_;
	protected:
	    virtual void setSymbols(const std::vector<std::string> &);
	public:
	    PhonemeSet(const Core::Configuration &c) : Precursor(c), phonemeMap_(0), empty_(true) {}
	    virtual ~PhonemeSet() { delete phonemeMap_; }

	    bool containsPhoneme(const Bliss::Phoneme *p) const {
		return Precursor::complementIfRequired((bool)(*phonemeMap_)[p]);
	    }
	    bool containsPhoneme(const Bliss::Phoneme::Id id) const {
		return Precursor::complementIfRequired((bool)(*phonemeMap_)[id]);
	    }

	    virtual bool contains(const Am::AllophoneState &as, s16 position = 0) const {
		return containsPhoneme(as.allophone()->phoneme(position));
	    }
	    virtual bool empty() const { return empty_; }
	};

	/**
	 *  Set of HMM states identifier.
	 */
	class StateSet : public Set  {
	    typedef Set Precursor;
	private:
	    typedef s16 State;
	private:
	    Core::hash_set<State> set_;
	protected:
	    virtual void setSymbols(const std::vector<std::string> &);
	public:
	    StateSet(const Core::Configuration &c) : Precursor(c) {}
	    virtual ~StateSet() {}

	    virtual bool contains(const Am::AllophoneState &as, s16 position = 0) const {
		return Precursor::complementIfRequired(set_.find(as.state()) != set_.end());
	    }
	    virtual bool empty() const { return set_.empty(); }
	};
    private:
	Sets sets_;
    private:
	void initialize();
    public:
	AlignmentToSequenceSelectionNode(const Core::Configuration &c);
	virtual ~AlignmentToSequenceSelectionNode() {}
	static std::string filterName() { return "speech-alignment-to-sequence-selection"; }

	virtual Flow::PortId getInput(const std::string &name) {
	    return 0; }
	virtual Flow::PortId getOutput(const std::string &name) {
	    return name == "ratio" ? 1 : 0; }

	virtual bool setParameter(const std::string &name, const std::string &value);
	virtual bool configure();
	virtual bool work(Flow::PortId out);
    };

} // namespace Speech

#endif // _SPEECH_TEXT_DEPENDENT_SEQUENCE_FILTERING_HH
