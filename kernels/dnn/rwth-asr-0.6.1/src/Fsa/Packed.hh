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
#ifndef _FSA_PACKED_HH
#define _FSA_PACKED_HH

#include "Storage.hh"

namespace Fsa {

    class PackedAutomaton : public StorageAutomaton {
    private:
	struct PackedState {
	    size_t start_;
	    PackedState(size_t start) : start_(start) {}
	};
	Core::Vector<u8> arcs_;
	Core::Vector<PackedState> states_;
	size_t nArcs_, nStates_, pArcs_, pStates_;

    public:
	PackedAutomaton();
	PackedAutomaton(const std::string &str);
	virtual ~PackedAutomaton();
	virtual void clear();
	virtual bool hasState(StateId sid) const;
	virtual void setState(State *sp);
	virtual void deleteState(StateId);
	virtual ConstStateRef getState(StateId s) const;
	virtual StateId maxStateId() const { return states_.size() - 1; }
	virtual StateId size() const { return states_.size(); }
	virtual void normalize() {}
	virtual size_t getMemoryUsed() const;
	float getStateRatio() const;
	float getArcRatio() const;
	float getRatio() const;
	void dumpMemoryUsage(Core::XmlWriter &o) const;
	virtual std::string describe() const { return "packed"; }
    };

    Core::Ref<PackedAutomaton> packedCopy(ConstAutomatonRef);

} // namespace Fsa

#endif // _FSA_PACKED_HH
