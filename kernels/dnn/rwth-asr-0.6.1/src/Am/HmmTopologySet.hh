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
#ifndef _AM_HMM_TOPOLOGY_SET_HH
#define _AM_HMM_TOPOLOGY_SET_HH


#include <list>
#include <vector>

#include <Core/Component.hh>
#include <Core/Types.hh>


namespace Am {

    class HmmTopology : public Core::Component {
    public:
	typedef u32 StateIndex;

    private:
	struct Transition { StateIndex from_, to_; };
	std::vector<StateIndex> states_;
	std::vector<Transition> transitions_;

    public:
	HmmTopology(const Core::Configuration &c) : Core::Component(c) {}

	void addState(const StateIndex s, const std::string &tie = "");
	void addTransition(const StateIndex from, const StateIndex to);
	void addHmmTopology(const HmmTopology &t, const StateIndex start);
	void scale(const u32 scale);

	friend std::ostream& operator<< (std::ostream &o, const HmmTopology &t)  {
	    o << t.name();
	    return o;
	}
    };


    class HmmTopologySet :
	public Core::Component,
	public Core::ReferenceCounted
    {
    private:
	std::list<HmmTopology*> topologies_;
	HmmTopology* default_silence_;
	HmmTopology* default_acoustic_unit_;

    public:
	HmmTopologySet(const Core::Configuration &c, const std::string &name);

	bool addHmmTopology(HmmTopology *t);
	bool setDefaultSilence(const std::string &name);
	bool setDefaultAcousticUnit(const std::string &name);

	const HmmTopology* getHmmTopology(const std::string &name);
	const HmmTopology* getDefaultSilence()
	    { return default_silence_; }
	const HmmTopology* getDefaultAcousticUnit()
	    { return default_acoustic_unit_; }

	friend std::ostream& operator<< (std::ostream &o, const HmmTopologySet &h)  {
	    return o;
	}
    };
    typedef Core::Ref<HmmTopologySet> HmmTopologySetRef;
}


#endif // _AM_HMM_TOPOLOGY_SET_HH
