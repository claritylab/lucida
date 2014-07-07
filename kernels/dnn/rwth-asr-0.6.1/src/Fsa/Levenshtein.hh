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
#ifndef _FSA_LEVENSHTEIN_HH
#define _FSA_LEVENSHTEIN_HH

#include "Automaton.hh"

namespace Fsa {

    // create levenshtein alignment graph
    ConstAutomatonRef levenshtein(ConstAutomatonRef ref, ConstAutomatonRef test,
				  f32 delCost = 1.0, f32 insCost = 1.0, f32 subCost = 1.0, f32 corCost = 0.0);

    struct LevenshteinInfo {
	size_t del_;
	size_t ins_;
	size_t sub_;
	size_t total_;
    };

    // calculate standard levenshtein statistics
    LevenshteinInfo levenshteinInfo(ConstAutomatonRef levensh);

} // namespace

#endif // _FSA_LEVENSHTEIN_HH
