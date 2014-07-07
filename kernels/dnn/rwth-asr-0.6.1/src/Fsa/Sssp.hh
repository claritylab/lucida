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
#ifndef _FSA_SSSP_HH
#define _FSA_SSSP_HH

#include "hSssp.hh"
#include "Automaton.hh"
#include "Types.hh"

namespace Fsa {
    typedef Ftl::StatePotentials<Weight> StatePotentials;

    typedef Ftl::SsspArcFilter<Automaton> SsspArcFilter;
    typedef Ftl::IsInputLabel<Automaton> IsInputLabel;
    typedef Ftl::AreBothLabels<Automaton> AreBothLabels;

    StatePotentials sssp(ConstAutomatonRef f, StateId start,
			 const SsspArcFilter &arcFilter = SsspArcFilter(), bool progress = false);
    StatePotentials sssp(ConstAutomatonRef f, bool progress = false);

    ConstAutomatonRef pushToInitial(ConstAutomatonRef f, bool progress = false);
    ConstAutomatonRef pushToFinal(ConstAutomatonRef f, bool progress = false);

    ConstAutomatonRef posterior(ConstAutomatonRef f);
    ConstAutomatonRef posterior(ConstAutomatonRef f, const StatePotentials &forward);
    ConstAutomatonRef posterior(ConstAutomatonRef f, Weight &totalInv);

    size_t countPaths(ConstAutomatonRef f);

} // namespace Fsa



namespace Fsa {

    typedef Core::Vector<f64> StatePotentials64;

    void sssp64(ConstAutomatonRef f, StatePotentials64 &);

    /*
     * numerically more stable version of posterior
     * supports only LogSemiring
     * calculate a->weight_ = sum_{path goes through arc a} p(path) / Z
     * and Z = sum_{path} exp(-f(path))
     */
    ConstAutomatonRef posterior64(ConstAutomatonRef f, s32 tol = 100);
    ConstAutomatonRef posterior64(ConstAutomatonRef f, Weight &totalInv, s32 tol = 100);
    ConstAutomatonRef posterior64(ConstAutomatonRef f, f64 &totalInv, s32 tol = 100);
    ConstAutomatonRef posterior64(ConstAutomatonRef f, Weight &total, bool normalize, s32 tol = 100);

    void ssspE(ConstAutomatonRef, ConstAutomatonRef, StatePotentials64&, StatePotentials64&);
    void ssspE(ConstAutomatonRef, ConstAutomatonRef, StatePotentials64&, StatePotentials64&, StatePotentials64&,f32);

    /*
     * calculate posterior automaton with expectation semiring, i.e.
     * first and second automata represent first and second component
     * of this multiplex semiring.
     */
    ConstAutomatonRef posteriorE(
	ConstAutomatonRef f, ConstAutomatonRef r, Weight &expectation,
	bool vNormalized = true, s32 tol = 100);

    ConstAutomatonRef posteriorE(
	ConstAutomatonRef f, ConstAutomatonRef r, Weight &expectation, Weight &totalInv,
	bool vNormalized = true, s32 tol = 100);


    /*
     * calculate expectation of random variable r given the probability model f
     * the same as @param expectation in function posteriorE()
     * (but more efficient)
     */
    Weight expectation(ConstAutomatonRef f, ConstAutomatonRef r);

} // namespace Fsa

#endif // _FSA_SSSP_HH
