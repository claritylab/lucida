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
#ifndef _LM_COMPOSE_HH
#define _LM_COMPOSE_HH

#include <Lm/ScaledLanguageModel.hh>
#include <Fsa/Compose.hh>
#include <Fsa/Mapping.hh>

namespace Lm {

    class ComposeAutomaton :
	public Fsa::Automaton
    {
    public:
	/**
	 * State in the original automaton.
	 * Given a state of the composition result, return the
	 * corresponding original state of the automaton (left).
	 */
	virtual Fsa::StateId leftStateId(Fsa::StateId) const = 0;

	/**
	 * Mapping returning the state's id in the left original automaton.
	 **/
	Fsa::ConstMappingRef mapToLeft() const;

	/**
	 * History in the language model.
	 * Given a state of the composition result, return the
	 * corresponding history of the language model (right).
	 */
	virtual History history(Fsa::StateId) const = 0;
    };

    /**
     * Add language model scores to a Fsa automaton.
     *
     * This is theoretically almost equivalent to using Fsa::compose()
     * with the FSA representation of the language model obtained from
     * getFsa(), but there are the following differences:
     * - This function works for any LanguageModel, even if it does not
     *   implement getFsa().
     * - Since the LanguageModel interface presents a deterministic
     *   epsilon-free automaton, this alogrithm is simpler.
     * - No back-off paths are included.  So the result is also correct
     *   if getFsa() suffers from the back-off arc shortcut phenomenon.
     * - As a corrolary: If the (left) automaton is epsilon-free, so
     *   is the result
     *
     * The alphabets of @c left should meet one of the following requirements:
     * 1) The output alphabet is the lemma alphabet of the lexicon used
     *    by the language model.
     * 2) The input alphabet of @c left is the lemma pronunciation alphabet
     *    of the lexicon used by the language model.
     * In case of ties 1) is chosen.
     */
    Core::Ref<const ComposeAutomaton> compose(
	Fsa::ConstAutomatonRef left,
	Core::Ref<const Lm::LanguageModel> right,
	Score lmScale,
	Score syntaxEmissionScale = 1.0);

    Core::Ref<const ComposeAutomaton> compose(
	Fsa::ConstAutomatonRef left,
	Core::Ref<const Lm::ScaledLanguageModel> right,
	Score syntaxEmissionScale = 1.0);

    Core::Ref<const ComposeAutomaton> composePron(
	Fsa::ConstAutomatonRef left,
	Core::Ref<const Lm::ScaledLanguageModel> right,
	Score pronunciationScale,
	Score syntaxEmissionScale = 1.0);

} // namespace Lm

#endif // _LM_COMPOSE_HH
