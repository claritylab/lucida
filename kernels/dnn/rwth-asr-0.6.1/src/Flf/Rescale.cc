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
#include <Core/Parameter.hh>

#include "FlfCore/Basic.hh"
#include "Rescale.hh"

namespace Flf {

    // -------------------------------------------------------------------------
    /*
      - Rescale lattice, i.e. change semiring scales (and semiring)
    */
    class RescaleNode : public FilterNode {
    public:
	static const Core::ParameterFloat paramScale;
	static const Core::ParameterString paramKey;
    private:
	ConstSemiringRef lastSemiring_;
	ConstSemiringRef lastRescaledSemiring_;
    private:
	virtual ConstLatticeRef filter(ConstLatticeRef l) {
	    if (!l)
		return l;
	    if (!lastRescaledSemiring_ || (l->semiring().get() != lastSemiring_.get())) {
		lastSemiring_ = l->semiring();
		lastRescaledSemiring_ = cloneSemiring(l->semiring());
		ScoreId id = 0;
		for (KeyList::const_iterator itKey = l->semiring()->keys().begin();
		     itKey != l->semiring()->keys().end(); ++itKey, ++id) {
		    if (itKey->empty())
			warning("RescaleNode: Dimension %zu has no symbolic name", id);
		    else {
			Score scale = paramScale(select(*itKey));
			if (scale != Semiring::UndefinedScale)
			    lastRescaledSemiring_->setScale(id, scale);
			Key key = paramKey(select(*itKey));
			if (!key.empty())
			    lastRescaledSemiring_->setKey(id, key);
		    }
		}
	    }
	    return changeSemiring(l, lastRescaledSemiring_);
	}
    public:
	RescaleNode(const std::string &name, const Core::Configuration &config) :
	    FilterNode(name, config) {}
	virtual ~RescaleNode() {}
    };
    const Core::ParameterFloat RescaleNode::paramScale(
	"scale",
	"new scale",
	Semiring::UndefinedScale);
    const Core::ParameterString RescaleNode::paramKey(
	"key",
	"new key name",
	"");

    NodeRef createRescaleNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new RescaleNode(name, config));
    }
    // -------------------------------------------------------------------------

} // namespace Flf
