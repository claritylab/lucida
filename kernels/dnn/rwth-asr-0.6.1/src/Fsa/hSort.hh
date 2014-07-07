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
#ifndef _H_FSA_SORT_HH
#define _H_FSA_SORT_HH

#include <functional>

namespace Fsa {
    typedef enum {
	SortTypeNone,
	SortTypeByArc,
	SortTypeByInput,
	SortTypeByInputAndOutput,
	SortTypeByInputAndTarget,
	SortTypeByInputAndOutputAndTarget,
	SortTypeByOutput,
	SortTypeByWeight
    } SortType;
} // namespace Fsa


namespace Ftl {
    template<class _Automaton>
    struct byArc : public std::binary_function<typename _Automaton::Arc, typename _Automaton::Arc, bool> {
	typename _Automaton::ConstSemiringRef semiring_;
	byArc(typename _Automaton::ConstSemiringRef semiring) : semiring_(semiring) {}
	bool operator()(const typename _Automaton::Arc &a, const typename _Automaton::Arc &b) const {
	    if (a.input()  < b.input())  return true;
	    if (a.input()  > b.input())  return false;
	    if (a.output() < b.output()) return true;
	    if (a.output() > b.output()) return false;
	    return semiring_->compare(a.weight(), b.weight()) < 0;
	}
    };

    template<class _Automaton>
    struct byInput : public std::binary_function<typename _Automaton::Arc, typename _Automaton::Arc, bool> {
	bool operator()(const typename _Automaton::Arc &a, const typename _Automaton::Arc &b) const {
	    return a.input() < b.input();
	}
    };

    template<class _Automaton>
    struct byOutput : public std::binary_function<typename _Automaton::Arc, typename _Automaton::Arc, bool> {
	bool operator()(const typename _Automaton::Arc &a, const typename _Automaton::Arc &b) const {
	    return a.output() < b.output();
	}
    };

    template<class _Automaton>
    struct byInputAndTarget : public std::binary_function<typename _Automaton::Arc, typename _Automaton::Arc, bool> {
	bool operator()(const typename _Automaton::Arc &a, const typename _Automaton::Arc &b) const {
	    if (a.input() < b.input()) return true;
	    if (a.input() > b.input()) return false;
	    return a.target() < b.target();
	}
    };

    template<class _Automaton>
    struct byInputAndOutput : public std::binary_function<typename _Automaton::Arc, typename _Automaton::Arc, bool> {
	bool operator()(const typename _Automaton::Arc &a, const typename _Automaton::Arc &b) const {
	    if (a.input()  < b.input())  return true;
	    if (a.input()  > b.input())  return false;
	    return a.output() < b.output();
	}
    };

    template<class _Automaton>
    struct byInputAndOutputAndTarget : public std::binary_function<typename _Automaton::Arc, typename _Automaton::Arc, bool> {
	bool operator()(const typename _Automaton::Arc &a, const typename _Automaton::Arc &b) const {
	    if (a.input()  < b.input())  return true;
	    if (a.input()  > b.input())  return false;
	    if (a.output() < b.output()) return true;
	    if (a.output() > b.output()) return false;
	    return a.target() < b.target();
	}
    };

    template<class _Automaton>
    struct byWeight : public std::binary_function<typename _Automaton::Arc, typename _Automaton::Arc, bool> {
	typename _Automaton::ConstSemiringRef semiring_;
	byWeight(typename _Automaton::ConstSemiringRef semiring) : semiring_(semiring) {}
	bool operator()(const typename _Automaton::Arc &a, const typename _Automaton::Arc &b) const {
	    return semiring_->compare(a.weight(), b.weight()) < 0;
	}
    };
} // namespace Ftl

#endif // _H_FSA_SORT_HH
