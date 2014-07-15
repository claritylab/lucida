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
#ifndef _FLOW_QUEUE_HH
#define _FLOW_QUEUE_HH


#include <queue>

#include <Core/Thread.hh>
#include "Data.hh"


namespace Flow {

    class Queue {
    private:
	mutable Core::Mutex m_;
	std::queue<DataPtr<Data> > l_;
	mutable Core::Condition c_;

    public:
	inline void lock() const { m_.lock(); }
	inline void release() const { m_.release(); }

	inline void clear() { while (l_.size()) l_.pop(); }
	inline size_t size() const { return l_.size(); }
	inline bool isEmpty() const { return (l_.size() == 0); }
	inline bool isEmptyAtomar() const {
	    lock();
	    bool is_empty = isEmpty();
	    release();
	    return is_empty;
	}

	inline void put(Data* d) { l_.push(DataPtr<Data>(d)); }
	inline void putAtomar(Data* d) {
	    lock();
	    l_.push(DataPtr<Data>(d));
	    release();
	    c_.signal(true);
	}
	template<class T> inline void get(DataPtr<T> &d) {
	    d = l_.front();
	    l_.pop();
	}
	template<class T> inline void getBlocking(DataPtr<T> &d) {
	    if (isEmptyAtomar()) c_.wait();
	    d = l_.front();
	    l_.pop();
	}
    };

}


#endif // _FLOW_QUEUE_HH
