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
#include "ReferenceCounting.hh"
#include "Hash.hh"
#include "Utility.hh"

using namespace Core;


namespace Core {

    /**
     * Central manager for weak references.
     *
     * When a reference counted object is deleted, we need to reset
     * any weak reference to it.  For this reason, we keep track of
     * all existing weak references.
     *
     * ReferenceManager is a Singleton.
     */

    class ReferenceManager {
    private:
	static ReferenceManager *us_;
	typedef const ReferenceCounted * const Pointee;
	typedef WeakRefBase * Reference;
	typedef __gnu_cxx::hash_multimap<
	    Pointee, Reference,
	    conversion<Pointee, size_t> > Map;
	Map map_;
    public:
	static ReferenceManager *us() {
	    if (!us_) us_ = new ReferenceManager;
	    return us_;
	}

	void registerWeakReference(Pointee object, Reference reference) {
	    map_.insert(std::make_pair(object, reference));
	}

	void unregisterWeakReference(Pointee object, Reference reference) {
	    std::pair<Map::iterator, Map::iterator> rr = map_.equal_range(object);
	    for (Map::iterator ii = rr.first; ii != rr.second; ++ii) {
		if (ii->second == reference) {
		    map_.erase(ii);
		    break;
		}
	    }
	}

	void notifyDeletion(Pointee object) {
	    std::pair<Map::iterator, Map::iterator> rr = map_.equal_range(object);
	    for (Map::iterator ii = rr.first; ii != rr.second; ++ii) {
		(ii->second)->invalidate();
	    }
	    map_.erase(rr.first, rr.second);
	}
    };

    ReferenceManager *ReferenceManager::us_ = 0;

};

ReferenceCounted::~ReferenceCounted() { }

void ReferenceCounted::acquireWeakReference(WeakRefBase *reference) const {
    // require(*reference == this);
    if (!isSentinel(this))
	ReferenceManager::us()->registerWeakReference(this, reference);
}

void ReferenceCounted::releaseWeakReference(WeakRefBase *reference) const {
    // require(*reference == this);
    if (!isSentinel(this))
	ReferenceManager::us()->unregisterWeakReference(this, reference);
}

void ReferenceCounted::free() const {
    require_(!referenceCount_);
    verify_(!isSentinel(this));
    ReferenceManager::us()->notifyDeletion(this);
    delete this;
}
