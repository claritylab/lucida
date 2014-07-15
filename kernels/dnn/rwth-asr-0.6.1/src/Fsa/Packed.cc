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
#include <Core/Assertions.hh>
#include "Packed.hh"
#include "Sort.hh"
#include "Utility.hh"

namespace Fsa {

    PackedAutomaton::PackedAutomaton() {
	clear();
    }

    PackedAutomaton::PackedAutomaton(const std::string &str) {
	clear();
	copy(this, str);
    }

    PackedAutomaton::~PackedAutomaton() {
	clear();
    }

    void PackedAutomaton::clear() {
	arcs_.erase(arcs_.begin(), arcs_.end());
	states_.erase(states_.begin(), states_.end());
	nArcs_ = nStates_ = pArcs_ = pStates_ = 0;
    }

    void PackedAutomaton::setState(State *sp) {
	static PackedState init(Core::Type<size_t>::max);
	states_.grow(sp->id(), init);
	require(states_[sp->id()].start_ == Core::Type<size_t>::max);

	if (arcs_.size() > 0) arcs_.pop_back(); // rewind as we can overwrite the automaton end marker
	states_[sp->id()].start_ = arcs_.size();
	u8 partition = (sp->tags() >> StateIdBits) << 5;
	if (sp->hasTags(StateTagFinal))
	    if (!semiring()->isDefault(sp->weight_)) partition |= 0x01;
	arcs_.push_back(partition | 0x1c);
	if (partition & 0x01) appendBytes(arcs_, sp->weight_, sizeof(Weight));
	size_t start = arcs_.size();
	pStates_ += start - states_[sp->id()].start_;

	for (State::const_iterator a = sp->begin(); a != sp->end(); ++a) {
	    partition = (estimateBytes(a->target()) - 1) << 6;
	    if (!semiring()->isDefault(a->weight())) partition |= 0x20;
	    int i = 0;
	    if (a->input() != Epsilon) i = estimateBytes(a->input());
	    int o = 0;
	    if (type() != TypeAcceptor)
		if (a->output() != Epsilon) o = estimateBytes(a->output());
	    partition |= (5 * o) + i;
	    arcs_.push_back(partition);

	    appendBytes(arcs_, a->target(), (partition >> 6) + 1);
	    if (partition & 0x20) appendBytes(arcs_, a->weight(), sizeof(Weight));
	    appendBytes(arcs_, a->input(), i);
	    if (type() != TypeAcceptor) appendBytes(arcs_, a->output(), o);
	    nArcs_++;
	}
	pArcs_ += arcs_.size() - start;
	arcs_.push_back(0xff); // automaton end marker
	nStates_++;
	delete sp;
    }

    void PackedAutomaton::deleteState(StateId) {
	std::cerr << "PackedAutomaton::deleteState() not implemented." << std::endl;
	defect();
    }

    ConstStateRef PackedAutomaton::getState(StateId s) const {
	if (s < states_.size()) {
	    Core::Vector<u8>::const_iterator a = arcs_.begin() + states_[s].start_;
	    u8 partition = *(a++);
	    StateTag tags = StateTag(partition & 0xe0) << (StateIdBits - 5);
	    State *sp = new State(s, tags);
	    if (partition & 0x01) sp->weight_ = Weight(getBytesAndIncrement(a, sizeof(Weight)));
	    else sp->weight_ = semiring()->one();
	    for (; ((partition = *(a++)) & 0x1c) != 0x1c;) {
		Arc *arc = sp->newArc();
		arc->target_ = getBytesAndIncrement(a, (partition >> 6) + 1);
		if (partition & 0x20) arc->weight_ = Weight(u32(getBytesAndIncrement(a, sizeof(Weight))));
		else arc->weight_ = semiring()->one();
		u8 io = partition & 0x1f;
		if (io % 5) arc->input_ = getBytesAndIncrement(a, io % 5);
		else arc->input_ = Epsilon;
		if (type() != TypeAcceptor) {
		    if (io / 5) arc->output_ = getBytesAndIncrement(a, io / 5);
		    else arc->output_ = Epsilon;
		} else arc->output_ = arc->input();
	    }
	    return ConstStateRef(sp);
	}
	return ConstStateRef();
    }

    bool PackedAutomaton::hasState(StateId s) const {
	return bool(getState(s));
    }

    size_t PackedAutomaton::getMemoryUsed() const {
	size_t size = 0;
	if (input_) size += input_->getMemoryUsed();
	if (output_) size += output_->getMemoryUsed();
	return size + sizeof(u8) * arcs_.size() + sizeof(PackedState) * states_.size() + 4 * sizeof(size_t);
    }

    float PackedAutomaton::getStateRatio() const {
	return (nStates_ * sizeof(State)) / float(pStates_ + nStates_ * sizeof(PackedState));
    }

    float PackedAutomaton::getArcRatio() const {
	return (sizeof(Arc) * nArcs_) / float(pArcs_);
    }

    float PackedAutomaton::getRatio() const {
	return (sizeof(Arc) * nArcs_ + nStates_ * sizeof(State)) / float(arcs_.size() + nStates_ * sizeof(PackedState));
    }

    void PackedAutomaton::dumpMemoryUsage(Core::XmlWriter &o) const {
	o << Core::XmlOpen("packed")
	  << Core::XmlFull("state-ratio", getStateRatio())
	  << Core::XmlFull("arc-ratio", getArcRatio())
	  << Core::XmlFull("ratio", getRatio())
	  << Core::XmlFull("total", getMemoryUsed())
	  << Core::XmlClose("packed");
    }

    Core::Ref<PackedAutomaton> packedCopy(ConstAutomatonRef f) {
	PackedAutomaton *result = new PackedAutomaton;
	copy(result, f);
	return Core::ref(result);
    }

} // namespace Fsa
