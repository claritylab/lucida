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
// $Id: check.cc 8249 2011-05-06 11:57:02Z rybach $

/**
 * This program illustrates typical use of some SprintCore functions.
 */

#include "Application.hh"
#include "Automaton.hh"
#include "Basic.hh"
#include "Cache.hh"
#include "Input.hh"
#include "Output.hh"
#include "Resources.hh"
#include "Static.hh"

// ===========================================================================
// Application

	/**
	 * queue:
	 * - end of queue is defined as self-referencing state id
	 **/
	class SsspQueue {
	protected:
		Fsa::StateId head_;
		Core::Vector<Fsa::StateId> queue_;
	public:
		SsspQueue() :
			head_(Fsa::InvalidStateId) {
		}
		bool empty() const {
			return head_ == Fsa::InvalidStateId;
		}
		Fsa::StateId dequeue() {
			require_(!empty());
			Fsa::StateId s = head_;
			head_ = queue_[s];
			if (head_ == s)
				head_ = Fsa::InvalidStateId;
			queue_[s] = Fsa::InvalidStateId;
			return s;
		}
		Fsa::StateId maxStateId() const {
			return Fsa::InvalidStateId;
		}
	};

	class FifoSsspQueue : public SsspQueue {
	public:
		void enqueue(Fsa::StateId s) {
			queue_.grow(s, Fsa::InvalidStateId);
			if (queue_[s] == Fsa::InvalidStateId) {
				if (head_ != Fsa::InvalidStateId)
					queue_[s] = head_;
				else
					queue_[s] = s;
				head_ = s;
			}
		}
	};


class MyApplication : public Fsa::Application {
public:
    std::string getUsage() const {
	return "...";
    }

    int main(const std::vector<std::string> &arguments) {


	FifoSsspQueue q;
	for(Fsa::StateId i=1; i<20; ++i)
		q.enqueue(i);
	while(!q.empty()) {
		Fsa::StateId s = q.dequeue();
		std::cout << s << std::endl;
	}

	return 0;
    }
} app; // <- You have to create ONE instance of the application

APPLICATION
