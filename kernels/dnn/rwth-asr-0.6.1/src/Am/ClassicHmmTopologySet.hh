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
#ifndef _AM_CLASSIC_HMM_TOPOLOGY_SET_HH
#define _AM_CLASSIC_HMM_TOPOLOGY_SET_HH

#include <Core/Component.hh>
#include <Core/Dependency.hh>
#include <Bliss/Phoneme.hh>

namespace Am {

    /**
     *  ClassicHmmTopology
     */
    class ClassicHmmTopology {
	int nPhoneStates_;
	int nSubStates_;
    public:
	ClassicHmmTopology(int nPhoneStates, int nSubStates) :
	    nPhoneStates_(nPhoneStates), nSubStates_(nSubStates) {}
	int nPhoneStates() const { return nPhoneStates_; }
	int nSubStates() const { return nSubStates_; }
    };

    /**
     *  ClassicHmmTopologySet
     */
    class ClassicHmmTopologySet :
	public virtual Core::Component,
	public Core::ReferenceCounted  {
    public:
	static const Core::ParameterInt paramPhoneStates;
	static const Core::ParameterInt paramRepeatedStates;
	static const Core::ParameterBool paramAcrossWordModel;
    private:
	Bliss::Phoneme::Id silenceId_;
	ClassicHmmTopology silence_;

	ClassicHmmTopology default_;

	bool isAcrossWordModelEnabled_;
    public:
	ClassicHmmTopologySet(const Core::Configuration&, Bliss::Phoneme::Id silenceId);

	void getDependencies(Core::DependencySet&) const;

	const ClassicHmmTopology *get(Bliss::Phoneme::Id phoneme) const {
	    return (phoneme != silenceId_ ? &default_ : &silence_);
	}
	const ClassicHmmTopology &getDefault() const { return default_; }
	const ClassicHmmTopology &getSilence() const { return silence_; }

	bool isAcrossWordModelEnabled() const { return isAcrossWordModelEnabled_; }
    };
    typedef Core::Ref<const ClassicHmmTopologySet> ClassicHmmTopologySetRef;
} // namespace Am

#endif // _AM_CLASSIC_HMM_TOPOLOGY_SET_HH
