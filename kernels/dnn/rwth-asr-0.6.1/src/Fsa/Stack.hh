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
#ifndef _FSA_STACK_HH
#define _FSA_STACK_HH

#include <stack>

namespace Fsa {

    template<class T> class Stack : public std::vector<T> {
    public:
	typedef std::vector<T> Precursor;
	typedef typename Precursor::reverse_iterator iterator;
	typedef typename Precursor::const_reverse_iterator const_iterator;
    public:
	void clear() { Precursor::erase(Precursor::begin(), Precursor::end()); }
	bool isEmpty() const { return (this->size() == 0); }
	void push(const T &t) { Precursor::push_back(t); }
	void push(const std::vector<T> &t) { Precursor::insert(Precursor::end(), t.begin(), t.end()); }
	T top() const { return this->back(); }
	T pop() { T t = this->back(); Precursor::erase(Precursor::end() - 1, Precursor::end()); return t; }
	iterator begin() { return this->rbegin(); }
	const_iterator begin() const { return this->rbegin(); }
	iterator end() { return this->rend(); }
	const_iterator end() const { return this->rend(); }
	size_t getMemoryUsed() const { return sizeof(T) * this->size(); }
    };

} // namespace Fsa

#endif // _FSA_STACK_HH
