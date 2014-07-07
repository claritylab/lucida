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
// $Id: ArpaLm.hh 9621 2014-05-13 17:35:55Z golik $

#ifndef _LM_ARPALM_HH
#define _LM_ARPALM_HH

#include "BackingOff.hh"
#include "ClassLm.hh"
#include "LanguageModel.hh"
#include <Core/Parameter.hh>

namespace Lm {

    /**
     * ARPA backing-off language model.
     *
     * Actually just a reader for the ARPA standard format introduced
     * by Doug Paul.  The LM implementation is just inherited from
     * BackingOffLm.
     *
     * Edited quote from documentation header by CMU software:
     *
     * p(w3|w1,w2) = if (trigram exists)       p_3(w1,w2,w3)
     *          else if (bigram w1,w2 exists)  bo_wt_2(w1,w2)*p(w3|w2)
     *          else                           p(w3|w2)
     *
     * p(w2|w1)    = if (bigram exists)         p_2(w1,w2)
     *          else if (unigram w1 exists)     bo_wt_1(w1)*p_1(w2)
     *          else                            p_1(w2)
     *
     * All probs and back-off weights (bo_wt) are given in log10 form.
     * Missing back-off weights are set to zero.
     *
     * Data format:
     *
     * \data\
     * ngram 1=nr            # number of unigrams
     * ngram 2=nr            # number of bigrams
     * ngram 3=nr            # number of trigrams
     *
     * \1-grams:
     * p_1   w1      bo_wt_1
     *
     * \2-grams:
     * p_2   w1 w2   bo_wt_2
     *
     * \3-grams:
     * p_3   w1 w2 w3
     *
     * \end\
     *
     * Notes:
     * - Format and implementation are not restricted to tri-grams.
     * - Currently we assume that the LM file is in UTF-8 encoding.
     */

    class ArpaLm :
	public BackingOffLm
    {
    private:
	class InitData;
	static const Core::ParameterString paramFilename;
	static const Core::ParameterString paramEncoding;
	static const Core::ParameterBool paramSkipInfScore;
	static const Core::ParameterBool paramReverseLm;
	static const f64 InfScore;
	virtual void read();
    public:
	ArpaLm(const Core::Configuration&, Bliss::LexiconRef);
	virtual ~ArpaLm();
    };


    class ArpaClassLm : public ArpaLm, public ClassLm
    {
    private:
	ConstClassMappingRef mapping_;
    protected:
	void loadClasses();

	virtual Token getSpecialToken(
	    const std::string &name, bool required = false) const;

    public:
	ArpaClassLm(const Core::Configuration&, Bliss::LexiconRef);
	virtual ~ArpaClassLm();

	virtual ConstClassMappingRef classMapping() const
	    { return mapping_; }

	virtual History extendedHistory(const History&, Token t) const;
	virtual Score score(const History&, Token t) const;
	virtual Fsa::ConstAutomatonRef getFsa() const;
    };

} // namespace Lm

#endif // LM_ARPALM_HH
