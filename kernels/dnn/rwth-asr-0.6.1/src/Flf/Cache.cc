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
#include "FlfCore/Ftl.hh"
#include "FlfCore/Traverse.hh"
#include "Cache.hh"

namespace Flf {

    // -------------------------------------------------------------------------
    class CacheBuilder : public TraverseState {
	typedef TraverseState Precursor;
    private:
	const Boundaries &boundaries_;
	StateBoundaryList &stateBoundaries_;
    public:
	CacheBuilder(ConstLatticeRef l, StateBoundaryList &stateBoundaries) :
	    Precursor(l),
	    boundaries_(*l->getBoundaries()),
	    stateBoundaries_(stateBoundaries) {
	    traverse();
	}
	virtual ~CacheBuilder() {}

	void exploreState(ConstStateRef sr) {
	    Fsa::StateId sid = sr->id();
	    stateBoundaries_.grow(sid);
	    stateBoundaries_[sid].state = sr;
	    stateBoundaries_[sid].boundary = boundaries_.get(sid);
	}
    };

    ConstStateBoundaryListRef cacheLattice(ConstLatticeRef l) {
	StateBoundaryList *stateBoundaries = new StateBoundaryList;
	CacheBuilder cacheBuilder(l, *stateBoundaries);
	return ConstStateBoundaryListRef(stateBoundaries);
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    ConstLatticeRef cache(ConstLatticeRef l, u32 maxAge) {
	return FtlWrapper::cache(l, maxAge);
    }

    class CacheNode : public FilterNode {
    public:
	static const Core::ParameterInt paramMaxAge;
    private:
	u32 maxAge_;
    protected:
	virtual ConstLatticeRef filter(ConstLatticeRef l) {
	    return l ? cache(l, maxAge_) : l;
	}
    public:
	CacheNode(const std::string &name, const Core::Configuration &config) :
	    FilterNode(name, config) {
	    maxAge_ = paramMaxAge(config);
	}
	virtual ~CacheNode() {}
    };
    const Core::ParameterInt CacheNode::paramMaxAge(
	"max-age",
	"max. age of cache entry",
	10000);

    NodeRef createCacheNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new CacheNode(name, config));
    }
    // -------------------------------------------------------------------------

} // namespace Flf
