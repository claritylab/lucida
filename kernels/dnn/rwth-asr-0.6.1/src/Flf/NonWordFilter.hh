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
#ifndef _FLF_NON_WORD_FILTER_HH
#define _FLF_NON_WORD_FILTER_HH

#include "FlfCore/Lattice.hh"
#include "Network.hh"


namespace Flf {

    /**
     * Epsilon Closure Filter:
     *
     * Given states s and e. Pathes_w(s,e) is the set of all pathes from
     * s to e having exactly one arc labeled with w and all others labeled
     * with epsilon. Arcs_w(s,e) is the set of all arcs in Pathes_w(s,e)
     * labeled with w. Arcs_s'/w(s,e) is the set of all arcs in Arcs_w(s,e)
     * having source state s'. Pathes_s'/w(s,e) is the subset of Pathes_w(s,e),
     * such that each path in Pathes_s'/w(s,e) includes an arc in Arcs_s'/w(s,e).
     *
     * for each w, (s,e):
     * 1) for each a in Arcs_w(s,e) keep only the best scoring path in Pathes_w(s,e)
     *    that includes a.
     * -> classical epsilon-removal over the tropical semiring
     * 2) for each s' keep only the best scoring path in Pathes_s'/w(s,e)
     * -> classical epsilon-removal over the tropical semiring
     *    with statewise determinization
     * 3) keep only the best scoring path in Pathes_w(s,e)
     * -> classical epsilon-removal over the tropical semiring
     *    with determinization over all pathes from s to e;
     *    attention:
     *    under some circumstances the determinization is not given;
     *    the problem could be avoided by introducing new states and arcs,
     *    but that could result in a (huge???) blow up of the lattice size
     *
     * Arcs and states are removed, but never new states and arcs are intorduced.
     * The implementation is static, i.e not lazy.
     **/
    StaticLatticeRef applyEpsClosureFilter(ConstLatticeRef l);
    StaticLatticeRef applyEpsClosureWeakDeterminizationFilter(ConstLatticeRef l);
    StaticLatticeRef applyEpsClosureStrongDeterminizationFilter(ConstLatticeRef l);


    /**
     * Non-word Closure Filter:
     *
     * The non-word filter nodes work only on acceptors:
     * mapOutputToInput(epsClosure...Filter(mapNonWordToEps(mapInputToOutput(l))))
     *
     * The three strategies corresponds to the three eps-closure filter strategies
     * (see above).
     **/
    NodeRef createNonWordClosureFilterNode(
	const std::string &name, const Core::Configuration &config);
    NodeRef createNonWordClosureWeakDeterminizationFilterNode(
	const std::string &name, const Core::Configuration &config);
    NodeRef createNonWordClosureStrongDeterminizationFilterNode(
	const std::string &name, const Core::Configuration &config);



    /**
     * Epsilon Closure Normalization:
     *
     * For each state s and each state s_end being a final state of the
     * eps-closure of s, keep exactly one eps-arc having the correct
     * score w.r.t to the used semiring (i.e. best score for tropical
     * semiring) and occupying the time needed for "crossing" the closure.
     *
     * The implementation is lazy.
     **/
    ConstLatticeRef normalizeEpsClosure(ConstLatticeRef l);

    /**
     * Non-Word Closure Normalization:
     *
     * The non-word normalization node works only on acceptors:
     * normalizeEpsClosure(mapInputToOutput(mapNonWordToEps(l)))
     **/
    NodeRef createNonWordClosureNormalizationFilterNode(
	const std::string &name, const Core::Configuration &config);



    /**
     * Non-Word Closure Removal:
     *
     * see epsilonRemoval and fastEpsilonRemoval
     * Attention: resulting word boundaries might be incorrect
     *
     * The non-word removal node works only on acceptors:
     * removeEpsClosure(mapInputToOutput(mapNonWordToEps(l)))
     **/
    NodeRef createNonWordClosureRemovalFilterNode(
	const std::string &name, const Core::Configuration &config);

} // namespace Flf

#endif // _FLF_NON_WORD_FILTER_HH
