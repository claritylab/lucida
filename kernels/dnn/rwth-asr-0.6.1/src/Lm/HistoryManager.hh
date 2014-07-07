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
// $Id: HistoryManager.hh 5439 2005-11-09 11:05:06Z bisani $

#ifndef _LM_HISTORYMANAGER_HH
#define _LM_HISTORYMANAGER_HH

#include <Core/ReferenceCounting.hh>

namespace Lm {

    /**
     * Abstract Language Model State.
     */
    typedef const void *HistoryHandle;
    typedef size_t HistoryHash;

    /**
     * The HistoryManager serves as a memory manger for the
     * HistoryDescriptors used by a LanguageModel.  Each LanguageModel
     * creates an appropriate HistoryManager.  Clients should never
     * bother about this class.  HistoryManager also plays the role of
     * a virtual method table for HistoryHandle.
     *
     */
    class HistoryManager {
    protected:
	HistoryManager() {};

	friend class History;

	/**
	 * Create a new reference to this history.
	 * This function is called whenever a new pointer to this
	 * history is required.  You may instantiate a copy, increase
	 * a reference count, or whatever
	 */
	virtual HistoryHandle acquire(HistoryHandle) = 0;

	/**
	 * Destroy a reference to this history.
	 * This function should be redefined to complement the chosen
	 * strategy fo acquire()
	 */
	virtual void release(HistoryHandle) = 0;

	/**
	 * If isEquivalent() is true For two descriptors, hashKey()
	 * must return the same value for both of them.
	 */
	virtual HistoryHash hashKey(HistoryHandle) const = 0;

	/**
	 * Test if two descriptors represent the same context.
	 * Note: The "check for equal pointers" optimization is
	 * already done before this function is called.
	 */
	virtual bool isEquivalent(HistoryHandle, HistoryHandle) const = 0;

	virtual std::string format(HistoryHandle hd) const {
	    return Core::form("n%p", hd);
	}

    public:
	virtual ~HistoryManager() {};
    };


    class SingletonHistoryManager : public HistoryManager {
    protected:
	virtual HistoryHandle acquire(HistoryHandle) { return 0; }
	virtual void release(HistoryHandle) {}
	virtual HistoryHash hashKey(HistoryHandle) const { return 0; }
	virtual bool isEquivalent(HistoryHandle, HistoryHandle) const { return true; }
	virtual std::string format(HistoryHandle) const {
	    return "singleton";
	}
    };

    /**
     * Manager for statically allocated HistoryHandles.
     * This is very trivial: aquire() and release() are no-ops.
     * hashKey() and isEquivalent() use the descriptors' address in
     * memory.
     **/

    class StaticHistoryManager : public HistoryManager {
    protected:
	virtual HistoryHandle acquire(HistoryHandle hd) {
	    return hd;
	}

	virtual void release(HistoryHandle hd) {}

	virtual HistoryHash hashKey(HistoryHandle hd) const {
	    return HistoryHash(hd);
	}

	virtual bool isEquivalent(HistoryHandle lhs, HistoryHandle rhs) const {
	    return lhs == rhs;
	}
    };


    /**
     * HistoryManager for reference counted descriptors.
     * The concrete HistoryHandle must inherit Core::ReferenceCounted.
     */
    class ReferenceCountingHistoryManager : public HistoryManager {
    protected:
	virtual HistoryHandle acquire(HistoryHandle hd) {
	    const Core::ReferenceCounted *rc = static_cast<const Core::ReferenceCounted*>(hd);
	    rc->acquireReference();
	    return hd;
	}

	virtual void release(HistoryHandle hd) {
	    const Core::ReferenceCounted *rc = static_cast<const Core::ReferenceCounted*>(hd);
	    if (rc->releaseReference()) rc->free();
	}
    };

} // namespace Lm

#endif // _LM_HISTORYMANAGER_HH
