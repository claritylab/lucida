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
#ifndef _CORE_DELEGATION_HH_
#define _CORE_DELEGATION_HH_

#include<Core/Assertions.hh>

namespace Core {


    /**
       Implements a kind of the delegation pattern.

       Use set to register either an instance of
       Delegation::Target or a member function of
       type "void C::*(T)", where C is an arbitrary
       class.
       A call to delegate will now delegate the call
       to the registered target.

       Attention:
       If a target is registered or not has to be
       checked externally; do not assume that delegate
       do this check.
    */
    template<typename T>
    class Delegation {
    public:
	struct Target {
	    Target() {}
	    virtual ~Target() {}
	    virtual void operator()(T) = 0;
	};

	template<class TargetClass>
	struct Forward :
	    public Target {

	    typedef void (TargetClass::*TargetMethod)(T);
	    TargetClass & tc;
	    TargetMethod  tm;

	    Forward(TargetClass & tc, TargetMethod tm) :
		tc(tc), tm(tm) {}
	    inline void operator()(T t) { (tc.*tm)(t); }
	};

    private:
	Target * target_;

    public:
	Delegation() :
	    target_(0) {}
	~Delegation() {
	    delete target_;
	}

	void set(Target * target) {
	    delete target_;
	    target_ = target;
	}

	template<class TargetClass>
	void set(TargetClass & tc, void (TargetClass::*tm)(T)) {
	    set(new Forward<TargetClass>(tc, tm));
	}

	void reset() {
	    set(0);
	}

	inline bool hasTarget() {
	    return target_;
	}

	inline void delegate(T t) {
	    require(hasTarget());
	    (*target_)(t);
	}
    };

} // namespace Core

#endif // _CORE_DELEGATION_HH_
