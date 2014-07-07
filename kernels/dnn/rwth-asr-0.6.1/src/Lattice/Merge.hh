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
#ifndef _LATTICE_MERGE_HH
#define _LATTICE_MERGE_HH

#include "Lattice.hh"
#include <Core/ReferenceCounting.hh>

namespace Bliss {
    class OrthographicParser;
}

namespace Lm {
    class LanguageModel;
}

namespace Lattice {

    /**
     * @param numerator is incorporated into @param denominator
     * without duplicating sentence hypotheses of @param denominator.
     * The resulting word lattice is word-conditioned w.r.t. the
     * given language model.
     */
    ConstWordLatticeRef merge(
	ConstWordLatticeRef numerator,
	ConstWordLatticeRef denominator,
	Core::Ref<const Lm::LanguageModel>);

    /**
     * Extract all sentence hypotheses from @param denominator which
     * share the orthography @param orth.
     */
    ConstWordLatticeRef extractNumerator(
	const std::string &orth,
	ConstWordLatticeRef denominator,
	Bliss::OrthographicParser *orthToLemma,
	Fsa::ConstAutomatonRef lemmaPronToLemma,
	Fsa::ConstAutomatonRef lemmaToLemmaConfusion);

    /**
     * Generate a lattice which shares the topology with @param denominator,
     * and the arc weights are set as follows:
     *    weight of input @param denominator: a path corresponding to the
     *        orthography @param orth goes through this arc
     *    0: otherwise
     */
    ConstWordLatticeRef turnOffCompetingHypotheses(
	const std::string &orth,
	ConstWordLatticeRef denominator,
	Bliss::OrthographicParser *orthToLemma,
	Fsa::ConstAutomatonRef lemmaPronToLemma,
	Fsa::ConstAutomatonRef lemmaToLemmaConfusion);

} // namespace Lattice

#endif //_LATTICE_MERGE_HH
