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
#include "tAutomaton.hh"
#include "tCache.hh"
#include "Types.hh"
#include <Core/Vector.hh>

namespace Ftl {

    template<class _Automaton>
    class CacheAutomaton : public SlaveAutomaton<_Automaton> {
	typedef SlaveAutomaton<_Automaton> Precursor;
    public:
	typedef typename _Automaton::State _State;
	typedef Core::Ref<const _State> _ConstStateRef;
	typedef Core::Ref<const _Automaton> _ConstAutomatonRef;
    private:
	u32 maxAge_;
	mutable u32 age_;
	mutable Core::Vector<u8> ages_;
	mutable Core::Vector<_ConstStateRef> states_;

    public:
	CacheAutomaton(_ConstAutomatonRef f, u32 maxAge) : Precursor(f), maxAge_(maxAge), age_(0) {
	    this->setProperties(Fsa::PropertyCached | Fsa::PropertyStorage, Fsa::PropertyCached);
	}
	virtual ~CacheAutomaton() { states_.erase(states_.begin(), states_.end()); }
	void age() const {
	    if (++age_ > maxAge_) {
		for (Core::Vector<u8>::iterator i = ages_.begin(); i != ages_.end(); ++i)
		    if (!(*i >>= 1)) states_[i - ages_.begin()].reset();
		age_ = 0;
	    }
	}
	virtual _ConstStateRef getState(Fsa::StateId s) const {
	    age();
	    states_.grow(s);
	    if (!states_[s]) states_[s] = Precursor::fsa_->getState(s);
	    ages_.grow(s, 0);
	    ages_[s] |= 0x80;
	    return states_[s];
	}
	virtual size_t getMemoryUsed() const {
	    size_t size = 0;
	    for (typename Core::Vector<_ConstStateRef>::iterator i = states_.begin(); i != states_.end(); ++i)
		if ((*i) && ((*i)->refCount() == 1)) size += (*i)->getMemoryUsed();
	    return Precursor::fsa_->getMemoryUsed() + 2 * sizeof(u32) + ages_.capacity() * sizeof(u8)
		+ states_.capacity() * sizeof(_ConstStateRef) + size;
	}
	virtual std::string describe() const {
	    return Core::form("cache(%s,%d)", Precursor::fsa_->describe().c_str(), maxAge_);
	}
    };

    template<class _Automaton>
    Core::Ref<const _Automaton> cache(Core::Ref<const _Automaton> f, u32 maxAge) {
	if (f->hasProperty(Fsa::PropertyCached | Fsa::PropertyStorage)) return f;
	return Core::Ref<const _Automaton>(new CacheAutomaton<_Automaton>(f, maxAge));
    }

} // namespace Ftl
