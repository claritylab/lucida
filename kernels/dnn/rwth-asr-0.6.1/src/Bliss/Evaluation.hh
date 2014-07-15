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
// $Id: Evaluation.hh 9621 2014-05-13 17:35:55Z golik $

#ifndef _BLISS_EVALUATION_HH
#define _BLISS_EVALUATION_HH

#include <Bliss/EditDistance.hh>
#include <Core/Component.hh>
#include <Fsa/Automaton.hh>
namespace Bliss {
    class ErrorStatistic;
    class Lexicon;
    class OrthographicParser;
}

namespace Bliss {

    class Evaluator :
	public Core::Component
    {
    private:
	static const Core::ParameterBool paramWordErrors;
	static const Core::ParameterBool paramLetterErrors;
	static const Core::ParameterBool paramFilterLettersByEvalTokens;
	static const Core::ParameterBool paramPhonemeErrors;

	bool shallComputeWordErrors_;
	bool shallComputeLetterErrors_;
	bool shallFilterLettersByEvalTokens_;
	bool shallComputePhonemeErrors_;

	Core::Ref<const Lexicon> lexicon_;
	Core::Ref<const OrthographicParser> orthParser_;
	Fsa::ConstAutomatonRef lemmaPronToLemma_;
	Fsa::ConstAutomatonRef lemmaPronToPhoneme_;
	Fsa::ConstAutomatonRef lemmaToSynt_;
	Fsa::ConstAutomatonRef lemmaToEval_;
	//	Fsa::ConstAutomatonRef lemmaToPreferredEval_;
	Fsa::ConstAutomatonRef lemmaToLemmaConfusion_;
	Fsa::ConstAutomatonRef lemmaToLetter_;
	Fsa::ConstAutomatonRef lemmaToPhoneme_;
	class LetterAcceptorBuilder;
	LetterAcceptorBuilder *letterAcceptorBuilder_;
	Bliss::EditDistance *editDistance_;
	struct {
	    Fsa::ConstAutomatonRef lemma, eval, phon, orth;
	} correct_;
	Core::XmlChannel graphStatisticsChannel_;
	Core::XmlChannel graphDumpChannel_;

	void reportLatticeDensity(
	    Fsa::ConstAutomatonRef lattice,
	    const Bliss::ErrorStatistic &latticeWordErrors);

	Bliss::ErrorStatistic evaluateMetric(
	    Fsa::ConstAutomatonRef correct,
	    Fsa::ConstAutomatonRef candidate,
	    const std::string &type,
	    const std::string &name);

    public:
	Evaluator(const Core::Configuration&,
		  Core::Ref<const Lexicon>);
	~Evaluator();

	void setReferenceTranscription(const std::string&);

	/**
	 * Evaluate a word sequence or lattice.
	 *
	 * For "normal" single-best error rates, make sure that the
	 * automaton passed is linear.  For non-linear automata oracle
	 * error rates are computed.  The best sentence hypothesis in
	 * the lattice (according to the evaluation metric) is
	 * reported.
	 *
	 * @param lemmaPron an acceptor representing the single-best
	 * hypothesis or a word lattice.  The labels must be
	 * lemma-pronunciations.
	 * @param name a string identifier that will be attached to
	 * the result.
	 */
	u32 evaluateWords(
	    Fsa::ConstAutomatonRef lemmaPron,
	    const std::string &name);

	/**
	 * Evaluate a phoneme sequence or lattice.
	 *
	 * This function is intended for use in a phoneme recognizer.
	 * The phoneme error rate of a word sequence or lattice can be
	 * evaluated using evaluateWords().
	 *
	 * For "normal" single-best error rates, make sure that the
	 * automaton passed is linear.  For non-linear automata oracle
	 * error rates are computed.  The best sentence hypothesis in
	 * the lattice (according to the evaluation metric) is
	 * reported.
	 *
	 * @param phonemes an acceptor representing the single-best
	 * hypothesis or a phoneme lattice.  The labels must be
	 * phonemes.
	 * @param name a string identifier that will be attached to
	 * the result.
	 */
	u32 evaluatePhonemes(
	    Fsa::ConstAutomatonRef phonemes,
	    const std::string &name);

	/**
	 * Evaluate a sequence or a lattice.
	 * This function automatically dispatches to evaluateWords()
	 * or evaluatePhonemes().
	 */
	u32 evaluate(
	    Fsa::ConstAutomatonRef,
	    const std::string &name);
    };

} // namespace Bliss

#endif //_BLISS_EVALUATION_HH
