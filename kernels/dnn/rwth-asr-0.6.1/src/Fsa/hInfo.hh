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
#ifndef _H_FSA_INFO_HH
#define _H_FSA_INFO_HH

#include "Types.hh"

namespace Fsa {
    /**
     * An object the contains all information that is gathered by the count
     * function for automata.
     **/
    class AutomatonCounts {
    public:
	StateId maxStateId_;
	StateId nStates_;
	StateId nFinals_;
	size_t nArcs_;
	size_t nIoEps_;
	size_t nIEps_;
	size_t nOEps_;
	size_t nIoFail_;
	size_t nIFail_;
	size_t nOFail_;
    public:
	AutomatonCounts() :
	    maxStateId_(InvalidStateId), nStates_(0), nFinals_(0), nArcs_(0), nIoEps_(0), nIEps_(0), nOEps_(0), nIoFail_(0), nIFail_(0), nOFail_(0) {}
    };
} // namespace Fsa

#endif // _H_FSA_INFO_HH
